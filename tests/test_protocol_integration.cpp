// Интеграционные тесты SerialLinkProtocol: проверяют весь путь
// encode -> драйвер -> decode -> колбэк на 1..5 stub-интерфейсах

#include <cstring>
#include <vector>

#include "doctest/doctest.h"
#include "esl/protocol/SerialLinkProtocol.h"
#include "esl/serial/UartStubDriver.h"

using esl::protocol::SerialLinkProtocol;
using esl::serial::UartStubDriver;

namespace {

struct ReceivedMessage {
    std::size_t channel;
    std::vector<std::uint8_t> payload;
};

struct MessageCapture {
    std::vector<ReceivedMessage> messages;
};

void captureMessage(void* context, std::size_t channel, const std::uint8_t* payload, std::size_t len) {
    auto* cap = static_cast<MessageCapture*>(context);
    cap->messages.push_back(ReceivedMessage{channel, std::vector<std::uint8_t>(payload, payload + len)});
}

std::vector<std::uint8_t> toBytes(const char* text) {
    return std::vector<std::uint8_t>(text, text + std::strlen(text));
}

}  // namespace

TEST_CASE("SerialLinkProtocol: end-to-end send/receive round trip on a single channel") {
    UartStubDriver uart("UART0");
    esl::serial::ISerialDriver* drivers[] = {&uart};
    SerialLinkProtocol protocol(drivers, 1);

    MessageCapture cap;
    protocol.setMessageCallback(&captureMessage, &cap);
    REQUIRE(protocol.start());

    auto payload = toBytes("integration-test");
    CHECK(protocol.send(0, payload.data(), payload.size()));

    protocol.poll();

    REQUIRE(cap.messages.size() == 1);
    CHECK(cap.messages[0].channel == 0);
    CHECK(cap.messages[0].payload == payload);
}

TEST_CASE("SerialLinkProtocol: 5 channels process independent, different-length messages in one poll sweep") {
    UartStubDriver d0("CH0");
    UartStubDriver d1("CH1");
    UartStubDriver d2("CH2");
    UartStubDriver d3("CH3");
    UartStubDriver d4("CH4");
    esl::serial::ISerialDriver* drivers[] = {&d0, &d1, &d2, &d3, &d4};
    SerialLinkProtocol protocol(drivers, 5);
    REQUIRE(protocol.channelCount() == 5);

    MessageCapture cap;
    protocol.setMessageCallback(&captureMessage, &cap);
    REQUIRE(protocol.start());

    std::vector<std::vector<std::uint8_t>> payloads = {
        toBytes("a"),
        toBytes("bb"),
        toBytes("ccc-message"),
        toBytes(""),
        toBytes("a somewhat longer message on the last channel"),
    };

    for (std::size_t ch = 0; ch < payloads.size(); ++ch) {
        CHECK(protocol.send(ch, payloads[ch].data(), payloads[ch].size()));
    }

    protocol.poll();

    REQUIRE(cap.messages.size() == payloads.size());
    for (const auto& msg : cap.messages) {
        REQUIRE(msg.channel < payloads.size());
        CHECK(msg.payload == payloads[msg.channel]);
    }
}

TEST_CASE("SerialLinkProtocol: a corrupted frame on one channel does not affect other channels") {
    UartStubDriver good("GOOD");
    UartStubDriver bad("BAD");
    esl::serial::ISerialDriver* drivers[] = {&good, &bad};
    SerialLinkProtocol protocol(drivers, 2);

    MessageCapture cap;
    protocol.setMessageCallback(&captureMessage, &cap);
    REQUIRE(protocol.start());

    // Канал 1 получает помехи от "битого" устройства: правдоподобный
    // заголовок SOF/LEN, мусор и преждевременный EOF без валидного CRC.
    const std::uint8_t noise[] = {0x7E, 0x05, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x7F};
    bad.setLoopbackEnabled(false);
    bad.injectRxData(noise, sizeof(noise));

    // Канал 0 отправляет нормальное сообщение через настоящий кодер.
    auto payload = toBytes("still fine");
    CHECK(protocol.send(0, payload.data(), payload.size()));

    protocol.poll();

    REQUIRE(cap.messages.size() == 1);
    CHECK(cap.messages[0].channel == 0);
    CHECK(cap.messages[0].payload == payload);
}

TEST_CASE("SerialLinkProtocol: channel count is clamped into [1,5]") {
    UartStubDriver d0("D0");
    UartStubDriver d1("D1");
    UartStubDriver d2("D2");
    UartStubDriver d3("D3");
    UartStubDriver d4("D4");
    UartStubDriver d5("D5");
    UartStubDriver d6("D6");
    esl::serial::ISerialDriver* many[] = {&d0, &d1, &d2, &d3, &d4, &d5, &d6};

    SerialLinkProtocol tooMany(many, 7);
    CHECK(tooMany.channelCount() == 5);

    esl::serial::ISerialDriver* none[] = {&d0};
    SerialLinkProtocol zero(none, 0);
    CHECK(zero.channelCount() == 1);
}
