#pragma once

#include "esl/serial/InMemoryLoopbackDriver.h"

namespace esl {
namespace serial {

// RS485 на МК полудуплексный
// Этот stub моделирует ограничение, чтобы код протокола не рассчитывал на полный дуплекс.
class Rs485StubDriver : public InMemoryLoopbackDriver {
public:
    explicit Rs485StubDriver(const char* name = "RS485", bool loopbackEnabled = true);

    // Эмуляция пина DE/RE
    void setDirection(bool transmitEnabled);
    bool transmitEnabled() const { return transmitEnabled_; }

    std::size_t write(const std::uint8_t* data, std::size_t len) override;

private:
    bool transmitEnabled_;
};

}  // namespace serial
}  // namespace esl
