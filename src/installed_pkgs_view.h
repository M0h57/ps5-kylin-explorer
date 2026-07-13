#ifndef KYLIN_EXPLORER_INSTALLED_PKGS_VIEW_H
#define KYLIN_EXPLORER_INSTALLED_PKGS_VIEW_H

#include <vector>

struct UiContext;
struct InstalledApp;

class InstalledPkgsView {
public:
    InstalledPkgsView();
    void Draw(const UiContext &ctx, const std::vector<InstalledApp> &apps, int selectedIndex);
};

#endif
