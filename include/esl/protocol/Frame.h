#pragma once

#include <cstddef>
#include <cstdint>

// Формат кадра SerialLinkProtocol.
// До экранирования: LEN(1) | PAYLOAD(0..kMaxPayloadSize) | CRC8(1)
// На линии:         SOF | stuff(LEN|PAYLOAD|CRC8) | EOF
// Байты SOF/EOF/ESC внутри кадра экранируются (ESC + byte^kEscXor), поэтому
// декодер всегда может пересинхронизироваться по следующему SOF после
// битого или оборванного кадра. Вместе с проверкой CRC8 это и есть
// механизм устойчивости к ошибкам на линии. Поле LEN — переменное.
namespace esl {
namespace protocol {

constexpr std::uint8_t kSof = 0x7E; //Start of frame
constexpr std::uint8_t kEof = 0x7F; //End of frame
constexpr std::uint8_t kEsc = 0x7D;
constexpr std::uint8_t kEscXor = 0x20;

constexpr std::size_t kMaxPayloadSize = 64;

// Худший случай: SOF + EOF + все байты (len + payload + crc) экранированы.
constexpr std::size_t kMaxEncodedFrameSize = 2 + 2 * (1 + kMaxPayloadSize + 1);

std::uint8_t crc8Update(std::uint8_t crc, std::uint8_t byte);
std::uint8_t crc8(const std::uint8_t* data, std::size_t len);

// Кодирует payload в кадр (экранирование + CRC + разделители)
std::size_t encodeFrame(const std::uint8_t* payload, std::size_t payloadLen, std::uint8_t* outBuffer,
                         std::size_t outBufferCapacity);

// Пошаговый декодер байтового потока одного канала. Без аллокаций,
// буфер под payload — фиксированного размера внутри объекта.
class FrameDecoder {
public:
    enum class Result {
        kNone,        // байт принят, кадр ещё не собран
        kFrameReady,  // кадр собран и прошёл проверку CRC
        kError        // битый кадр отброшен
    };

    FrameDecoder();

    Result feed(std::uint8_t byte);

    const std::uint8_t* payload() const { return payload_; }
    std::size_t payloadLen() const { return payloadLen_; }

private:
    enum class State { kIdle, kLen, kPayload, kCrc, kWaitEof };

    void reset();

    State state_;
    bool escapeNext_;
    std::uint8_t expectedLen_;
    std::uint8_t payload_[kMaxPayloadSize];
    std::size_t payloadLen_;
    std::uint8_t crcAccum_;
};

}  // namespace protocol
}  // namespace esl
