// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Thomas Basler and others
 */
#include "Logging.h"
#include "Configuration.h"

LoggingClass Logging;

#undef TAG
static const char* TAG = "logging";

LoggingClass::LoggingClass()
{
    _configurableModules.reserve(5);
    _configurableModules.push_back("CORE");
    _configurableModules.push_back("hoymiles");
    _configurableModules.push_back("mqtt");
    _configurableModules.push_back("network");
    _configurableModules.push_back("webapi");
}

const std::vector<String>& LoggingClass::getConfigurableModules() const
{
    return _configurableModules;
}

void LoggingClass::applyLogLevels()
{
    const auto& config = Configuration.get().Logging;

    ESP_LOGD(TAG, "Set default log level: %" PRId8, config.Default);
    esp_log_level_set("*", static_cast<esp_log_level_t>(config.Default));

    for (int8_t i = 0; i < LOG_MODULE_COUNT; i++) {
        bool isValidModule = std::find(_configurableModules.begin(), _configurableModules.end(), config.Modules[i].Name) != _configurableModules.end();
        if (!isValidModule
            || strlen(config.Modules[i].Name) == 0
            || config.Modules[i].Level < ESP_LOG_NONE
            || config.Modules[i].Level > ESP_LOG_VERBOSE) {
            continue;
        }

        ESP_LOGD(TAG, "Set log level for %s: %" PRId8, config.Modules[i].Name, config.Modules[i].Level);
        esp_log_level_set(config.Modules[i].Name, static_cast<esp_log_level_t>(config.Modules[i].Level));
    }
}
