#include <cstring>
#include <vector>

#include "doctest/doctest.h"
#include "esl/serial/Rs485StubDriver.h"
#include "esl/serial/UartStubDriver.h"
#include "esl/serial/UsbCdcStubDriver.h"

namespace {

struct RxCapture {
    std::vector<std::uint8_t> data;
    int callCount = 0;
};

void captureCallback(void* context, const std::uint8_t* data, std::size_t len) {
    auto* cap = static_cast<RxCapture*>(context);
    ++cap->callCount;
    cap->data.insert(cap->data.end(), data, data + len);
}

}  // namespace

TEST_CASE("UartStubDriver: loopback delivers written bytes back only once poll() runs") {
    esl::serial::UartStubDriver uart("UART_TEST");
    REQUIRE(uart.open());

    RxCapture cap;
    uart.setRxCallback(&captureCallback, &cap);

    const char* msg = "loopback";
    auto len = static_cast<std::size_t>(std::strlen(msg));
    CHECK(uart.write(reinterpret_cast<const std::uint8_t*>(msg), len) == len);

    CHECK(cap.callCount == 0);  // асинхронно: ничего не доставлено до poll()
    uart.poll();
    REQUIRE(cap.data.size() == len);
    CHECK(std::memcmp(cap.data.data(), msg, len) == 0);
}

TEST_CASE("UartStubDriver: write is rejected while the driver is closed") {
    esl::serial::UartStubDriver uart("UART_TEST");
    const std::uint8_t byte = 0x42;
    CHECK(uart.write(&byte, 1) == 0);
}

TEST_CASE("InMemoryLoopbackDriver: injectRxData simulates an external peer independent of loopback") {
    esl::serial::UartStubDriver uart("UART_TEST", /*loopbackEnabled=*/false);
    REQUIRE(uart.open());

    RxCapture cap;
    uart.setRxCallback(&captureCallback, &cap);

    const std::uint8_t peerData[] = {1, 2, 3, 4};
    uart.injectRxData(peerData, sizeof(peerData));
    uart.poll();

    REQUIRE(cap.data.size() == sizeof(peerData));
    CHECK(std::memcmp(cap.data.data(), peerData, sizeof(peerData)) == 0);

    // При выключенном loopback write() не должен попадать в rx.
    cap.data.clear();
    const std::uint8_t out[] = {9, 9};
    uart.write(out, sizeof(out));
    uart.poll();
    CHECK(cap.data.empty());
}

TEST_CASE("Rs485StubDriver: write is rejected while the transceiver is in receive mode") {
    esl::serial::Rs485StubDriver rs485("RS485_TEST");
    REQUIRE(rs485.open());

    const std::uint8_t byte = 0xAA;
    CHECK(rs485.write(&byte, 1) == 1);  // передача разрешена по умолчанию

    rs485.setDirection(false);
    CHECK(rs485.write(&byte, 1) == 0);

    rs485.setDirection(true);
    CHECK(rs485.write(&byte, 1) == 1);
}

TEST_CASE("UsbCdcStubDriver: open() fails until the device is connected") {
    esl::serial::UsbCdcStubDriver usb("USB_TEST");
    usb.setConnected(false);
    CHECK_FALSE(usb.open());
    CHECK_FALSE(usb.isOpen());

    usb.setConnected(true);
    CHECK(usb.open());
    CHECK(usb.isOpen());
}

TEST_CASE("UsbCdcStubDriver: disconnecting while open forces the driver closed") {
    esl::serial::UsbCdcStubDriver usb("USB_TEST");
    REQUIRE(usb.open());
    CHECK(usb.isOpen());

    usb.setConnected(false);
    CHECK_FALSE(usb.isOpen());
}
