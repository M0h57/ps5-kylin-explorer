#include <assert.h>
#include <stdio.h>
#include <stddef.h>

#include "../src/context_menu.h"

namespace {
bool HasAction(const ContextMenu &menu, MenuAction action) {
    for (int index = 0; index < menu.ItemCount(); ++index) {
        if (menu.Items()[index].action == action) return true;
    }
    return false;
}
}

int main() {
    ContextMenu menu;
    assert(menu.ItemCount() == kContextMenuItemCount);
    assert(!menu.IsOpen());

    const MenuAction expected[] = {
        MenuAction::Refresh,
        MenuAction::CycleSort,
        MenuAction::Properties,
        MenuAction::NewFolder,
        MenuAction::Rename,
        MenuAction::Copy,
        MenuAction::Cut,
        MenuAction::Paste,
        MenuAction::Delete,
    };
    for (size_t index = 0; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        assert(HasAction(menu, expected[index]));
    }

    menu.Open(false, false);
    assert(menu.IsOpen());
    assert(menu.Selected().action == MenuAction::Refresh);
    for (int index = 0; index < menu.ItemCount(); ++index) {
        const MenuAction action = menu.Items()[index].action;
        if (action == MenuAction::Properties || action == MenuAction::Rename ||
            action == MenuAction::Copy || action == MenuAction::Cut ||
            action == MenuAction::Delete ||
            action == MenuAction::Paste) {
            assert(!menu.Items()[index].enabled);
        } else {
            assert(menu.Items()[index].enabled);
        }
    }

    menu.Open(true, false);
    assert(menu.Selected().action == MenuAction::Refresh);
    assert(!menu.Items()[7].enabled); // Paste
    menu.Move(-1);
    assert(menu.Selected().action == MenuAction::Delete);
    menu.Move(1);
    assert(menu.Selected().action == MenuAction::Refresh);

    menu.Open(true, true);
    for (int index = 0; index < menu.ItemCount(); ++index) {
        assert(menu.Items()[index].enabled);
    }
    menu.Close();
    assert(!menu.IsOpen());
    assert(menu.SelectedIndex() == 0);

    printf("context-menu-host-tests-ok\n");
    return 0;
}
