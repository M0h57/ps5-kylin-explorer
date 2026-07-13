#include "ui.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "graphics.h"

namespace {
struct EmbeddedIcon {
    const unsigned char *start;
    const unsigned char *end;
};

const int kEmbeddedIconSize = 256;
const int kInlineIconSize = 32;
const int kInlineIconBaselineOffset = 24;

std::string EntryDisplayName(const FileEntry &entry) {
    return entry.displayName.empty() ? DisplayCodec::DecodeBytes(entry.name).text : entry.displayName;
}

std::string DisplayBytes(const std::string &bytes) {
    return DisplayCodec::DecodeBytes(bytes).text;
}

#define DECLARE_ICON(symbol) \
    extern "C" const unsigned char symbol##_start[]; \
    extern "C" const unsigned char symbol##_end[]

DECLARE_ICON(kylin_icon_button_cross);
DECLARE_ICON(kylin_icon_button_circle);
DECLARE_ICON(kylin_icon_button_square);
DECLARE_ICON(kylin_icon_button_triangle);
DECLARE_ICON(kylin_icon_button_options);
DECLARE_ICON(kylin_icon_button_l1);
DECLARE_ICON(kylin_icon_button_l2);
DECLARE_ICON(kylin_icon_button_r1);
DECLARE_ICON(kylin_icon_button_r2);
DECLARE_ICON(kylin_icon_button_touchpad);
DECLARE_ICON(kylin_icon_button_up_down);
DECLARE_ICON(kylin_icon_button_left_right);
DECLARE_ICON(kylin_icon_button_l3);
DECLARE_ICON(kylin_icon_button_r3);
DECLARE_ICON(kylin_icon_button_rs);

DECLARE_ICON(kylin_icon_menu_refresh);
DECLARE_ICON(kylin_icon_menu_sort);
DECLARE_ICON(kylin_icon_menu_properties);
DECLARE_ICON(kylin_icon_menu_new_folder);
DECLARE_ICON(kylin_icon_menu_rename);
DECLARE_ICON(kylin_icon_menu_copy);
DECLARE_ICON(kylin_icon_menu_cut);
DECLARE_ICON(kylin_icon_menu_forward);
DECLARE_ICON(kylin_icon_menu_delete);
DECLARE_ICON(kylin_icon_explorer_folder);
DECLARE_ICON(kylin_icon_explorer_file);
DECLARE_ICON(kylin_icon_settings_language);
DECLARE_ICON(kylin_icon_settings_temperature);
DECLARE_ICON(kylin_icon_settings_installed_apps);
DECLARE_ICON(kylin_icon_settings_operation_log);
DECLARE_ICON(kylin_icon_common_in_operation);
DECLARE_ICON(kylin_icon_common_success);
DECLARE_ICON(kylin_icon_common_error);
DECLARE_ICON(kylin_icon_common_package_installing);
DECLARE_ICON(kylin_icon_common_usb_0);
DECLARE_ICON(kylin_icon_common_usb_1);
DECLARE_ICON(kylin_icon_common_usb_2);
DECLARE_ICON(kylin_icon_common_usb_3);
DECLARE_ICON(kylin_icon_common_usb_4);
DECLARE_ICON(kylin_icon_common_usb_5);
DECLARE_ICON(kylin_icon_common_usb_6);
DECLARE_ICON(kylin_icon_common_ext_1);

#undef DECLARE_ICON

#define ICON_ASSET(symbol) { symbol##_start, symbol##_end }

EmbeddedIcon EmbeddedIconFor(UiIcon icon) {
    switch (icon) {
        case UiIcon::Folder: return ICON_ASSET(kylin_icon_explorer_folder);
        case UiIcon::File: return ICON_ASSET(kylin_icon_explorer_file);
        case UiIcon::ButtonCross: return ICON_ASSET(kylin_icon_button_cross);
        case UiIcon::ButtonCircle: return ICON_ASSET(kylin_icon_button_circle);
        case UiIcon::ButtonSquare: return ICON_ASSET(kylin_icon_button_square);
        case UiIcon::ButtonTriangle: return ICON_ASSET(kylin_icon_button_triangle);
        case UiIcon::ButtonOptions: return ICON_ASSET(kylin_icon_button_options);
        case UiIcon::ButtonL1: return ICON_ASSET(kylin_icon_button_l1);
        case UiIcon::ButtonL2: return ICON_ASSET(kylin_icon_button_l2);
        case UiIcon::ButtonR1: return ICON_ASSET(kylin_icon_button_r1);
        case UiIcon::ButtonR2: return ICON_ASSET(kylin_icon_button_r2);
        case UiIcon::ButtonTouchpad: return ICON_ASSET(kylin_icon_button_touchpad);
        case UiIcon::ButtonUpDown: return ICON_ASSET(kylin_icon_button_up_down);
        case UiIcon::ButtonLeftRight: return ICON_ASSET(kylin_icon_button_left_right);
        case UiIcon::ButtonL3: return ICON_ASSET(kylin_icon_button_l3);
        case UiIcon::ButtonR3: return ICON_ASSET(kylin_icon_button_r3);
        case UiIcon::ButtonRS: return ICON_ASSET(kylin_icon_button_rs);
        case UiIcon::MenuRefresh: return ICON_ASSET(kylin_icon_menu_refresh);
        case UiIcon::MenuSort: return ICON_ASSET(kylin_icon_menu_sort);
        case UiIcon::MenuProperties: return ICON_ASSET(kylin_icon_menu_properties);
        case UiIcon::MenuNewFolder: return ICON_ASSET(kylin_icon_menu_new_folder);
        case UiIcon::MenuRename: return ICON_ASSET(kylin_icon_menu_rename);
        case UiIcon::MenuCopy: return ICON_ASSET(kylin_icon_menu_copy);
        case UiIcon::MenuCut: return ICON_ASSET(kylin_icon_menu_cut);
        case UiIcon::MenuForward: return ICON_ASSET(kylin_icon_menu_forward);
        case UiIcon::MenuDelete: return ICON_ASSET(kylin_icon_menu_delete);
        case UiIcon::SettingsLanguage: return ICON_ASSET(kylin_icon_settings_language);
        case UiIcon::SettingsTemperature: return ICON_ASSET(kylin_icon_settings_temperature);
        case UiIcon::SettingsInstalledApps: return ICON_ASSET(kylin_icon_settings_installed_apps);
        case UiIcon::SettingsOperationLog: return ICON_ASSET(kylin_icon_settings_operation_log);
        case UiIcon::Copy: return ICON_ASSET(kylin_icon_common_in_operation);
        case UiIcon::SendTo: return ICON_ASSET(kylin_icon_common_in_operation);
        case UiIcon::Delete: return ICON_ASSET(kylin_icon_common_in_operation);
        case UiIcon::Success: return ICON_ASSET(kylin_icon_common_success);
        case UiIcon::Failed: return ICON_ASSET(kylin_icon_common_error);
        case UiIcon::PackageFile: return ICON_ASSET(kylin_icon_common_package_installing);
        case UiIcon::GlimpseUsb0: return ICON_ASSET(kylin_icon_common_usb_0);
        case UiIcon::GlimpseUsb1: return ICON_ASSET(kylin_icon_common_usb_1);
        case UiIcon::GlimpseUsb2: return ICON_ASSET(kylin_icon_common_usb_2);
        case UiIcon::GlimpseUsb3: return ICON_ASSET(kylin_icon_common_usb_3);
        case UiIcon::GlimpseUsb4: return ICON_ASSET(kylin_icon_common_usb_4);
        case UiIcon::GlimpseUsb5: return ICON_ASSET(kylin_icon_common_usb_5);
        case UiIcon::GlimpseUsb6: return ICON_ASSET(kylin_icon_common_usb_6);
        case UiIcon::GlimpseExt1: return ICON_ASSET(kylin_icon_common_ext_1);
        default: return { NULL, NULL };
    }
}

#undef ICON_ASSET

struct ButtonHint {
    UiIcon icon;
    TextId label;
};


int DrawButtonHints(ExplorerUi *ui, TextRenderer &text, Localizer *localizer,
                    const ButtonHint *hints, int count, int x, int baselineY, Color color) {
    int nextX = x;
    for (int i = 0; i < count; ++i) {
        nextX = ui->DrawButtonHint(hints[i].icon, localizer->Get(hints[i].label), nextX, baselineY, color);
    }
    return nextX;
}
}

ExplorerUi::ExplorerUi(Scene2D *scene, FT_Face bodyLatin, FT_Face bodyFallback,
                       FT_Face smallLatin, FT_Face smallFallback, Localizer *localizer)
    : scene_(scene), bodyText_(scene, bodyLatin, bodyFallback),
      smallText_(scene, smallLatin, smallFallback), localizer_(localizer),
      ctx_({scene, bodyText_, smallText_, localizer, this}) {
    ctx_.ApplyTheme(Theme::Night);
}

void ExplorerUi::SetTheme(Theme t) { ctx_.ApplyTheme(t); }

void ExplorerUi::Text(TextRenderer &renderer, const std::string &text, int x, int y, Color color) {
    renderer.Draw(text, x, y, color);
}

void ExplorerUi::DrawIcon(UiIcon icon, int x, int y, int size, Color color) {
    DrawIconAsset(icon, x, y, size, color);
}

void ExplorerUi::DrawIconAtBaseline(UiIcon icon, int x, int baselineY, Color color) {
    DrawIcon(icon, x, baselineY - kInlineIconBaselineOffset, kInlineIconSize, color);
}

int ExplorerUi::DrawButtonHint(UiIcon icon, const std::string &label, int x, int baselineY, Color color) {
    DrawIconAtBaseline(icon, x, baselineY, color);
    Text(smallText_, label, x + kInlineIconSize + 8, baselineY, color);
    // Increased extra spacing from 18px to 38px to add a beautiful 20px Gap between items
    return x + kInlineIconSize + 38 + static_cast<int>(label.size()) * 12;
}

void ExplorerUi::TextRight(TextRenderer &renderer, const std::string &text, int rightX, int y, Color color) {
    Text(renderer, text, rightX - renderer.MeasureWidth(text), y, color);
}

void ExplorerUi::DrawLoading(int attempts) {
    scene_->FrameBufferFill(ctx_.background);
    const int x = 680;
    const int y = 390;
    scene_->DrawRectangle(x, y, 560, 274, ctx_.panelDark);
    scene_->DrawRectangle(x + 18, y + 72, 524, 2, ctx_.accent);
    Text(bodyText_, localizer_->Get(TextId::LoadingTitle), x + 28, y + 50, ctx_.text);
    Text(smallText_, localizer_->Get(TextId::LoadingMessage), x + 36, y + 104, ctx_.text);
    Text(smallText_, localizer_->Get(TextId::LoadingHint), x + 36, y + 212, ctx_.muted);

    char status[64];
    snprintf(status, sizeof(status), "REQUEST  %d", attempts);
    Text(smallText_, status, x + 36, y + 264, ctx_.muted);
}

void ExplorerUi::Draw(const FileBrowser &browser, const ContextMenu &menu, const FileOperations &operations,
                      bool propertiesOpen, bool errorOpen,
                      bool overwriteConfirmOpen, bool deleteConfirmOpen, bool recursiveDeleteConfirmOpen,
                      bool operationLogOpen, const OperationLog &operationLog,
                      bool installedPkgsOpen, int installedPkgsIndex, const std::vector<InstalledApp> &installedApps,
                      bool settingsOpen, int settingsIndex, const AppConfig &config,
                      bool quickPlacesOpen, int quickPlacesIndex, const std::vector<BrowserLocation> &quickPlaces,
                      bool pkgInstallerOpen, const std::string &pkgInstallerPath, int pkgInstallerProgress, const std::string &pkgInstallerStatus, bool pkgInstallerFinished,
                      const ToastManager &toastManager,
                      bool l2Held) {
    scene_->FrameBufferFill(ctx_.background);
    scene_->DrawRectangle(0, 0, 1920, 104, ctx_.panelDark);
    scene_->DrawRectangle(0, 1000, 1920, 80, ctx_.panelDark);
    scene_->DrawRectangle(40, 124, 1840, 856, ctx_.panel);

    logoView_.Draw(ctx_);
    glimpseView_.Draw(ctx_, config, browser);

    std::string path = std::string(localizer_->Get(TextId::Path)) + "  " + DisplayBytes(browser.CurrentPath());
    Text(smallText_, smallText_.Truncate(path, 84), 60, 172, ctx_.muted);

    const std::vector<FileEntry> &entries = browser.Entries();
    const int visibleRows = 13;
    const int first = browser.SelectedIndex() < 12 ? 0 : browser.SelectedIndex() - 11;
    for (int row = 0; row < visibleRows && first + row < static_cast<int>(entries.size()); ++row) {
        const int index = first + row;
        const int y = 224 + row * 56;
        const bool selected = index == browser.SelectedIndex();
        const bool marked = browser.IsMarked(entries[index]);
        const int kRowWidth = 1808;
        if (selected) {
            scene_->DrawRectangle(56, y - 34, kRowWidth, 48, ctx_.selection);
        } else if (marked) {
            scene_->DrawRectangle(56, y - 34, kRowWidth, 48, ctx_.panelLight);
        }

        DrawIconAtBaseline(FileEntryIcon(entries[index]), 76, y, FileEntryIconColor(entries[index]));
        Text(smallText_, smallText_.Truncate(EntryDisplayName(entries[index]), 74), 116, y, ctx_.text);

        // Detail info right-aligned in the row
        char detail[24];
        if (entries[index].isDirectory) {
            snprintf(detail, sizeof(detail), "--");
        } else {
            FormatSize(detail, sizeof(detail), entries[index].size);
        }
        const int kDetailRight = 1864;
        TextRight(smallText_, smallText_.Truncate(detail, 28), kDetailRight, y,
                  entries[index].isDirectory ? ctx_.muted : (marked ? ctx_.text : ctx_.muted));
    }

    if (settingsOpen) {
        const ButtonHint hints[] = {
            { UiIcon::ButtonUpDown, TextId::HintNavigate },
            { UiIcon::ButtonLeftRight, TextId::HintSwitch },
            { UiIcon::ButtonCross, TextId::HintAction },
            { UiIcon::ButtonCircle, TextId::HintSaveExit },
        };
        DrawButtonHints(this, smallText_, localizer_, hints, 4, 40, 1044, ctx_.text);
    } else if (menu.IsOpen()) {
        const ButtonHint hints[] = {
            { UiIcon::ButtonCross, TextId::HintSelect },
            { UiIcon::ButtonCircle, TextId::HintClose },
            { UiIcon::ButtonUpDown, TextId::HintNavigate },
        };
        DrawButtonHints(this, smallText_, localizer_, hints, 3, 40, 1044, ctx_.text);
    } else {
        const ButtonHint hints[] = {
            { UiIcon::ButtonTouchpad, TextId::HintSettings },
            { UiIcon::ButtonCross, TextId::HintOpen },
            { UiIcon::ButtonCircle, TextId::HintBack },
            { UiIcon::ButtonTriangle, TextId::HintMenu },
            { UiIcon::ButtonOptions, TextId::HintQuickPlaces },
            { UiIcon::ButtonLeftRight, TextId::HintSort },
        };
        int nextX = DrawButtonHints(this, smallText_, localizer_, hints, 6, 40, 1044, ctx_.text);

        std::string l2Label;
        if (l2Held) {
            // When L2 is held, show dynamically combined hint: "L2 + D-pad Navigate: Fast scroll"
            l2Label = "+ D-pad " + std::string(localizer_->Get(TextId::HintNavigate)) + ": " + std::string(localizer_->Get(TextId::HintFastScroll));
        } else {
            l2Label = localizer_->Get(TextId::HintFastScroll);
        }
        DrawButtonHint(UiIcon::ButtonL2, l2Label, nextX, 1044, ctx_.text);
    }

    TextRight(smallText_, smallText_.Truncate(StatusText(browser), 34), 1880, 1044, ctx_.muted);

    if (menu.IsOpen()) {
        DrawMenu(browser, menu);
    }
    if (propertiesOpen) {
        modalView_.DrawProperties(ctx_, browser);
    }
    if (errorOpen) {
        modalView_.DrawError(ctx_, browser, errorText_, errorTitle_);
    }
    if (operations.IsBusy() || operations.HasResult()) {
        modalView_.DrawCopyProgress(ctx_, operations);
    }
    if (overwriteConfirmOpen) {
        modalView_.DrawOverwriteConfirm(ctx_);
    }
    if (deleteConfirmOpen) {
        modalView_.DrawDeleteConfirm(ctx_);
    }
    if (recursiveDeleteConfirmOpen) {
        modalView_.DrawRecursiveDeleteConfirm(ctx_);
    }
    if (operationLogOpen) {
        modalView_.DrawOperationLog(ctx_, operationLog);
    }
    if (installedPkgsOpen) {
        installedPkgsView_.Draw(ctx_, installedApps, installedPkgsIndex);
    }
    if (settingsOpen) {
        settingsView_.Draw(ctx_, settingsIndex, config);
    }
    if (quickPlacesOpen) {
        modalView_.DrawQuickPlaces(ctx_, quickPlaces, quickPlacesIndex);
    }
    if (pkgInstallerOpen) {
        modalView_.DrawPkgProgress(ctx_, pkgInstallerPath, pkgInstallerProgress, pkgInstallerStatus, pkgInstallerFinished);
    }
    DrawToasts(toastManager);
}

void ExplorerUi::DrawMenu(const FileBrowser &browser, const ContextMenu &menu) {
    const int x = 760;
    const int y = 250;
    const int width = 500;
    const int rowHeight = 52;
    scene_->DrawRectangle(x, y, width, 32 + menu.ItemCount() * rowHeight, ctx_.panelDark);
    for (int index = 0; index < menu.ItemCount(); ++index) {
        const int rowY = y + 50 + index * rowHeight;
        const MenuItem &item = menu.Items()[index];
        if (index == menu.SelectedIndex()) {
            scene_->DrawRectangle(x + 12, rowY - 30, width - 24, 44, ctx_.selection);
        }
        const Color iconColor = item.enabled ? ctx_.accent : ctx_.muted;
        DrawIconAtBaseline(MenuIcon(item.action), x + 30, rowY, iconColor);
        std::string label = MenuText(item.action);
        if (item.action == MenuAction::CycleSort) {
            label += std::string("  ") + SortText(browser.CurrentSort());
        }
        Text(smallText_, label, x + 76, rowY, item.enabled ? ctx_.text : ctx_.muted);
    }
}

void ExplorerUi::DrawToasts(const ToastManager &toastManager) {
    const std::vector<ToastNotification> &activeToasts = toastManager.Toasts();
    const int toastWidth = 360;
    const int toastHeight = 80;

    for (const auto &toast : activeToasts) {
        const int x = 1856 - toastWidth + static_cast<int>(toast.slideX);
        const int y = static_cast<int>(toast.currentY);

        const Color cardBg = { 24, 24, 28 };
        scene_->DrawRectangle(x, y, toastWidth, toastHeight, cardBg);

        Color accentColor = ctx_.accent;
        UiIcon statusIcon = UiIcon::Success;

        switch (toast.type) {
            case ToastType::Success:
                accentColor = { 46, 204, 113 };
                statusIcon = UiIcon::Success;
                break;
            case ToastType::Info:
                accentColor = { 52, 152, 219 };
                statusIcon = UiIcon::Folder;
                break;
            case ToastType::Warning:
                accentColor = { 241, 196, 15 };
                statusIcon = UiIcon::Failed;
                break;
            case ToastType::Error:
                accentColor = { 231, 76, 60 };
                statusIcon = UiIcon::Failed;
                break;
        }

        scene_->DrawRectangle(x, y, 6, toastHeight, accentColor);
        DrawIconNearest(statusIcon, x + 20, y + 26, 24);
        Text(smallText_, smallText_.Truncate(toast.message, 25), x + 54, y + 46, ctx_.text);
    }
}

bool ExplorerUi::DrawIconAsset(UiIcon icon, int x, int y, int size, Color color) {
    (void)color;
    const EmbeddedIcon asset = EmbeddedIconFor(icon);
    if (asset.start == NULL || asset.end == NULL ||
        static_cast<size_t>(asset.end - asset.start) < kEmbeddedIconSize * kEmbeddedIconSize * 4) {
        return false;
    }
    const unsigned char *rgba = asset.start;
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            const int sourceY0 = (r * kEmbeddedIconSize) / size;
            int sourceY1 = ((r + 1) * kEmbeddedIconSize + size - 1) / size;
            const int sourceX0 = (c * kEmbeddedIconSize) / size;
            int sourceX1 = ((c + 1) * kEmbeddedIconSize + size - 1) / size;
            if (sourceY1 <= sourceY0) sourceY1 = sourceY0 + 1;
            if (sourceX1 <= sourceX0) sourceX1 = sourceX0 + 1;

            long long red = 0;
            long long green = 0;
            long long blue = 0;
            int alpha = 0;
            int samples = 0;
            for (int sourceY = sourceY0; sourceY < sourceY1 && sourceY < kEmbeddedIconSize; ++sourceY) {
                for (int sourceX = sourceX0; sourceX < sourceX1 && sourceX < kEmbeddedIconSize; ++sourceX) {
                    const size_t offset = (sourceY * kEmbeddedIconSize + sourceX) * 4;
                    const int a = rgba[offset + 3];
                    red += rgba[offset] * a;
                    green += rgba[offset + 1] * a;
                    blue += rgba[offset + 2] * a;
                    alpha += a;
                    ++samples;
                }
            }
            if (samples == 0 || alpha == 0) continue;
            Color pixel = {
                static_cast<uint8_t>(red / (samples * 255)),
                static_cast<uint8_t>(green / (samples * 255)),
                static_cast<uint8_t>(blue / (samples * 255)),
            };
            scene_->DrawRectangle(x + c, y + r, 1, 1, pixel);
        }
    }
    return true;
}

// Nearest-neighbor fast path for animated / frequently-redrawn icons.
// Avoids the O(n²×m²) area-weighted sampling of DrawIconAsset.
bool ExplorerUi::DrawIconNearest(UiIcon icon, int x, int y, int size) {
    const EmbeddedIcon asset = EmbeddedIconFor(icon);
    if (asset.start == NULL || asset.end == NULL ||
        static_cast<size_t>(asset.end - asset.start) < kEmbeddedIconSize * kEmbeddedIconSize * 4) {
        return false;
    }
    const unsigned char *rgba = asset.start;
    for (int r = 0; r < size; ++r) {
        const int sourceY = (r * kEmbeddedIconSize) / size;
        const size_t rowBase = static_cast<size_t>(sourceY) * kEmbeddedIconSize * 4;
        for (int c = 0; c < size; ++c) {
            const int sourceX = (c * kEmbeddedIconSize) / size;
            const size_t offset = rowBase + static_cast<size_t>(sourceX) * 4;
            if (rgba[offset + 3] == 0) continue;
            Color pixel = { rgba[offset], rgba[offset + 1], rgba[offset + 2] };
            scene_->DrawRectangle(x + c, y + r, 1, 1, pixel);
        }
    }
    return true;
}


UiIcon ExplorerUi::MenuIcon(MenuAction action) const {
    switch (action) {
        case MenuAction::Refresh: return UiIcon::MenuRefresh;
        case MenuAction::CycleSort: return UiIcon::MenuSort;
        case MenuAction::Properties: return UiIcon::MenuProperties;
        case MenuAction::NewFolder: return UiIcon::MenuNewFolder;
        case MenuAction::Rename: return UiIcon::MenuRename;
        case MenuAction::Copy: return UiIcon::MenuCopy;
        case MenuAction::Cut: return UiIcon::MenuCut;
        case MenuAction::Paste: return UiIcon::MenuForward;
        case MenuAction::Delete: return UiIcon::MenuDelete;
    }
    return UiIcon::MenuForward;
}

UiIcon ExplorerUi::FileEntryIcon(const FileEntry &entry) const {
    return entry.type == FileType::Directory ? UiIcon::Folder : UiIcon::File;
}

struct Color ExplorerUi::FileEntryIconColor(const FileEntry &entry) const {
    switch (entry.type) {
        case FileType::Directory: return ctx_.folder;
        case FileType::Text: return ctx_.textFile;
        case FileType::Image: return ctx_.imageFile;
        case FileType::Package: return ctx_.packageFile;
        case FileType::Archive: return ctx_.archiveFile;
        case FileType::Executable: return ctx_.executableFile;
        case FileType::Binary: return ctx_.binaryFile;
        case FileType::Symlink: return ctx_.accent;
        case FileType::Special: return ctx_.danger;
        case FileType::Unknown: return ctx_.muted;
    }
    return ctx_.muted;
}

const char *ExplorerUi::OperationLogText(OperationLogAction action) const {
    switch (action) {
        case OperationLogAction::NewFolder: return localizer_->Get(TextId::LogNewFolder);
        case OperationLogAction::Rename: return localizer_->Get(TextId::LogRename);
        case OperationLogAction::Copy: return localizer_->Get(TextId::LogCopy);
        case OperationLogAction::Move: return localizer_->Get(TextId::LogMove);
        case OperationLogAction::Delete: return localizer_->Get(TextId::LogDelete);
    }
    return "";
}

std::string ExplorerUi::StatusText(const FileBrowser &browser) const {
    char buffer[64];
    if (browser.HasMarkedEntries()) {
        snprintf(buffer, sizeof(buffer), "%d %s", browser.MarkedCount(), localizer_->Get(TextId::StatusSelected));
        return buffer;
    }
    switch (browser.Status()) {
        case BrowserStatus::Items:
            snprintf(buffer, sizeof(buffer), "%d %s", browser.StatusValue(), localizer_->Get(TextId::StatusItems));
            return buffer;
        case BrowserStatus::OpenFailed: return localizer_->Get(TextId::StatusOpenFailed);
        case BrowserStatus::Empty: return localizer_->Get(TextId::StatusEmpty);
        case BrowserStatus::PreviewLater: return localizer_->Get(TextId::StatusPreviewLater);
        case BrowserStatus::PreviewAvailable: return localizer_->Get(TextId::StatusPreviewAvailable);
        case BrowserStatus::NoLocations: return localizer_->Get(TextId::StatusNoLocations);
        case BrowserStatus::Refreshed: return localizer_->Get(TextId::StatusRefreshed);
        case BrowserStatus::Created: return localizer_->Get(TextId::StatusCreated);
        case BrowserStatus::Renamed: return localizer_->Get(TextId::StatusRenamed);
        case BrowserStatus::Deleted: return localizer_->Get(TextId::StatusDeleted);
        case BrowserStatus::InvalidName: return localizer_->Get(TextId::StatusInvalidName);
        case BrowserStatus::AlreadyExists: return localizer_->Get(TextId::StatusAlreadyExists);
        case BrowserStatus::OperationFailed: return localizer_->Get(TextId::StatusOperationFailed);
    }
    return "";
}

const char *ExplorerUi::SortText(SortMode mode) const {
    if (mode == SortMode::Size) return localizer_->Get(TextId::SortSize);
    if (mode == SortMode::Modified) return localizer_->Get(TextId::SortModified);
    return localizer_->Get(TextId::SortName);
}

const char *ExplorerUi::FileTypeText(FileType type) const {
    switch (type) {
        case FileType::Directory: return localizer_->Get(TextId::TypeDirectory);
        case FileType::Text: return localizer_->Get(TextId::TypeText);
        case FileType::Image: return localizer_->Get(TextId::TypeImage);
        case FileType::Package: return localizer_->Get(TextId::TypePackage);
        case FileType::Archive: return localizer_->Get(TextId::TypeArchive);
        case FileType::Executable: return localizer_->Get(TextId::TypeExecutable);
        case FileType::Binary: return localizer_->Get(TextId::TypeBinary);
        case FileType::Symlink: return "Symlink";
        case FileType::Special: return "Special";
        case FileType::Unknown: return localizer_->Get(TextId::TypeUnknown);
    }
    return localizer_->Get(TextId::TypeUnknown);
}

const char *ExplorerUi::MenuText(MenuAction action) const {
    switch (action) {
        case MenuAction::Refresh: return localizer_->Get(TextId::MenuRefresh);
        case MenuAction::CycleSort: return localizer_->Get(TextId::MenuSort);
        case MenuAction::Properties: return localizer_->Get(TextId::MenuProperties);
        case MenuAction::NewFolder: return localizer_->Get(TextId::MenuNewFolder);
        case MenuAction::Rename: return localizer_->Get(TextId::MenuRename);
        case MenuAction::Copy: return localizer_->Get(TextId::MenuCopy);
        case MenuAction::Cut: return localizer_->Get(TextId::MenuCut);
        case MenuAction::Paste: return localizer_->Get(TextId::MenuPaste);
        case MenuAction::Delete: return localizer_->Get(TextId::MenuDelete);
    }
    return "";
}
