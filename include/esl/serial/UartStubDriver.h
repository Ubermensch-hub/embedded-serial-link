#pragma once

#include "esl/serial/InMemoryLoopbackDriver.h"

namespace esl {
namespace serial {

// Полнодуплексный UART работает точно как базовый loopback-класс
class UartStubDriver : public InMemoryLoopbackDriver {
public:
    explicit UartStubDriver(const char* name = "UART", bool loopbackEnabled = true)
        : InMemoryLoopbackDriver(name, loopbackEnabled) {}
};

}  // namespace serial
}  // namespace esl
