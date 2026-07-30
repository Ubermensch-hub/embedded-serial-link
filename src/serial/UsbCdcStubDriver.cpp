#include "esl/serial/UsbCdcStubDriver.h"

#include "esl/util/Logger.h"

namespace esl {
namespace serial {

UsbCdcStubDriver::UsbCdcStubDriver(const char* name, bool loopbackEnabled)
    : InMemoryLoopbackDriver(name, loopbackEnabled), connected_(true) {}

bool UsbCdcStubDriver::open() {
    if (!connected_) {
        ESL_LOG_CALL(name(), "open() rejected: device not enumerated (cable unplugged)");
        return false;
    }
    return InMemoryLoopbackDriver::open();
}

void UsbCdcStubDriver::setConnected(bool connected) {
    ESL_LOG_CALL(name(), "setConnected(%s)", connected ? "true" : "false");
    connected_ = connected;
    if (!connected_) {
        close();
    }
}

}  // namespace serial
}  // namespace esl
