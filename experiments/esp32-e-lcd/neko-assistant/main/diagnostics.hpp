#pragma once

#include "board_bus.hpp"
#include "display_manager.hpp"
#include "ui_manager.hpp"

namespace neko {

class Diagnostics final {
public:
    Diagnostics(BoardBus &bus, DisplayManager &display) : bus_(bus), display_(display) {}
    void populate(UiModel &model) const;
    void log_boot() const;
private:
    BoardBus &bus_;
    DisplayManager &display_;
};

}  // namespace neko
