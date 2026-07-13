#ifndef KYLIN_EXPLORER_SIDEBAR_VIEW_H
#define KYLIN_EXPLORER_SIDEBAR_VIEW_H

struct UiContext;
class FileBrowser;

class SidebarView {
public:
    SidebarView();
    void Draw(const UiContext &ctx, const FileBrowser &browser);
};

#endif
