#include "settings_view.h"
#include "ui_context.h"
#include "ui.h"
#include "graphics.h"

SettingsView::SettingsView() {}

void SettingsView::Draw(const UiContext &ctx, int selectedIndex, const AppConfig &config) {
    const int x = 480;
    const int y = 210;
    const int width = 960;
    const int height = 660;

    ctx.scene->DrawRectangle(x, y, width, height, ctx.panelDark);
    ctx.scene->DrawRectangle(x + 18, y + 72, width - 36, 2, ctx.accent);

    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::SettingsTitle), x + 28, y + 50, ctx.text);

    const int rowCount = 2;
    const int startRowY = y + 104;
    const int rowHeight = 74;

    for (int i = 0; i < rowCount; ++i) {
        const int rowY = startRowY + i * rowHeight;
        const bool selected = i == selectedIndex;

        if (selected) {
            ctx.scene->DrawRectangle(x + 18, rowY - 20, width - 36, 58, ctx.selection);
        } else {
            ctx.scene->DrawRectangle(x + 18, rowY - 20, width - 36, 58, ctx.panel);
        }

        const struct Color iconColor = selected ? ctx.text : ctx.accent;
        const struct Color textColor = ctx.text;
        const struct Color valColor = selected ? ctx.text : ctx.muted;

        if (i == 0) {
            ctx.ui->DrawIconAtBaseline(UiIcon::SettingsLanguage, x + 36, rowY + 16, iconColor);
            ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::SettingsLanguage), x + 90, rowY + 16, textColor);
            ctx.ui->Text(ctx.smallText, ctx.localizer->LanguageName(), x + width - 300, rowY + 16, valColor);
        } else if (i == 1) {
            ctx.ui->DrawIconAtBaseline(UiIcon::SettingsTemperature, x + 36, rowY + 16, iconColor);
            ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::SettingsTempUnit), x + 90, rowY + 16, textColor);
            ctx.ui->Text(ctx.smallText, config.tempFahrenheit ? ctx.localizer->Get(TextId::OptionFahrenheit) : ctx.localizer->Get(TextId::OptionCelsius), x + width - 300, rowY + 16, valColor);
        } else if (i == 2) {
            ctx.ui->DrawIconAtBaseline(UiIcon::SettingsLanguage, x + 36, rowY + 16, iconColor);
            ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::SettingsTheme), x + 90, rowY + 16, textColor);
            ctx.ui->Text(ctx.smallText, ctx.localizer->Get(TextId::ThemeNight), x + width - 300, rowY + 16, valColor);
        }
    }

    const int footerY = y + height - 28;
    int nextX = ctx.ui->DrawButtonHint(UiIcon::ButtonUpDown, ctx.localizer->Get(TextId::HintNavigate), x + 28, footerY, ctx.text);
    nextX = ctx.ui->DrawButtonHint(UiIcon::ButtonLeftRight, ctx.localizer->Get(TextId::HintSwitch), nextX, footerY, ctx.text);
    nextX = ctx.ui->DrawButtonHint(UiIcon::ButtonCross, ctx.localizer->Get(TextId::HintAction), nextX, footerY, ctx.text);
    ctx.ui->DrawButtonHint(UiIcon::ButtonCircle, ctx.localizer->Get(TextId::HintSaveExit), nextX, footerY, ctx.text);
}
