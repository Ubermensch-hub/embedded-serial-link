#include "esl/serial/Rs485StubDriver.h"

#include "esl/util/Logger.h"

namespace esl {
namespace serial {

Rs485StubDriver::Rs485StubDriver(const char* name, bool loopbackEnabled)
    : InMemoryLoopbackDriver(name, loopbackEnabled), transmitEnabled_(true) {}

void Rs485StubDriver::setDirection(bool transmitEnabled) {
    ESL_LOG_CALL(name(), "setDirection(%s)", transmitEnabled ? "TX" : "RX");
    transmitEnabled_ = transmitEnabled;
}

std::size_t Rs485StubDriver::write(const std::uint8_t* data, std::size_t len) {
    if (!transmitEnabled_) {
        ESL_LOG_CALL(name(), "write(len=%zu) rejected: transceiver in RX mode", len);
        return 0;
    }
    return InMemoryLoopbackDriver::write(data, len);
}

}  // namespace serial
}  // namespace esl
