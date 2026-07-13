#ifndef KYLIN_EXPLORER_INPUT_H
#define KYLIN_EXPLORER_INPUT_H

#include <stdint.h>
#include <orbis/Pad.h>

class Input {
public:
    Input();
    bool Init();
    void Update();
    bool Pressed(uint32_t button) const;
    bool Held(uint32_t button) const;
    int UserId() const;

private:
    int pad_;
    int userId_;
    uint32_t current_;
    uint32_t previous_;
};

#endif
