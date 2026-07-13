#ifndef KYLIN_EXPLORER_UI_CONTEXT_H
#define KYLIN_EXPLORER_UI_CONTEXT_H

#include <proto-include.h>
#include "graphics.h"

class Scene2D;
class TextRenderer;
class Localizer;
class ExplorerUi;

enum class Theme { Night };

struct ThemeColors {
    Color background, panel, panelDark, panelLight, selection, accent;
    Color folder, file, textFile, imageFile, packageFile, archiveFile;
    Color executableFile, binaryFile, green, danger, text, muted, black;
};

// Night theme — PS5-inspired dark palette
const ThemeColors kNightTheme = {
    { 18, 20, 28 },    { 17, 17, 27 },    { 18, 20, 28 },    { 36, 40, 55 },
    { 68, 72, 91 },    { 130, 168, 212 }, { 247, 207, 98 },  { 112, 201, 255 },
    { 112, 210, 160 }, { 195, 138, 250 }, { 255, 178, 92 },  { 255, 218, 117 },
    { 255, 112, 115 }, { 155, 170, 195 }, { 72, 200, 130 },  { 240, 90, 105 },
    { 209, 211, 233 }, { 130, 142, 168 }, { 0, 0, 0 },
};

inline const ThemeColors& GetThemeColors(Theme) {
    return kNightTheme;
}

struct UiContext {
    Scene2D *scene;
    TextRenderer &bodyText;
    TextRenderer &smallText;
    Localizer *localizer;
    ExplorerUi *ui;

    // Active theme colors — set via ApplyTheme()
    Color background, panel, panelDark, panelLight, selection, accent;
    Color folder, file, textFile, imageFile, packageFile, archiveFile;
    Color executableFile, binaryFile, green, danger, text, muted, black;

    void ApplyTheme(Theme t) {
        const ThemeColors &tc = GetThemeColors(t);
        background = tc.background;   panel = tc.panel;
        panelDark = tc.panelDark;     panelLight = tc.panelLight;
        selection = tc.selection;     accent = tc.accent;
        folder = tc.folder;           file = tc.file;
        textFile = tc.textFile;       imageFile = tc.imageFile;
        packageFile = tc.packageFile; archiveFile = tc.archiveFile;
        executableFile = tc.executableFile; binaryFile = tc.binaryFile;
        green = tc.green;             danger = tc.danger;
        text = tc.text;               muted = tc.muted;
        black = tc.black;
    }
};

#endif
