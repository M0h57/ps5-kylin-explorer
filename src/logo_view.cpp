#include "logo_view.h"
#include "ui_context.h"
#include "ui.h"
#include "localization.h"
#include "graphics.h"

LogoView::LogoView() {}

void LogoView::Draw(const UiContext &ctx) {
    ctx.ui->Text(ctx.bodyText, ctx.localizer->Get(TextId::AppTitle), 40, 66, ctx.text);
}
