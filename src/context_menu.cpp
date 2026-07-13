#include "context_menu.h"

ContextMenu::ContextMenu() : open_(false), selectedIndex_(0) {
    items_[0] = { MenuAction::Refresh, true };
    items_[1] = { MenuAction::CycleSort, true };
    items_[2] = { MenuAction::Properties, true };
    items_[3] = { MenuAction::NewFolder, true };
    items_[4] = { MenuAction::Rename, true };
    items_[5] = { MenuAction::Copy, true };
    items_[6] = { MenuAction::Cut, true };
    items_[7] = { MenuAction::Paste, true };
    items_[8] = { MenuAction::Delete, true };
}

void ContextMenu::Open(bool hasSelection, bool hasClipboard) {
    open_ = true;
    selectedIndex_ = 0;

    for (int i = 0; i < kContextMenuItemCount; ++i) {
        switch (items_[i].action) {
            case MenuAction::Properties:
            case MenuAction::Rename:
            case MenuAction::Copy:
            case MenuAction::Cut:
            case MenuAction::Delete:
                items_[i].enabled = hasSelection;
                break;
            case MenuAction::Paste:
                items_[i].enabled = hasClipboard;
                break;
            default:
                items_[i].enabled = true;
                break;
        }
    }

    // Ensure we start selectedIndex_ on an enabled item
    while (selectedIndex_ < kContextMenuItemCount && !items_[selectedIndex_].enabled) {
        selectedIndex_++;
    }
    if (selectedIndex_ >= kContextMenuItemCount) {
        selectedIndex_ = 0;
    }
}

void ContextMenu::Close() {
    open_ = false;
    selectedIndex_ = 0;
}

void ContextMenu::Move(int delta) {
    if (ItemCount() == 0) return;
    
    int originalIndex = selectedIndex_;
    do {
        selectedIndex_ += delta;
        if (selectedIndex_ < 0) {
            selectedIndex_ = ItemCount() - 1;
        } else if (selectedIndex_ >= ItemCount()) {
            selectedIndex_ = 0;
        }
        
        if (items_[selectedIndex_].enabled) {
            return;
        }
    } while (selectedIndex_ != originalIndex);
}

bool ContextMenu::IsOpen() const { return open_; }
int ContextMenu::SelectedIndex() const { return selectedIndex_; }
const MenuItem &ContextMenu::Selected() const { return items_[selectedIndex_]; }
const MenuItem *ContextMenu::Items() const { return items_; }
int ContextMenu::ItemCount() const { return sizeof(items_) / sizeof(items_[0]); }
