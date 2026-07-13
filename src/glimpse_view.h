#ifndef KYLIN_EXPLORER_GLIMPSE_VIEW_H
#define KYLIN_EXPLORER_GLIMPSE_VIEW_H

struct UiContext;
struct AppConfig;
class FileBrowser;

class GlimpseView {
public:
    GlimpseView();
    void Draw(const UiContext &ctx, const AppConfig &config, const FileBrowser &browser);
};

#endif
