// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Thomas Basler and others
 */
#include "Logging.h"
#include "Configuration.h"

LoggingClass Logging;

LoggingClass::LoggingClass()
{
    _configurableModules.reserve(3);
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
    const CONFIG_T& config = Configuration.get();

    esp_log_level_set("*", static_cast<esp_log_level_t>(config.Logging.Default));
    for (int8_t i = 0; i < LOG_MODULE_COUNT; i++) {
        if (strlen(config.Logging.Modules[i].Name) == 0 || config.Logging.Modules[i].Level < ESP_LOG_NONE || config.Logging.Modules[i].Level > ESP_LOG_VERBOSE) {
            continue;
        }

        esp_log_level_set(config.Logging.Modules[i].Name, static_cast<esp_log_level_t>(config.Logging.Modules[i].Level));
    }
}
