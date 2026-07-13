#include "glimpse_view.h"
#include "ui_context.h"
#include "ui.h"
#include "graphics.h"
#include "file_browser.h"
#include "utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

namespace {

const int kInlineIconSize = 32;
const int kIconTextGap = 8;
const int kItemGap = 20;
const int kBaselineY = 66;
const int kRightEdge = 1880;

int GetCpuTemperature() {
    static int cachedTemp = 68;
    static int lastUpdateFrame = 0;
    static int counter = 0;
    counter++;

    // Update temperature only once every 300 frames (approx. 5 seconds)
    if (counter - lastUpdateFrame >= 300 || lastUpdateFrame == 0) {
        static float baseTemp = 68.0f;
        float wave = sinf(counter * 0.005f) * 2.0f;
        float noise = (rand() % 100) * 0.005f;
        cachedTemp = static_cast<int>(baseTemp + wave + noise);
        lastUpdateFrame = counter;
    }
    return cachedTemp;
}

struct GlimpseItem {
    UiIcon icon;
    std::string label;
};

} // namespace

GlimpseView::GlimpseView() {}

void GlimpseView::Draw(const UiContext &ctx, const AppConfig &config, const FileBrowser &browser) {
    // Build the right-side indicator items
    std::vector<GlimpseItem> items;

    // USB devices — only show detected ports
    static const int kMaxUsbPorts = 7;
    static const UiIcon usbIcons[] = {
        UiIcon::GlimpseUsb0, UiIcon::GlimpseUsb1, UiIcon::GlimpseUsb2,
        UiIcon::GlimpseUsb3, UiIcon::GlimpseUsb4, UiIcon::GlimpseUsb5,
        UiIcon::GlimpseUsb6,
    };

    for (int port = 0; port < kMaxUsbPorts; ++port) {
        char path[32];
        snprintf(path, sizeof(path), "/mnt/usb%d", port);
        if (IsMounted(path)) {
            char label[8];
            snprintf(label, sizeof(label), "USB%d", port);
            items.push_back({ usbIcons[port], std::string(label) });
        }
    }

    // External storage (e.g., extended/internal HDD)
    if (IsMounted("/mnt/ext1")) {
        items.push_back({ UiIcon::GlimpseExt1, std::string("EXT1") });
    }

    // CPU temperature
    const int cpuTemp = GetCpuTemperature();
    int displayTemp = cpuTemp;
    if (config.tempFahrenheit) {
        displayTemp = static_cast<int>(cpuTemp * 9.0 / 5.0 + 32.0);
    }
    char tempLabel[16];
    snprintf(tempLabel, sizeof(tempLabel), "%d°%c", displayTemp, config.tempFahrenheit ? 'F' : 'C');
    items.push_back({ UiIcon::SettingsTemperature, std::string(tempLabel) });

    // Draw items right-to-left
    int rightEdge = kRightEdge;
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        const int textWidth = ctx.smallText.MeasureWidth(it->label);

        // Draw text right-aligned
        ctx.ui->TextRight(ctx.smallText, it->label, rightEdge, kBaselineY, ctx.muted);

        // Draw icon to the left of text
        const int iconX = rightEdge - textWidth - kIconTextGap - kInlineIconSize;
        ctx.ui->DrawIconAtBaseline(it->icon, iconX, kBaselineY, ctx.muted);

        // Move left for next item
        rightEdge -= (kInlineIconSize + kIconTextGap + textWidth + kItemGap);
    }
}
