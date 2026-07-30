#include "esl/serial/InMemoryLoopbackDriver.h"

#include "esl/util/Logger.h"

namespace esl {
namespace serial {

InMemoryLoopbackDriver::InMemoryLoopbackDriver(const char* name, bool loopbackEnabled)
    : name_(name),
      loopbackEnabled_(loopbackEnabled),
      open_(false),
      rxCallback_(nullptr),
      rxContext_(nullptr) {}

bool InMemoryLoopbackDriver::open() {
    ESL_LOG_CALL(name_, "open()");
    open_ = true;
    return true;
}

void InMemoryLoopbackDriver::close() {
    ESL_LOG_CALL(name_, "close()");
    open_ = false;
    rxBuffer_.clear();
    txLog_.clear();
}

bool InMemoryLoopbackDriver::isOpen() const { return open_; }

std::size_t InMemoryLoopbackDriver::write(const std::uint8_t* data, std::size_t len) {
    ESL_LOG_CALL(name_, "write(len=%zu)", len);
    if (!open_) {
        ESL_LOG_CALL(name_, "write() rejected: driver not open");
        return 0;
    }

    std::size_t accepted = txLog_.pushBulk(data, len);
    if (loopbackEnabled_ && accepted > 0) {
        rxBuffer_.pushBulk(data, accepted);
    }
    return accepted;
}

void InMemoryLoopbackDriver::setRxCallback(RxCallback callback, void* context) {
    ESL_LOG_CALL(name_, "setRxCallback(%s)", callback ? "set" : "clear");
    rxCallback_ = callback;
    rxContext_ = context;
}

void InMemoryLoopbackDriver::poll() {
    if (!rxCallback_) {
        return;
    }
    std::uint8_t chunk[32];
    std::size_t n;
    while ((n = rxBuffer_.popBulk(chunk, sizeof(chunk))) > 0) {
        rxCallback_(rxContext_, chunk, n);
    }
}

const char* InMemoryLoopbackDriver::name() const { return name_; }

std::size_t InMemoryLoopbackDriver::injectRxData(const std::uint8_t* data, std::size_t len) {
    return rxBuffer_.pushBulk(data, len);
}

void InMemoryLoopbackDriver::setLoopbackEnabled(bool enabled) { loopbackEnabled_ = enabled; }

std::size_t InMemoryLoopbackDriver::drainTxLog(std::uint8_t* out, std::size_t maxLen) {
    return txLog_.popBulk(out, maxLen);
}

}  // namespace serial
}  // namespace esl
