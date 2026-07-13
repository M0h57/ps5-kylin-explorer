#include "utf8.h"

bool NextUtf8Codepoint(const std::string &text, size_t *offset, uint32_t *codepoint) {
    if (*offset >= text.size()) return false;

    const unsigned char first = static_cast<unsigned char>(text[*offset]);
    size_t length = 1;
    uint32_t value = first;
    if ((first & 0xE0) == 0xC0) { length = 2; value = first & 0x1F; }
    else if ((first & 0xF0) == 0xE0) { length = 3; value = first & 0x0F; }
    else if ((first & 0xF8) == 0xF0) { length = 4; value = first & 0x07; }
    else if ((first & 0x80) != 0) { *codepoint = 0xFFFD; ++*offset; return true; }

    if (*offset + length > text.size()) {
        *codepoint = 0xFFFD;
        *offset = text.size();
        return true;
    }
    for (size_t index = 1; index < length; ++index) {
        const unsigned char next = static_cast<unsigned char>(text[*offset + index]);
        if ((next & 0xC0) != 0x80) {
            *codepoint = 0xFFFD;
            ++*offset;
            return true;
        }
        value = (value << 6) | (next & 0x3F);
    }
    *offset += length;
    *codepoint = value;
    return true;
}

std::string Utf8FromWide(const wchar_t *text) {
    std::string output;
    for (size_t index = 0; text[index] != 0; ++index) {
        const uint32_t value = static_cast<uint32_t>(text[index]);
        if (value <= 0x7F) output += static_cast<char>(value);
        else if (value <= 0x7FF) {
            output += static_cast<char>(0xC0 | (value >> 6));
            output += static_cast<char>(0x80 | (value & 0x3F));
        } else if (value <= 0xFFFF) {
            output += static_cast<char>(0xE0 | (value >> 12));
            output += static_cast<char>(0x80 | ((value >> 6) & 0x3F));
            output += static_cast<char>(0x80 | (value & 0x3F));
        } else {
            output += static_cast<char>(0xF0 | (value >> 18));
            output += static_cast<char>(0x80 | ((value >> 12) & 0x3F));
            output += static_cast<char>(0x80 | ((value >> 6) & 0x3F));
            output += static_cast<char>(0x80 | (value & 0x3F));
        }
    }
    return output;
}

void WideFromUtf8(const std::string &text, wchar_t *output, size_t outputCount) {
    size_t inputOffset = 0;
    size_t outputOffset = 0;
    uint32_t codepoint = 0;
    while (outputOffset + 1 < outputCount && NextUtf8Codepoint(text, &inputOffset, &codepoint)) {
        output[outputOffset++] = static_cast<wchar_t>(codepoint);
    }
    output[outputOffset] = 0;
}
