#ifndef KYLIN_EXPLORER_TEXT_RENDERER_H
#define KYLIN_EXPLORER_TEXT_RENDERER_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <proto-include.h>

class Scene2D;
struct Color;

class TextRenderer {
public:
    TextRenderer(Scene2D *scene, FT_Face latinFace, FT_Face fallbackFace);
    void Draw(const std::string &text, int x, int baselineY, Color color);
    int MeasureWidth(const std::string &text) const;
    std::string Truncate(const std::string &text, size_t maxCodepoints) const;

private:
    bool NextCodepoint(const std::string &text, size_t *offset, uint32_t *codepoint) const;
    FT_Face FaceFor(uint32_t codepoint) const;

    Scene2D *scene_;
    FT_Face latinFace_;
    FT_Face fallbackFace_;
};

#endif
