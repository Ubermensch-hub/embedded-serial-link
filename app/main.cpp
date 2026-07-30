// Демо: подключает 4 stub-интерфейса (2×UART, RS485, USB CDC) к
// SerialLinkProtocol, отправляет по каждому сообщение своей длины и
// показывает, как loopback-драйверы доставляют их обратно через poll().

#include <cstdio>
#include <cstring>

#include "esl/protocol/SerialLinkProtocol.h"
#include "esl/serial/Rs485StubDriver.h"
#include "esl/serial/UartStubDriver.h"
#include "esl/serial/UsbCdcStubDriver.h"

namespace {

void onMessage(void* /*context*/, std::size_t channel, const std::uint8_t* payload, std::size_t len) {
    std::printf("[app] channel %zu received %zu-byte message: \"", channel, len);
    for (std::size_t i = 0; i < len; ++i) {
        std::putchar(static_cast<char>(payload[i]));
    }
    std::printf("\"\n");
}

bool sendText(esl::protocol::SerialLinkProtocol& protocol, std::size_t channel, const char* text) {
    auto len = static_cast<std::size_t>(std::strlen(text));
    bool ok = protocol.send(channel, reinterpret_cast<const std::uint8_t*>(text), len);
    std::printf("[app] send on channel %zu (%zu bytes): %s\n", channel, len, ok ? "queued" : "FAILED");
    return ok;
}

}  // namespace

int main() {
    esl::serial::UartStubDriver uart0("UART0");
    esl::serial::UartStubDriver uart1("UART1");
    esl::serial::Rs485StubDriver rs485("RS485");
    esl::serial::UsbCdcStubDriver usbCdc("USB_CDC");

    esl::serial::ISerialDriver* drivers[] = {&uart0, &uart1, &rs485, &usbCdc};
    const std::size_t channelCount = sizeof(drivers) / sizeof(drivers[0]);

    esl::protocol::SerialLinkProtocol protocol(drivers, channelCount);
    protocol.setMessageCallback(&onMessage, nullptr);

    if (!protocol.start()) {
        std::fprintf(stderr, "[app] one or more interfaces failed to open\n");
        return 1;
    }

    sendText(protocol, 0, "PING");
    sendText(protocol, 1, "The quick brown fox jumps over the lazy dog");
    sendText(protocol, 2, "OK");
    sendText(protocol, 3, "USB CDC virtual COM port test message with some length to it");

    // Прогоняет loopback-драйверы, чтобы отправленные сообщения дошли до
    // onMessage(). На реальном железе это обычный главный цикл приложения,
    // обслуживающий байты по мере прихода прерываний/DMA.
    for (int i = 0; i < 4; ++i) {
        protocol.poll();
    }

    return 0;
}
