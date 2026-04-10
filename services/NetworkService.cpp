#include "NetworkService.h"

#include "LoggerService.h"
#include "ProcessService.h"

#include <QByteArray>
#include <QHostAddress>
#include <QSet>

#include <algorithm>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#endif

namespace {

QString normalizeAdapterId(const QString &value)
{
    QString normalized = value.trimmed();
    if (normalized.startsWith(QLatin1Char('{')) && normalized.endsWith(QLatin1Char('}')) && normalized.size() > 2) {
        normalized = normalized.mid(1, normalized.size() - 2);
    }

    return normalized.toLower();
}

#ifdef Q_OS_WIN

QString fromWide(const wchar_t *value)
{
    return value ? QString::fromWCharArray(value) : QString();
}

QString errorMessage(const QString &action, DWORD errorCode)
{
    QString message = QStringLiteral("%1失败，错误码 %2。").arg(action).arg(errorCode);
    if (errorCode == ERROR_ACCESS_DENIED) {
        message += QStringLiteral(" 请以管理员身份运行程序。");
    }

    return message;
}

QString operStatusToString(IF_OPER_STATUS status)
{
    switch (status) {
    case IfOperStatusUp:
        return QStringLiteral("Up");
    case IfOperStatusDown:
        return QStringLiteral("Down");
    case IfOperStatusDormant:
        return QStringLiteral("Dormant");
    case IfOperStatusLowerLayerDown:
        return QStringLiteral("LowerLayerDown");
    case IfOperStatusTesting:
        return QStringLiteral("Testing");
    case IfOperStatusNotPresent:
        return QStringLiteral("NotPresent");
    default:
        return QStringLiteral("Unknown");
    }
}

QString routeProtocolToString(NL_ROUTE_PROTOCOL protocol)
{
    switch (protocol) {
    case RouteProtocolLocal:
        return QStringLiteral("Local");
    case RouteProtocolNetMgmt:
        return QStringLiteral("NetMgmt");
    case RouteProtocolIcmp:
        return QStringLiteral("ICMP");
    case RouteProtocolDhcp:
        return QStringLiteral("DHCP");
    case RouteProtocolOther:
        return QStringLiteral("Other");
    default:
        return QStringLiteral("Protocol(%1)").arg(static_cast<int>(protocol));
    }
}

QString sockaddrToString(const SOCKADDR *sockaddr)
{
    if (!sockaddr || sockaddr->sa_family != AF_INET) {
        return {};
    }

    wchar_t buffer[INET_ADDRSTRLEN] = {};
    const sockaddr_in *ipv4 = reinterpret_cast<const sockaddr_in *>(sockaddr);
    if (!InetNtopW(AF_INET, const_cast<IN_ADDR *>(&ipv4->sin_addr), buffer, INET_ADDRSTRLEN)) {
        return {};
    }

    return QString::fromWCharArray(buffer);
}

QString sockaddrInetToString(const SOCKADDR_INET &sockaddr)
{
    if (sockaddr.si_family != AF_INET) {
        return {};
    }

    wchar_t buffer[INET_ADDRSTRLEN] = {};
    if (!InetNtopW(AF_INET, const_cast<IN_ADDR *>(&sockaddr.Ipv4.sin_addr), buffer, INET_ADDRSTRLEN)) {
        return {};
    }

    return QString::fromWCharArray(buffer);
}

SOCKADDR_INET ipv4Sockaddr(const QString &ipv4)
{
    SOCKADDR_INET sockaddr = {};
    sockaddr.si_family = AF_INET;
    sockaddr.Ipv4.sin_family = AF_INET;

    IN_ADDR address = {};
    if (InetPtonW(AF_INET, reinterpret_cast<LPCWSTR>(ipv4.utf16()), &address) == 1) {
        sockaddr.Ipv4.sin_addr = address;
    }

    return sockaddr;
}

bool parseDestinationPrefix(const QString &cidr, IP_ADDRESS_PREFIX *destinationPrefix)
{
    if (!destinationPrefix) {
        return false;
    }

    const QStringList parts = cidr.trimmed().split(QLatin1Char('/'));
    if (parts.size() != 2) {
        return false;
    }

    bool prefixOk = false;
    const int prefixLength = parts.at(1).toInt(&prefixOk);
    if (!prefixOk || prefixLength < 0 || prefixLength > 32) {
        return false;
    }

    const SOCKADDR_INET prefixAddress = ipv4Sockaddr(parts.at(0).trimmed());
    if (prefixAddress.si_family != AF_INET) {
        return false;
    }

    *destinationPrefix = {};
    destinationPrefix->Prefix = prefixAddress;
    destinationPrefix->PrefixLength = static_cast<UINT8>(prefixLength);
    return true;
}

QString destinationPrefixToString(const IP_ADDRESS_PREFIX &prefix)
{
    return QStringLiteral("%1/%2")
        .arg(sockaddrInetToString(prefix.Prefix))
        .arg(prefix.PrefixLength);
}

QString interfaceAliasFromLuid(const NET_LUID &luid)
{
    wchar_t alias[IF_MAX_STRING_SIZE + 1] = {};
    if (ConvertInterfaceLuidToAlias(&luid, alias, IF_MAX_STRING_SIZE + 1) != NO_ERROR) {
        return {};
    }

    return QString::fromWCharArray(alias);
}

bool shouldIncludeAdapter(const IP_ADAPTER_ADDRESSES *entry)
{
    if (!entry) {
        return false;
    }

    return entry->IfType != IF_TYPE_SOFTWARE_LOOPBACK;
}

bool shouldKeepRouteWhenClearing(const MIB_IPFORWARD_ROW2 &row)
{
    if (row.Loopback) {
        return true;
    }

    const QString prefix = destinationPrefixToString(row.DestinationPrefix);
    return prefix == QStringLiteral("127.0.0.0/8")
        || prefix == QStringLiteral("127.0.0.1/32")
        || prefix == QStringLiteral("224.0.0.0/4")
        || prefix == QStringLiteral("255.255.255.255/32");
}

#endif

const char kHostsPath[] = "C:\\Windows\\System32\\drivers\\etc\\hosts";

DECL_LOGGER(networkLogger, "Network");
DECL_LOGGER(routeLogger, "Route");
DECL_LOGGER(explorerLogger, "Explorer");

} // namespace

NetworkService::NetworkService(ProcessService *processService, QObject *parent)
    : QObject(parent)
    , m_processService(processService)
{
}

QList<AdapterInfo> NetworkService::queryAdapters() const
{
    QList<AdapterInfo> adapters;

#ifdef Q_OS_WIN
    ULONG bufferSize = 15 * 1024;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
    DWORD errorCode = ERROR_BUFFER_OVERFLOW;
    QByteArray buffer;

    for (int attempt = 0; attempt < 3 && errorCode == ERROR_BUFFER_OVERFLOW; ++attempt) {
        buffer.resize(static_cast<int>(bufferSize));
        errorCode = GetAdaptersAddresses(
            AF_INET,
            flags,
            nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()),
            &bufferSize);
    }

    if (errorCode != NO_ERROR) {
        networkLogger.error(errorMessage(QStringLiteral("读取网卡信息"), errorCode));
        return adapters;
    }

    for (IP_ADAPTER_ADDRESSES *entry = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());
         entry;
         entry = entry->Next) {
        if (!shouldIncludeAdapter(entry)) {
            continue;
        }

        AdapterInfo adapter;
        adapter.id = normalizeAdapterId(QString::fromLocal8Bit(entry->AdapterName));
        adapter.interfaceIndex = static_cast<int>(entry->IfIndex);
        adapter.name = fromWide(entry->FriendlyName);
        adapter.description = fromWide(entry->Description);
        adapter.status = operStatusToString(entry->OperStatus);

        for (IP_ADAPTER_UNICAST_ADDRESS *unicast = entry->FirstUnicastAddress; unicast; unicast = unicast->Next) {
            const QString ipv4 = sockaddrToString(unicast->Address.lpSockaddr);
            if (ipv4.isEmpty() || ipv4.startsWith(QStringLiteral("169.254."))) {
                continue;
            }

            adapter.ipv4 = ipv4;
            adapter.prefixLength = static_cast<int>(unicast->OnLinkPrefixLength);
            break;
        }

        for (IP_ADAPTER_GATEWAY_ADDRESS_LH *gateway = entry->FirstGatewayAddress; gateway; gateway = gateway->Next) {
            const QString nextHop = sockaddrToString(gateway->Address.lpSockaddr);
            if (!nextHop.isEmpty()) {
                adapter.defaultGateway = nextHop;
                break;
            }
        }

        adapter.subnetMask = prefixLengthToSubnetMask(adapter.prefixLength);
        adapters.append(adapter);
    }

    std::sort(adapters.begin(), adapters.end(), [](const AdapterInfo &lhs, const AdapterInfo &rhs) {
        return lhs.name.localeAwareCompare(rhs.name) < 0;
    });

    networkLogger.info(QStringLiteral("已通过 WinAPI 加载 %1 个网卡。").arg(adapters.count()));
#else
    networkLogger.error(QStringLiteral("当前平台不支持 WinAPI 网卡枚举。"));
#endif

    return adapters;
}

QList<RouteEntry> NetworkService::queryRoutes() const
{
    QList<RouteEntry> routes;

#ifdef Q_OS_WIN
    PMIB_IPFORWARD_TABLE2 table = nullptr;
    const DWORD errorCode = GetIpForwardTable2(AF_INET, &table);
    if (errorCode != NO_ERROR) {
        routeLogger.error(errorMessage(QStringLiteral("读取路由表"), errorCode));
        return routes;
    }

    for (ULONG index = 0; index < table->NumEntries; ++index) {
        const MIB_IPFORWARD_ROW2 &row = table->Table[index];
        RouteEntry route;
        route.destinationPrefix = destinationPrefixToString(row.DestinationPrefix);
        route.nextHop = sockaddrInetToString(row.NextHop);
        route.interfaceAlias = interfaceAliasFromLuid(row.InterfaceLuid);
        route.routeMetric = static_cast<int>(row.Metric);
        route.protocol = routeProtocolToString(row.Protocol);
        route.state = row.Loopback ? QStringLiteral("Loopback") : QStringLiteral("Active");
        route.policyStore = QStringLiteral("ActiveStore");
        routes.append(route);
    }

    FreeMibTable(table);

    std::sort(routes.begin(), routes.end(), [](const RouteEntry &lhs, const RouteEntry &rhs) {
        if (lhs.destinationPrefix == rhs.destinationPrefix) {
            return lhs.routeMetric < rhs.routeMetric;
        }

        return lhs.destinationPrefix < rhs.destinationPrefix;
    });

    routeLogger.info(QStringLiteral("已通过 WinAPI 加载 %1 条 IPv4 路由。").arg(routes.count()));
#else
    routeLogger.error(QStringLiteral("当前平台不支持 WinAPI 路由查询。"));
#endif

    return routes;
}

AdapterInfo NetworkService::findAdapterById(const QString &adapterId) const
{
    const QString targetId = normalizeAdapterId(adapterId);
    const QList<AdapterInfo> adapters = queryAdapters();
    for (const AdapterInfo &adapter : adapters) {
        if (normalizeAdapterId(adapter.id) == targetId) {
            return adapter;
        }
    }

    return {};
}

bool NetworkService::enableAdapter(const QString &adapterId)
{
    return setAdapterEnabled(adapterId, true);
}

bool NetworkService::disableAdapter(const QString &adapterId)
{
    return setAdapterEnabled(adapterId, false);
}

bool NetworkService::removeRoutes(const QStringList &cidrs)
{
#ifdef Q_OS_WIN
    QSet<QString> targets;
    for (const QString &cidr : cidrs) {
        const QString trimmed = cidr.trimmed();
        if (!trimmed.isEmpty()) {
            targets.insert(trimmed);
        }
    }

    if (targets.isEmpty()) {
        return true;
    }

    PMIB_IPFORWARD_TABLE2 table = nullptr;
    const DWORD errorCode = GetIpForwardTable2(AF_INET, &table);
    if (errorCode != NO_ERROR) {
        routeLogger.error(errorMessage(QStringLiteral("读取路由表以删除目标路由"), errorCode));
        return false;
    }

    bool overallSuccess = true;
    for (ULONG index = 0; index < table->NumEntries; ++index) {
        const MIB_IPFORWARD_ROW2 &row = table->Table[index];
        if (!targets.contains(destinationPrefixToString(row.DestinationPrefix))) {
            continue;
        }

        const DWORD deleteError = DeleteIpForwardEntry2(&row);
        if (deleteError != NO_ERROR) {
            overallSuccess = false;
            routeLogger.error(errorMessage(QStringLiteral("删除路由 %1").arg(destinationPrefixToString(row.DestinationPrefix)), deleteError));
        }
    }

    FreeMibTable(table);
    return overallSuccess;
#else
    Q_UNUSED(cidrs)
    routeLogger.error(QStringLiteral("当前平台不支持 WinAPI 路由删除。"));
    return false;
#endif
}

bool NetworkService::addRoutes(const QStringList &cidrs, const AdapterInfo &adapter)
{
    if (adapter.id.trimmed().isEmpty() || adapter.interfaceIndex < 0) {
        routeLogger.error(QStringLiteral("本地局域网网卡信息不完整，无法添加路由。"));
        return false;
    }

    bool overallSuccess = true;

#ifdef Q_OS_WIN
    const QString nextHop = adapter.defaultGateway.trimmed().isEmpty()
        ? QStringLiteral("0.0.0.0")
        : adapter.defaultGateway.trimmed();

    for (const QString &cidr : cidrs) {
        const QString trimmed = cidr.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        MIB_IPFORWARD_ROW2 row;
        InitializeIpForwardEntry(&row);
        if (!parseDestinationPrefix(trimmed, &row.DestinationPrefix)) {
            overallSuccess = false;
            routeLogger.error(QStringLiteral("无效的 CIDR：%1").arg(trimmed));
            continue;
        }

        row.InterfaceIndex = static_cast<NET_IFINDEX>(adapter.interfaceIndex);
        row.NextHop = ipv4Sockaddr(nextHop);
        row.SitePrefixLength = row.DestinationPrefix.PrefixLength;
        row.ValidLifetime = 0xFFFFFFFFu;
        row.PreferredLifetime = 0xFFFFFFFFu;
        row.Metric = 25;
        row.Protocol = RouteProtocolNetMgmt;

        const DWORD createError = CreateIpForwardEntry2(&row);
        if (createError != NO_ERROR) {
            overallSuccess = false;
            routeLogger.error(errorMessage(QStringLiteral("添加路由 %1").arg(trimmed), createError));
        }
    }
#else
    Q_UNUSED(cidrs)
    overallSuccess = false;
#endif

    return overallSuccess;
}

bool NetworkService::clearAllRoutes()
{
#ifdef Q_OS_WIN
    PMIB_IPFORWARD_TABLE2 table = nullptr;
    const DWORD errorCode = GetIpForwardTable2(AF_INET, &table);
    if (errorCode != NO_ERROR) {
        routeLogger.error(errorMessage(QStringLiteral("读取路由表以执行清空"), errorCode));
        return false;
    }

    bool overallSuccess = true;
    for (ULONG index = 0; index < table->NumEntries; ++index) {
        const MIB_IPFORWARD_ROW2 &row = table->Table[index];
        if (shouldKeepRouteWhenClearing(row)) {
            continue;
        }

        const DWORD deleteError = DeleteIpForwardEntry2(&row);
        if (deleteError != NO_ERROR) {
            overallSuccess = false;
            routeLogger.error(errorMessage(QStringLiteral("清理路由 %1").arg(destinationPrefixToString(row.DestinationPrefix)), deleteError));
        }
    }

    FreeMibTable(table);

    if (overallSuccess) {
        routeLogger.warn(QStringLiteral("已通过 WinAPI 清理 IPv4 路由表。"));
    }

    return overallSuccess;
#else
    routeLogger.error(QStringLiteral("当前平台不支持 WinAPI 路由清理。"));
    return false;
#endif
}

bool NetworkService::openHostsInExplorer()
{
    const CommandResult result = m_processService->runProcess(
        QStringLiteral("explorer.exe"),
        { QStringLiteral("/select,%1").arg(QString::fromLatin1(kHostsPath)) },
        QStringLiteral("Explorer"),
        10000);

    if (result.success) {
        explorerLogger.info(QStringLiteral("已定位 Hosts 文件。"));
    }

    return result.success;
}

QString NetworkService::prefixLengthToSubnetMask(int prefixLength) const
{
    if (prefixLength <= 0 || prefixLength > 32) {
        return {};
    }

    quint32 maskValue = prefixLength == 32 ? 0xFFFFFFFFu : (0xFFFFFFFFu << (32 - prefixLength));
    return QHostAddress(maskValue).toString();
}

bool NetworkService::setAdapterEnabled(const QString &adapterId, bool enabled)
{
    if (adapterId.trimmed().isEmpty()) {
        networkLogger.error(QStringLiteral("未配置本地局域网网卡。"));
        return false;
    }

#ifdef Q_OS_WIN
    const AdapterInfo adapter = findAdapterById(adapterId);
    if (adapter.interfaceIndex < 0) {
        networkLogger.error(QStringLiteral("未找到目标网卡：%1").arg(adapterId));
        return false;
    }

    MIB_IFROW row = {};
    row.dwIndex = static_cast<DWORD>(adapter.interfaceIndex);

    const DWORD getError = GetIfEntry(&row);
    if (getError != NO_ERROR) {
        networkLogger.error(errorMessage(QStringLiteral("读取网卡状态"), getError));
        return false;
    }

    row.dwAdminStatus = enabled ? MIB_IF_ADMIN_STATUS_UP : MIB_IF_ADMIN_STATUS_DOWN;
    const DWORD setError = SetIfEntry(&row);
    if (setError != NO_ERROR) {
        networkLogger.error(errorMessage(enabled ? QStringLiteral("启用网卡") : QStringLiteral("禁用网卡"), setError));
        return false;
    }

    networkLogger.info(QStringLiteral("已通过 WinAPI%1网卡：%2").arg(enabled ? QStringLiteral("启用") : QStringLiteral("禁用"), adapter.name));
    return true;
#else
    Q_UNUSED(enabled)
    networkLogger.error(QStringLiteral("当前平台不支持 WinAPI 网卡控制。"));
    return false;
#endif
}
