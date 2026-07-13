#ifndef KYLIN_EXPLORER_CONTEXT_MENU_H
#define KYLIN_EXPLORER_CONTEXT_MENU_H

enum class MenuAction {
    Refresh,
    CycleSort,
    Properties,
    NewFolder,
    Rename,
    Copy,
    Cut,
    Paste,
    Delete,
};

static const int kContextMenuItemCount = 9;

struct MenuItem {
    MenuAction action;
    bool enabled;
};

class ContextMenu {
public:
    ContextMenu();
    void Open(bool hasSelection, bool hasClipboard);
    void Close();
    void Move(int delta);
    bool IsOpen() const;
    int SelectedIndex() const;
    const MenuItem &Selected() const;
    const MenuItem *Items() const;
    int ItemCount() const;

private:
    bool open_;
    int selectedIndex_;
    MenuItem items_[kContextMenuItemCount];
};

#endif
