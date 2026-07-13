#include <assert.h>
#include <stdio.h>
#include <string>

#include "../src/localization.h"

int main() {
    Localizer localizer;
    assert(localizer.CurrentLanguage() == Language::English);
    assert(std::string(localizer.LanguageName()) == "English");
    assert(std::string(localizer.Get(TextId::AppTitle)) == "KYLIN EXPLORER");

    for (int index = 0; index < static_cast<int>(TextId::Count); ++index) {
        const char *english = localizer.Get(static_cast<TextId>(index));
        assert(english != NULL);
        assert(english[0] != '\0');
    }

    localizer.ToggleLanguage();
    assert(localizer.CurrentLanguage() == Language::SimplifiedChinese);
    assert(std::string(localizer.LanguageName()) == "简体中文");
    assert(std::string(localizer.Get(TextId::AppTitle)) == "麒麟浏览器");

    for (int index = 0; index < static_cast<int>(TextId::Count); ++index) {
        const char *chinese = localizer.Get(static_cast<TextId>(index));
        assert(chinese != NULL);
        assert(chinese[0] != '\0');
    }

    localizer.SetLanguage(Language::English);
    assert(localizer.CurrentLanguage() == Language::English);

    printf("localization-host-tests-ok\n");
    return 0;
}
