#include "installed_pkgs_view.h"
#include "ui_context.h"
#include "ui.h"
#include "graphics.h"
#include "sfo_parser.h"
#include <stdio.h>

InstalledPkgsView::InstalledPkgsView() {}

void InstalledPkgsView::Draw(const UiContext &ctx, const std::vector<InstalledApp> &apps, int selectedIndex) {
    const int x = 480;
    const int y = 210;
    const int width = 960;
    const int height = 660;
    ctx.scene->DrawRectangle(x, y, width, height, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::InstalledPkgsTitle), x + 28, y + 50, ctx.text);

    if (apps.empty()) {
        ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::InstalledPkgsEmpty), x + 28, y + 142, ctx.muted);
    } else {
        int visibleCount = 8;
        int totalApps = static_cast<int>(apps.size());
        int first = selectedIndex < 5 ? 0 : selectedIndex - 4;
        if (first + visibleCount > totalApps) {
            first = totalApps - visibleCount;
        }
        if (first < 0) first = 0;

        for (int i = 0; i < visibleCount && first + i < totalApps; ++i) {
            int appIndex = first + i;
            const int rowY = y + 124 + i * 58;
            const bool selected = appIndex == selectedIndex;

            if (selected) {
                ctx.scene->DrawRectangle(x + 18, rowY - 32, width - 36, 50, ctx.selection);
            } else {
                ctx.scene->DrawRectangle(x + 18, rowY - 32, width - 36, 50, ctx.panel);
            }

            ctx.ui->DrawIconAtBaseline(UiIcon::PackageFile, x + 32, rowY, selected ? ctx.text : ctx.packageFile);

            std::string appTitle = apps[appIndex].title;
            ctx.ui->Text(ctx.smallText, ctx.smallText.Truncate(appTitle, 32), x + 84, rowY, selected ? ctx.text : ctx.text);

            ctx.ui->Text(ctx.smallText, apps[appIndex].titleId, x + 500, rowY, selected ? ctx.text : ctx.muted);

            std::string ver = std::string("v") + apps[appIndex].version;
            ctx.ui->Text(ctx.smallText, ver, x + 720, rowY, selected ? ctx.text : ctx.muted);
        }

        char position[32];
        snprintf(position, sizeof(position), "%d / %d", selectedIndex + 1, totalApps);
        ctx.ui->Text(ctx.smallText, position, x + width - 120, y + 50, ctx.muted);
    }

    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintClose), x + 28, y + height - 28, ctx.text);
}
