#include "modal_view.h"
#include "ui_context.h"
#include "ui.h"
#include "graphics.h"
#include "file_browser.h"
#include "file_operations.h"
#include "operation_log.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>

namespace {
std::string EntryDisplayName(const FileEntry &entry) {
    return entry.displayName.empty() ? DisplayCodec::DecodeBytes(entry.name).text : entry.displayName;
}

std::string DisplayBytes(const std::string &bytes) {
    return DisplayCodec::DecodeBytes(bytes).text;
}
}

ModalView::ModalView() {}

void ModalView::DrawProperties(const UiContext &ctx, const FileBrowser &browser) {
    const FileEntry *selected = browser.Selected();
    if (selected == NULL) return;

    const int x = 650;
    const int y = 330;
    ctx.scene->DrawRectangle(x, y, 620, 360, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 74, 584, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::PropertiesTitle), x + 28, y + 52, ctx.text);
    ctx.ui->Text(ctx.bodyText, ctx.bodyText.Truncate(EntryDisplayName(*selected), 32), x + 28, y + 124, ctx.text);

    std::string type = std::string("TYPE  ") + ctx.ui->FileTypeText(selected->type);
    ctx.ui->Text(ctx.smallText, type, x + 28, y + 180, ctx.muted);

    char size[64];
    FormatSize(size, sizeof(size), selected->size);
    ctx.ui->Text(ctx.smallText, std::string(ctx.localizer->Get(TextId::Size)) + "  " + size, x + 28, y + 224, ctx.muted);

    char modified[48];
    FormatModifiedTime(modified, sizeof(modified), selected->modifiedTime);
    ctx.ui->Text(ctx.smallText, std::string(ctx.localizer->Get(TextId::Modified)) + "  " + modified, x + 28, y + 268, ctx.muted);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintClose), x + 28, y + 324, ctx.text);
}

void ModalView::DrawError(const UiContext &ctx, const FileBrowser &browser, const std::string &errorText, const std::string &title) {
    const int x = 680;
    const int y = 390;
    ctx.scene->DrawRectangle(x, y, 560, 238, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, 524, 2, ctx.accent);
    std::string displayTitle = title.empty() ? ctx.localizer->Get(TextId::ErrorTitle) : title;
    ctx.ui->Text(ctx.bodyText, displayTitle, x + 28, y + 50, ctx.text);
    std::string displayMsg = errorText.empty() ? ctx.ui->StatusText(browser) : errorText;
    ctx.ui->Text(ctx.smallText, displayMsg, x + 28, y + 132, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintClose), x + 28, y + 194, ctx.muted);
}

void ModalView::DrawCopyProgress(const UiContext &ctx, const FileOperations &operations) {
    const int x = 610;
    const int y = 390;
    const int width = 700;
    ctx.scene->DrawRectangle(x, y, width, 270, ctx.panelDark);
    const bool moving = operations.IsMoveOperation();
    const bool deleting = operations.IsDeleteOperation();
    ctx.ui->DrawIcon(deleting ? UiIcon::Delete : (moving ? UiIcon::SendTo : UiIcon::Copy),
             x + 28, y + 24, 38, deleting ? ctx.danger : ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(deleting ? TextId::DeleteProgressTitle :
                                   (moving ? TextId::MoveTitle : TextId::CopyTitle)), x + 82, y + 52, ctx.text);
    ctx.ui->Text(ctx.smallText, ctx.smallText.Truncate(DisplayBytes(operations.CurrentName()), 48), x + 28, y + 108, ctx.text);

    ctx.scene->DrawRectangle(x + 28, y + 142, width - 56, 22, ctx.panel);
    const int progressWidth = ((width - 56) * operations.ProgressPercent()) / 100;
    ctx.scene->DrawRectangle(x + 28, y + 142, progressWidth, 22, ctx.accent);

    char progress[64];
    snprintf(progress, sizeof(progress), "%d%%", operations.ProgressPercent());
    ctx.ui->Text(ctx.smallText, progress, x + 28, y + 204, ctx.muted);

    if (!deleting) {
        char speed[32];
        FormatSize(speed, sizeof(speed), operations.BytesPerSecond());
        ctx.ui->Text(ctx.smallText, std::string(speed) + "/s", x + 126, y + 204, ctx.muted);
    }

    const char *footer = deleting ? ctx.localizer->Get(TextId::DeleteInProgress) : NULL;
    if (deleting) {
        if (operations.State() == CopyState::Completed) footer = ctx.localizer->Get(TextId::DeleteCompleted);
        if (operations.State() == CopyState::Failed) footer = ctx.localizer->Get(TextId::DeleteFailed);
    } else {
        if (operations.State() == CopyState::Completed) footer = ctx.localizer->Get(moving ? TextId::MoveCompleted : TextId::CopyCompleted);
        if (operations.State() == CopyState::Canceled) footer = ctx.localizer->Get(moving ? TextId::MoveCanceled : TextId::CopyCanceled);
        if (operations.State() == CopyState::Failed) footer = ctx.localizer->Get(moving ? TextId::MoveFailed : TextId::CopyFailed);
    }
    if (footer != NULL) {
        ctx.ui->Text(ctx.smallText, footer, x + 520, y + 204, ctx.text);
    } else {
        ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintCancel), x + 520, y + 204, ctx.text);
    }
}

void ModalView::DrawQuickPlaces(const UiContext &ctx, const std::vector<BrowserLocation> &places, int selectedIndex) {
    const int x = 560;
    const int y = 280;
    const int width = 800;
    const int rowHeight = 56;
    const int rowCount = static_cast<int>(places.size());
    const int contentHeight = 120 + rowCount * rowHeight;
    const int panelHeight = contentHeight > 280 ? contentHeight : 280;

    ctx.scene->DrawRectangle(x, y, width, panelHeight, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::QuickPlacesTitle), x + 28, y + 50, ctx.text);

    if (rowCount == 0) {
        ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::QuickPlacesEmpty), x + 28, y + 126, ctx.muted);
    }

    for (int i = 0; i < rowCount; ++i) {
        const int rowY = y + 106 + i * rowHeight;
        const bool selected = i == selectedIndex;

        if (selected) {
            ctx.scene->DrawRectangle(x + 14, rowY - 30, width - 28, 48, ctx.selection);
        }

        // Map USB/EXT label to icon
        UiIcon icon = UiIcon::GlimpseExt1;
        const std::string &label = places[i].label;
        if (label.find("USB") == 0) {
            int n = label[3] - '0';
            if (n >= 0 && n <= 6) {
                icon = static_cast<UiIcon>(static_cast<int>(UiIcon::GlimpseUsb0) + n);
            }
        }

        ctx.ui->DrawIconAtBaseline(icon, x + 32, rowY, ctx.accent);
        const std::string text = label + "  " + DisplayBytes(places[i].path);
        ctx.ui->Text(ctx.smallText, ctx.smallText.Truncate(text, 46), x + 80, rowY, ctx.text);
    }

    const int footerY = y + panelHeight - 28;
    ctx.ui->DrawButtonHint(UiIcon::ButtonUpDown, ctx.localizer->Get(TextId::HintNavigate), x + 28, footerY, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCross, ctx.localizer->Get(TextId::QuickPlacesJump), x + 240, footerY, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintClose), x + 440, footerY, ctx.muted);
}

void ModalView::DrawOverwriteConfirm(const UiContext &ctx) {
    const int x = 650;
    const int y = 390;
    const int width = 620;
    ctx.scene->DrawRectangle(x, y, width, 238, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::OverwriteTitle), x + 28, y + 50, ctx.text);
    ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::OverwriteMessage), x + 28, y + 132, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCross, ctx.localizer->Get(TextId::HintReplace), x + 28, y + 194, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintCancel), x + 430, y + 194, ctx.muted);
}

void ModalView::DrawDeleteConfirm(const UiContext &ctx) {
    const int x = 650;
    const int y = 390;
    const int width = 620;
    ctx.scene->DrawRectangle(x, y, width, 238, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::DeleteTitle), x + 28, y + 50, ctx.text);
    ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::DeleteMessage), x + 28, y + 132, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCross, ctx.localizer->Get(TextId::HintDelete), x + 28, y + 194, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintCancel), x + 430, y + 194, ctx.muted);
}

void ModalView::DrawRecursiveDeleteConfirm(const UiContext &ctx) {
    const int x = 610;
    const int y = 390;
    const int width = 700;
    ctx.scene->DrawRectangle(x, y, width, 238, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::RecursiveDeleteTitle), x + 28, y + 50, ctx.text);
    ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::RecursiveDeleteMessage), x + 28, y + 132, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCross, ctx.localizer->Get(TextId::HintDeleteTree), x + 28, y + 194, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintCancel), x + 510, y + 194, ctx.muted);
}

void ModalView::DrawOperationLog(const UiContext &ctx, const OperationLog &operationLog) {
    const int x = 500;
    const int y = 230;
    const int width = 920;
    const int height = 610;
    ctx.scene->DrawRectangle(x, y, width, height, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::OperationLogTitle), x + 28, y + 50, ctx.text);

    const std::vector<OperationLogEntry> &entries = operationLog.Entries();
    if (entries.empty()) {
        ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::OperationLogEmpty), x + 28, y + 142, ctx.muted);
    }
    for (size_t index = 0; index < entries.size(); ++index) {
        const int rowY = y + 124 + static_cast<int>(index) * 54;
        const std::string status = ctx.localizer->Get(entries[index].succeeded ? TextId::LogSucceeded : TextId::LogFailed);
        ctx.ui->DrawIconAtBaseline(entries[index].succeeded ? UiIcon::Success : UiIcon::Failed,
                 x + 28, rowY, entries[index].succeeded ? ctx.green : ctx.danger);
        ctx.ui->Text(ctx.smallText, ctx.ui->OperationLogText(entries[index].action), x + 78, rowY, ctx.text);
        ctx.ui->Text(ctx.smallText, status, x + 220, rowY, entries[index].succeeded ? ctx.green : ctx.danger);
        ctx.ui->Text(ctx.smallText, ctx.smallText.Truncate(DisplayBytes(entries[index].path), 52), x + 310, rowY, ctx.muted);
    }
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintClose), x + 28, y + height - 28, ctx.text);
}

void ModalView::DrawPkgProgress(const UiContext &ctx, const std::string &path, int progress, const std::string &status, bool finished) {
    const int x = 610;
    const int y = 390;
    const int width = 700;
    ctx.scene->DrawRectangle(x, y, width, 270, ctx.panelDark);
    
    // Draw Package Icon
    ctx.ui->DrawIcon(UiIcon::PackageFile, x + 28, y + 24, 38, ctx.accent);
    
    // Draw Title
    ctx.ui->Text(ctx.bodyText, "Package Installation", x + 82, y + 52, ctx.text);
    
    // Draw PKG filename
    ctx.ui->Text(ctx.smallText, ctx.smallText.Truncate(DisplayBytes(path), 48), x + 28, y + 108, ctx.text);

    // Draw Progress Bar background
    ctx.scene->DrawRectangle(x + 28, y + 142, width - 56, 22, ctx.panel);
    
    // Draw Progress Bar foreground
    const int progressWidth = ((width - 56) * progress) / 100;
    ctx.scene->DrawRectangle(x + 28, y + 142, progressWidth, 22, finished ? ctx.green : ctx.accent);

    // Draw percentage text
    char progress_str[64];
    snprintf(progress_str, sizeof(progress_str), "%d%%", progress);
    ctx.ui->Text(ctx.smallText, progress_str, x + 28, y + 204, ctx.muted);

    // Draw status / footer text
    ctx.ui->Text(ctx.smallText, status, x + 160, y + 204, ctx.text);
    
    // Draw cancel/dismiss footer instructions
    if (finished) {
        int nextX = ctx.ui->DrawButtonHint(UiIcon::ButtonCross, ctx.localizer->Get(TextId::HintBack), x + 500, y + 204, ctx.muted);
        ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintBack), nextX, y + 204, ctx.muted);
    } else {
        ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintCancel), x + 520, y + 204, ctx.muted);
    }
}
