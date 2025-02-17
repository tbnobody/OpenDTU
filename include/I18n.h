// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <WString.h>
#include <list>

struct LanguageInfo_t {
    String code;
    String name;
    String filename;
};

class I18nClass {
public:
    I18nClass();
    void init(Scheduler& scheduler);
    std::list<LanguageInfo_t> getAvailableLanguages();
    String getFilenameByLocale(const String& locale) const;
    void readDisplayStrings(
        const String& locale,
        String& date_format,
        String& offline,
        String& power_w, String& power_kw,
        String& yield_today_wh, String& yield_today_kwh,
        String& yield_total_kwh, String& yield_total_mwh);

private:
    void readLangPacks();
    void readConfig(String file);

    std::list<LanguageInfo_t> _availLanguages;
};

extern I18nClass I18n;
