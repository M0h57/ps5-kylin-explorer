#include "input.h"

#include <string.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

Input::Input() : pad_(-1), userId_(-1), current_(0), previous_(0) {
}

bool Input::Init() {
    if (scePadInit() != 0) {
        return false;
    }

    OrbisUserServiceInitializeParams params;
    memset(&params, 0, sizeof(params));
    params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&params);

    if (sceUserServiceGetInitialUser(&userId_) < 0) {
        return false;
    }

    pad_ = scePadOpen(userId_, 0, 0, NULL);
    return pad_ >= 0;
}

void Input::Update() {
    OrbisPadData data;
    memset(&data, 0, sizeof(data));
    previous_ = current_;
    if (scePadReadState(pad_, &data) >= 0) {
        current_ = data.buttons;
    }
}

bool Input::Pressed(uint32_t button) const {
    return (current_ & button) != 0 && (previous_ & button) == 0;
}

bool Input::Held(uint32_t button) const {
    return (current_ & button) != 0;
}

int Input::UserId() const { return userId_; }
