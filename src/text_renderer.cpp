#include "text_renderer.h"

#include "graphics.h"
#include "logger.h"
#include "utf8.h"

namespace {
bool IsHexDigit(char value) {
    return (value >= '0' && value <= '9') ||
           (value >= 'A' && value <= 'F') ||
           (value >= 'a' && value <= 'f');
}

bool IsSplitEscape(const std::string &text, size_t offset, size_t *escapeStart) {
    for (size_t back = 1; back <= 3 && offset >= back; ++back) {
        const size_t start = offset - back;
        if (start + 3 < text.size() && text[start] == '\\' && text[start + 1] == 'x' &&
            IsHexDigit(text[start + 2]) && IsHexDigit(text[start + 3]) &&
            offset > start && offset < start + 4) {
            *escapeStart = start;
            return true;
        }
    }
    return false;
}
}

TextRenderer::TextRenderer(Scene2D *scene, FT_Face latinFace, FT_Face fallbackFace)
    : scene_(scene), latinFace_(latinFace), fallbackFace_(fallbackFace) {
    LOG_INFO("text ctor latin face: %p", latinFace_);
    LOG_INFO("text ctor fallback face: %p", fallbackFace_);
}

bool TextRenderer::NextCodepoint(const std::string &text, size_t *offset, uint32_t *codepoint) const {
    return NextUtf8Codepoint(text, offset, codepoint);
}

FT_Face TextRenderer::FaceFor(uint32_t codepoint) const {
    if (latinFace_ != NULL && FT_Get_Char_Index(latinFace_, codepoint) != 0) {
        return latinFace_;
    }
    if (fallbackFace_ != NULL && FT_Get_Char_Index(fallbackFace_, codepoint) != 0) {
        return fallbackFace_;
    }
    return latinFace_ != NULL ? latinFace_ : fallbackFace_;
}

void TextRenderer::Draw(const std::string &text, int startX, int baselineY, Color color) {
    static bool loggedFirstDraw = false;

    int x = startX;
    size_t offset = 0;
    uint32_t codepoint = 0;
    while (NextCodepoint(text, &offset, &codepoint)) {
        if (codepoint == '\n') {
            x = startX;
            baselineY += latinFace_ != NULL ? latinFace_->size->metrics.height >> 6 : 32;
            continue;
        }

        FT_Face face = FaceFor(codepoint);
        int advance = scene_->DrawCodepoint(codepoint, face, x, baselineY, color);
        if (advance <= 0 && codepoint == ' ') {
            advance = face != NULL ? face->size->metrics.max_advance >> 6 : 8;
        }
        x += advance;
    }

    if (!loggedFirstDraw) {
        loggedFirstDraw = true;
        LOG_INFO("text first size: %d (0x%08x)", static_cast<int>(text.size()), static_cast<int>(text.size()));
        LOG_INFO("text draw latin face: %p", latinFace_);
        LOG_INFO("text draw fallback face: %p", fallbackFace_);
        LOG_INFO("text utf8 DrawCodepoint path");
    }
}

int TextRenderer::MeasureWidth(const std::string &text) const {
    int width = 0;
    size_t offset = 0;
    uint32_t codepoint = 0;
    while (NextCodepoint(text, &offset, &codepoint)) {
        if (codepoint == '\n') break;

        FT_Face face = FaceFor(codepoint);
        int advance = 0;
        if (face != NULL && FT_Load_Char(face, codepoint, FT_LOAD_DEFAULT) == 0) {
            advance = static_cast<int>(face->glyph->advance.x >> 6);
        }
        if (advance <= 0 && codepoint == ' ') {
            advance = face != NULL ? static_cast<int>(face->size->metrics.max_advance >> 6) : 8;
        }
        width += advance;
    }
    return width;
}

std::string TextRenderer::Truncate(const std::string &text, size_t maxCodepoints) const {
    size_t offset = 0;
    size_t count = 0;
    uint32_t codepoint = 0;
    while (count < maxCodepoints && NextCodepoint(text, &offset, &codepoint)) {
        ++count;
    }
    if (offset >= text.size()) {
        return text;
    }
    size_t escapeStart = 0;
    if (IsSplitEscape(text, offset, &escapeStart)) {
        offset = escapeStart;
    }
    return text.substr(0, offset) + "...";
}
