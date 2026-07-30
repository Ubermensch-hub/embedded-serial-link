#pragma once

#include "esl/serial/ISerialDriver.h"
#include "esl/util/RingBuffer.h"

namespace esl {
namespace serial {

// Общая база для stub-драйверов UART/RS485/USB CDC
// Только очереди фиксированного размера в памяти.
//  - injectRxData() хук только для тестов, имитирует данные от внешнего
//    устройства независимо от loopback
// Коллбэк на приём вызывается только внутри poll() — так же, как на
// реальном драйвере, где приём по прерываниям.
class InMemoryLoopbackDriver : public ISerialDriver {
public:
    static constexpr std::size_t kRxCapacity = 256;
    static constexpr std::size_t kTxLogCapacity = 256;

    explicit InMemoryLoopbackDriver(const char* name, bool loopbackEnabled = true);

    bool open() override;
    void close() override;
    bool isOpen() const override;
    std::size_t write(const std::uint8_t* data, std::size_t len) override;
    void setRxCallback(RxCallback callback, void* context) override;
    void poll() override;
    const char* name() const override;

    // Только для тестов
    std::size_t injectRxData(const std::uint8_t* data, std::size_t len);
    void setLoopbackEnabled(bool enabled);
    std::size_t drainTxLog(std::uint8_t* out, std::size_t maxLen);

protected:
    const char* name_;
    bool loopbackEnabled_;
    bool open_;
    RxCallback rxCallback_;
    void* rxContext_;
    util::RingBuffer<std::uint8_t, kRxCapacity> rxBuffer_;
    util::RingBuffer<std::uint8_t, kTxLogCapacity> txLog_;
};

}  // namespace serial
}  // namespace esl
