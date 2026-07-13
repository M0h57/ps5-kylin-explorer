#ifndef KYLIN_EXPLORER_UTF8_H
#define KYLIN_EXPLORER_UTF8_H

#include <stddef.h>
#include <stdint.h>
#include <string>

bool NextUtf8Codepoint(const std::string &text, size_t *offset, uint32_t *codepoint);
std::string Utf8FromWide(const wchar_t *text);
void WideFromUtf8(const std::string &text, wchar_t *output, size_t outputCount);

#endif
