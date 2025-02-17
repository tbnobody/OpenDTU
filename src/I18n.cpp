// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Thomas Basler and others
 */
#include "I18n.h"
#include "MessageOutput.h"
#include "Utils.h"
#include "defaults.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

I18nClass I18n;

I18nClass::I18nClass()
{
}

void I18nClass::init(Scheduler& scheduler)
{
    readLangPacks();
}

std::list<LanguageInfo_t> I18nClass::getAvailableLanguages()
{
    return _availLanguages;
}

String I18nClass::getFilenameByLocale(const String& locale) const
{
    auto it = std::find_if(_availLanguages.begin(), _availLanguages.end(), [locale](const LanguageInfo_t& elem) {
        return elem.code == locale;
    });

    if (it != _availLanguages.end()) {
        return it->filename;
    } else {
        return String();
    }
}

void I18nClass::readDisplayStrings(
    const String& locale,
    String& date_format,
    String& offline,
    String& power_w, String& power_kw,
    String& yield_today_wh, String& yield_today_kwh,
    String& yield_total_kwh, String& yield_total_mwh)
{
    auto filename = getFilenameByLocale(locale);
    if (filename == "") {
        return;
    }

    JsonDocument filter;
    filter["display"] = true;

    File f = LittleFS.open(filename, "r", false);

    JsonDocument doc;

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f, DeserializationOption::Filter(filter));
    if (error) {
        MessageOutput.printf("Failed to read file %s\r\n", filename.c_str());
        f.close();
        return;
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return;
    }

    auto displayData = doc["display"];

    if (displayData["date_format"].as<String>() != "null") {
        date_format = displayData["date_format"].as<String>();
    }

    if (displayData["offline"].as<String>() != "null") {
        offline = displayData["offline"].as<String>();
    }

    if (displayData["power_w"].as<String>() != "null") {
        power_w = displayData["power_w"].as<String>();
    }

    if (displayData["power_kw"].as<String>() != "null") {
        power_kw = displayData["power_kw"].as<String>();
    }

    if (displayData["yield_today_wh"].as<String>() != "null") {
        yield_today_wh = displayData["yield_today_wh"].as<String>();
    }

    if (displayData["yield_today_kwh"].as<String>() != "null") {
        yield_today_kwh = displayData["yield_today_kwh"].as<String>();
    }

    if (displayData["yield_total_kwh"].as<String>() != "null") {
        yield_total_kwh = displayData["yield_total_kwh"].as<String>();
    }

    if (displayData["yield_total_mwh"].as<String>() != "null") {
        yield_total_mwh = displayData["yield_total_mwh"].as<String>();
    }

    f.close();
}

void I18nClass::readLangPacks()
{
    auto root = LittleFS.open("/");
    auto file = root.getNextFileName();

    while (file != "") {
        if (file.endsWith(LANG_PACK_SUFFIX)) {
            MessageOutput.printf("Read File %s\r\n", file.c_str());
            readConfig(file);
        }
        file = root.getNextFileName();
    }
    root.close();
}

void I18nClass::readConfig(String file)
{
    JsonDocument filter;
    filter["meta"] = true;

    File f = LittleFS.open(file, "r", false);

    JsonDocument doc;

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f, DeserializationOption::Filter(filter));
    if (error) {
        MessageOutput.printf("Failed to read file %s\r\n", file.c_str());
        f.close();
        return;
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return;
    }

    LanguageInfo_t lang;
    lang.code = String(doc["meta"]["code"] | "");
    lang.name = String(doc["meta"]["name"] | "");
    lang.filename = file;

    if (lang.code != "" && lang.name != "") {
        _availLanguages.push_back(lang);
    } else {
        MessageOutput.printf("Invalid meta data\r\n");
    }

    f.close();
}
