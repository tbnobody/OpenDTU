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
    String getFilenameByLocale(String& locale) const;

private:
    void readLangPacks();
    void readConfig(String file);

    std::list<LanguageInfo_t> _availLanguages;
};

extern I18nClass I18n;
