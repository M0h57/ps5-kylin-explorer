#ifndef KYLIN_EXPLORER_UI_H
#define KYLIN_EXPLORER_UI_H

#include <proto-include.h>
#include "context_menu.h"
#include "file_browser.h"
#include "file_operations.h"
#include "localization.h"
#include "operation_log.h"
#include "text_renderer.h"
#include "sfo_parser.h"
#include <vector>
#include <string>

#include "ui_context.h"
#include "logo_view.h"
#include "glimpse_view.h"
#include "settings_view.h"
#include "installed_pkgs_view.h"
#include "modal_view.h"
#include "toast.h"

class Scene2D;

enum class UiIcon {
    Folder,
    File,
    TextFile,
    ImageFile,
    PackageFile,
    ArchiveFile,
    ExecutableFile,
    BinaryFile,
    UnknownFile,
    Refresh,
    Sort,
    Language,
    Properties,
    NewFolder,
    Rename,
    Copy,
    Cut,
    Paste,
    SendTo,
    Delete,
    OperationLog,
    Loading,
    Success,
    Failed,
    ButtonCross,
    ButtonCircle,
    ButtonSquare,
    ButtonTriangle,
    ButtonOptions,
    ButtonL1,
    ButtonL2,
    ButtonR1,
    ButtonR2,
    ButtonTouchpad,
    ButtonUpDown,
    ButtonLeftRight,
    ButtonL3,
    ButtonR3,
    ButtonRS,
    MenuRefresh,
    MenuSort,
    MenuProperties,
    MenuNewFolder,
    MenuRename,
    MenuCopy,
    MenuCut,
    MenuForward,
    MenuDelete,
    SettingsLanguage,
    SettingsTemperature,
    SettingsInstalledApps,
    SettingsOperationLog,
    GlimpseUsb0,
    GlimpseUsb1,
    GlimpseUsb2,
    GlimpseUsb3,
    GlimpseUsb4,
    GlimpseUsb5,
    GlimpseUsb6,
    GlimpseExt1,
    Count,
};

struct AppConfig {
    bool tempFahrenheit;
    Theme theme;
    AppConfig() : tempFahrenheit(false), theme(Theme::Night) {}
};

class ExplorerUi {
public:
    ExplorerUi(Scene2D *scene, FT_Face bodyLatin, FT_Face bodyFallback,
               FT_Face smallLatin, FT_Face smallFallback, Localizer *localizer);
    void DrawLoading(int attempts);
    void Draw(const FileBrowser &browser, const ContextMenu &menu, const FileOperations &operations,
              bool propertiesOpen, bool errorOpen,
              bool overwriteConfirmOpen, bool deleteConfirmOpen, bool recursiveDeleteConfirmOpen,
              bool operationLogOpen, const OperationLog &operationLog,
              bool installedPkgsOpen, int installedPkgsIndex, const std::vector<InstalledApp> &installedApps,
              bool settingsOpen, int settingsIndex, const AppConfig &config,
              bool quickPlacesOpen, int quickPlacesIndex, const std::vector<BrowserLocation> &quickPlaces,
              bool pkgInstallerOpen, const std::string &pkgInstallerPath, int pkgInstallerProgress, const std::string &pkgInstallerStatus, bool pkgInstallerFinished,
              const ToastManager &toastManager,
              bool l2Held);

    void SetTheme(Theme t);
    void SetErrorText(const std::string &text, const std::string &title = "") { errorText_ = text; errorTitle_ = title; }

    // Public drawing and conversion helpers for component access
    void Text(TextRenderer &renderer, const std::string &text, int x, int y, struct Color color);
    void DrawIcon(UiIcon icon, int x, int y, int size, struct Color color);
    void DrawIconAtBaseline(UiIcon icon, int x, int baselineY, struct Color color);
    bool DrawIconAsset(UiIcon icon, int x, int y, int size, struct Color color);
    bool DrawIconNearest(UiIcon icon, int x, int y, int size);
    int DrawButtonHint(UiIcon icon, const std::string &label, int x, int baselineY, struct Color color);
    void TextRight(TextRenderer &renderer, const std::string &text, int rightX, int y, struct Color color);
    UiIcon MenuIcon(MenuAction action) const;
    UiIcon FileEntryIcon(const FileEntry &entry) const;
    struct Color FileEntryIconColor(const FileEntry &entry) const;
    const char *SortText(SortMode mode) const;
    const char *FileTypeText(FileType type) const;
    const char *MenuText(MenuAction action) const;
    const char *OperationLogText(OperationLogAction action) const;
    std::string StatusText(const FileBrowser &browser) const;

private:
    void DrawMenu(const FileBrowser &browser, const ContextMenu &menu);
    void DrawToasts(const ToastManager &toastManager);

    Scene2D *scene_;
    TextRenderer bodyText_;
    TextRenderer smallText_;
    Localizer *localizer_;

    // Shared context and component instances
    UiContext ctx_;
    LogoView logoView_;
    GlimpseView glimpseView_;
    SettingsView settingsView_;
    InstalledPkgsView installedPkgsView_;
    ModalView modalView_;
    std::string errorText_;
    std::string errorTitle_;
};

#endif
