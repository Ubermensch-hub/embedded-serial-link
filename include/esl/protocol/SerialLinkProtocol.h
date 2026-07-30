#pragma once

#include "esl/protocol/Frame.h"
#include "esl/serial/ISerialDriver.h"

namespace esl {
namespace protocol {

constexpr std::size_t kMinChannels = 1;
constexpr std::size_t kMaxChannels = 5;

using MessageCallback = void (*)(void* context, std::size_t channel, const std::uint8_t* payload,
                                  std::size_t payloadLen);

// Кадрирует и демультиплексирует сообщения на 1..5 независимых
// интерфейсах. У каждого канала свой FrameDecoder, поэтому каналы
// обрабатываются параллельно и независимо — битый байт на одном
// интерфейсе не влияет на остальные. Без хипа: драйверы и декодеры
// лежат в массивах фиксированного размера (kMaxChannels).
class SerialLinkProtocol {
public:
    // drivers указывает на channelCount указателей ISerialDriver;
    // channelCount ограничивается диапазоном [kMinChannels, kMaxChannels].
    SerialLinkProtocol(serial::ISerialDriver* const* drivers, std::size_t channelCount);

    // Открывает все настроенные драйверы и подключает rx-коллбэки
    bool start();

    // Опрашивает все драйверы, скармливает принятые байты декодеру
    // своего канала и вызывает коллбэк на каждый собранный кадр
    void poll();

    void setMessageCallback(MessageCallback callback, void* context);

    // Кадрирует payload и отправляет его в канал
    bool send(std::size_t channel, const std::uint8_t* payload, std::size_t payloadLen);

    std::size_t channelCount() const { return channelCount_; }

private:
    struct RxContext {
        SerialLinkProtocol* self;
        std::size_t channel;
    };

    static void onDriverRx(void* context, const std::uint8_t* data, std::size_t len);
    void handleRx(std::size_t channel, const std::uint8_t* data, std::size_t len);

    serial::ISerialDriver* drivers_[kMaxChannels];
    std::size_t channelCount_;
    FrameDecoder decoders_[kMaxChannels];
    RxContext rxContexts_[kMaxChannels];

    MessageCallback messageCallback_;
    void* messageContext_;
};

}  // namespace protocol
}  // namespace esl
