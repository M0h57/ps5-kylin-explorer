#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string>

#include "../src/utf8.h"

int main() {
    const std::string text = "A中文";
    size_t offset = 0;
    uint32_t codepoint = 0;

    assert(NextUtf8Codepoint(text, &offset, &codepoint));
    assert(codepoint == 'A');
    assert(offset == 1);

    assert(NextUtf8Codepoint(text, &offset, &codepoint));
    assert(codepoint == 0x4E2D);

    assert(NextUtf8Codepoint(text, &offset, &codepoint));
    assert(codepoint == 0x6587);
    assert(!NextUtf8Codepoint(text, &offset, &codepoint));

    std::string invalid;
    invalid.push_back(static_cast<char>(0x80));
    offset = 0;
    assert(NextUtf8Codepoint(invalid, &offset, &codepoint));
    assert(codepoint == 0xFFFD);
    assert(offset == 1);

    invalid.clear();
    invalid.push_back(static_cast<char>(0xE4));
    invalid.push_back('A');
    offset = 0;
    assert(NextUtf8Codepoint(invalid, &offset, &codepoint));
    assert(codepoint == 0xFFFD);
    assert(offset == invalid.size());

    wchar_t wide[16];
    WideFromUtf8("PS5中文", wide, sizeof(wide) / sizeof(wide[0]));
    assert(wide[0] == L'P');
    assert(wide[3] == static_cast<wchar_t>(0x4E2D));
    assert(Utf8FromWide(wide) == "PS5中文");

    wchar_t tiny[3];
    WideFromUtf8("abcd", tiny, sizeof(tiny) / sizeof(tiny[0]));
    assert(tiny[0] == L'a');
    assert(tiny[1] == L'b');
    assert(tiny[2] == 0);

    printf("utf8-host-tests-ok\n");
    return 0;
}
