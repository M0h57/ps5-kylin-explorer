#ifndef KYLIN_EXPLORER_SETTINGS_VIEW_H
#define KYLIN_EXPLORER_SETTINGS_VIEW_H

struct UiContext;
struct AppConfig;

class SettingsView {
public:
    SettingsView();
    void Draw(const UiContext &ctx, int selectedIndex, const AppConfig &config);
};

#endif
