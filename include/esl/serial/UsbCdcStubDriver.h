#pragma once

#include "esl/serial/InMemoryLoopbackDriver.h"

namespace esl {
namespace serial {

// USB CDC требует enumeration хостом до появления виртуального COM-порта
class UsbCdcStubDriver : public InMemoryLoopbackDriver {
public:
    explicit UsbCdcStubDriver(const char* name = "USB_CDC", bool loopbackEnabled = true);

    bool open() override;

    // Эмулирует подключение/отключение USB-кабеля
    void setConnected(bool connected);
    bool connected() const { return connected_; }

private:
    bool connected_;
};

}  // namespace serial
}  // namespace esl
