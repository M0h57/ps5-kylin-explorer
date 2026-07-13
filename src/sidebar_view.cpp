#include "sidebar_view.h"
#include "ui_context.h"
#include "ui.h"
#include "graphics.h"
#include "file_browser.h"
#include "utils.h"

#include <stdio.h>
#include <time.h>
#include <algorithm>

namespace {
std::string EntryDisplayName(const FileEntry &entry) {
    return entry.displayName.empty() ? DisplayCodec::DecodeBytes(entry.name).text : entry.displayName;
}

void FormatPermissions(char *output, size_t size, uint32_t mode) {
    const char values[] = {
        static_cast<char>((mode & 0400) ? 'r' : '-'),
        static_cast<char>((mode & 0200) ? 'w' : '-'),
        static_cast<char>((mode & 0100) ? 'x' : '-'),
        static_cast<char>((mode & 0040) ? 'r' : '-'),
        static_cast<char>((mode & 0020) ? 'w' : '-'),
        static_cast<char>((mode & 0010) ? 'x' : '-'),
        static_cast<char>((mode & 0004) ? 'r' : '-'),
        static_cast<char>((mode & 0002) ? 'w' : '-'),
        static_cast<char>((mode & 0001) ? 'x' : '-'),
        '\0'
    };
    snprintf(output, size, "%s", values);
}
}

SidebarView::SidebarView() {}

void SidebarView::Draw(const UiContext &ctx, const FileBrowser &browser) {
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::Details), 1460, 182, ctx.text);
    const FileEntry *selected = browser.Selected();
    if (selected == NULL) {
        ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::NoItems), 1460, 254, ctx.muted);
        return;
    }

    ctx.scene->DrawRectangle(1460, 222, 390, 2, ctx.accent);
    ctx.ui->DrawIcon(ctx.ui->FileEntryIcon(*selected), 1468, 264, 34, ctx.ui->FileEntryIconColor(*selected));
    ctx.ui->Text(ctx.bodyText, ctx.bodyText.Truncate(EntryDisplayName(*selected), 20), 1530, 292, ctx.text);
    std::string type = std::string("TYPE  ") + ctx.ui->FileTypeText(selected->type);
    ctx.ui->Text(ctx.smallText, type, 1460, 360, ctx.muted);

    char size[48];
    if (selected->isDirectory) {
        snprintf(size, sizeof(size), "%s  --", ctx.localizer->Get(TextId::Size));
    } else {
        char formatted[24];
        FormatSize(formatted, sizeof(formatted), selected->size);
        snprintf(size, sizeof(size), "%s  %s", ctx.localizer->Get(TextId::Size), formatted);
    }
    ctx.ui->Text(ctx.smallText, size, 1460, 406, ctx.muted);

    char modified[48];
    FormatModifiedTime(modified, sizeof(modified), selected->modifiedTime);
    char modifiedText[72];
    snprintf(modifiedText, sizeof(modifiedText), "%s  %s", ctx.localizer->Get(TextId::Modified), modified);
    ctx.ui->Text(ctx.smallText, modifiedText, 1460, 452, ctx.muted);

    char permissions[16];
    FormatPermissions(permissions, sizeof(permissions), selected->mode);
    char permissionText[48];
    snprintf(permissionText, sizeof(permissionText), "%s  %s", ctx.localizer->Get(TextId::Mode), permissions);
    ctx.ui->Text(ctx.smallText, permissionText, 1460, 498, ctx.muted);

    std::string sort = std::string("SORT  ") + ctx.ui->SortText(browser.CurrentSort());
    ctx.ui->Text(ctx.smallText, sort, 1460, 544, ctx.muted);
    if (browser.HasMarkedEntries()) {
        char selectedText[64];
        snprintf(selectedText, sizeof(selectedText), "%d %s", browser.MarkedCount(), ctx.localizer->Get(TextId::StatusSelected));
        int markedY = 608;
        ctx.ui->Text(ctx.smallText, selectedText, 1460, markedY, ctx.green);
        ctx.ui->Text(ctx.smallText, ctx.smallText.Truncate(ctx.localizer->Get(TextId::SelectionHint), 34),
             1460, markedY + 44, ctx.muted);
    }
}
