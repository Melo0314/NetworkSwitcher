#pragma once

#include "AppSettings.h"

class SettingsRepository
{
public:
    SettingsRepository() = default;

    AppSettings load() const;
    void save(const AppSettings &settings) const;
};
