#include "esl/protocol/Frame.h"

namespace esl {
namespace protocol {

std::uint8_t crc8Update(std::uint8_t crc, std::uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
        crc = (crc & 0x80) ? static_cast<std::uint8_t>((crc << 1) ^ 0x07) : static_cast<std::uint8_t>(crc << 1);
    }
    return crc;
}

std::uint8_t crc8(const std::uint8_t* data, std::size_t len) {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < len; ++i) {
        crc = crc8Update(crc, data[i]);
    }
    return crc;
}

namespace {

bool putStuffed(std::uint8_t raw, std::uint8_t* outBuffer, std::size_t outCapacity, std::size_t& pos) {
    if (raw == kSof || raw == kEof || raw == kEsc) {
        if (pos + 2 > outCapacity) {
            return false;
        }
        outBuffer[pos++] = kEsc;
        outBuffer[pos++] = static_cast<std::uint8_t>(raw ^ kEscXor);
    } else {
        if (pos + 1 > outCapacity) {
            return false;
        }
        outBuffer[pos++] = raw;
    }
    return true;
}

}  // namespace

std::size_t encodeFrame(const std::uint8_t* payload, std::size_t payloadLen, std::uint8_t* outBuffer,
                         std::size_t outBufferCapacity) {
    if (payloadLen > kMaxPayloadSize) {
        return 0;
    }

    std::uint8_t crc = crc8Update(0, static_cast<std::uint8_t>(payloadLen));
    for (std::size_t i = 0; i < payloadLen; ++i) {
        crc = crc8Update(crc, payload[i]);
    }

    std::size_t pos = 0;
    if (pos + 1 > outBufferCapacity) {
        return 0;
    }
    outBuffer[pos++] = kSof;

    if (!putStuffed(static_cast<std::uint8_t>(payloadLen), outBuffer, outBufferCapacity, pos)) {
        return 0;
    }
    for (std::size_t i = 0; i < payloadLen; ++i) {
        if (!putStuffed(payload[i], outBuffer, outBufferCapacity, pos)) {
            return 0;
        }
    }
    if (!putStuffed(crc, outBuffer, outBufferCapacity, pos)) {
        return 0;
    }

    if (pos + 1 > outBufferCapacity) {
        return 0;
    }
    outBuffer[pos++] = kEof;
    return pos;
}

FrameDecoder::FrameDecoder() : state_(State::kIdle), escapeNext_(false), expectedLen_(0), payloadLen_(0), crcAccum_(0) {}

void FrameDecoder::reset() {
    escapeNext_ = false;
    expectedLen_ = 0;
    payloadLen_ = 0;
    crcAccum_ = 0;
}

FrameDecoder::Result FrameDecoder::feed(std::uint8_t byte) {
    if (byte == kSof) {
        reset();
        state_ = State::kLen;
        return Result::kNone;
    }

    if (state_ == State::kIdle) {
        return Result::kNone;
    }

    if (byte == kEof && !escapeNext_) {
        if (state_ == State::kWaitEof) {
            state_ = State::kIdle;
            return Result::kFrameReady;
        }
        state_ = State::kIdle;
        return Result::kError;
    }

    std::uint8_t value = byte;
    if (escapeNext_) {
        value = static_cast<std::uint8_t>(byte ^ kEscXor);
        escapeNext_ = false;
    } else if (byte == kEsc) {
        escapeNext_ = true;
        return Result::kNone;
    }

    switch (state_) {
        case State::kLen:
            expectedLen_ = value;
            if (expectedLen_ > kMaxPayloadSize) {
                state_ = State::kIdle;
                return Result::kError;
            }
            payloadLen_ = 0;
            crcAccum_ = crc8Update(0, value);
            state_ = (expectedLen_ == 0) ? State::kCrc : State::kPayload;
            return Result::kNone;

        case State::kPayload:
            payload_[payloadLen_++] = value;
            crcAccum_ = crc8Update(crcAccum_, value);
            if (payloadLen_ == expectedLen_) {
                state_ = State::kCrc;
            }
            return Result::kNone;

        case State::kCrc:
            if (value != crcAccum_) {
                state_ = State::kIdle;
                return Result::kError;
            }
            state_ = State::kWaitEof;
            return Result::kNone;

        case State::kWaitEof:
            // Лишний байт вместо ожидаемого EOF: кадр битый, ресинк.
            state_ = State::kIdle;
            return Result::kError;

        default:
            state_ = State::kIdle;
            return Result::kError;
    }
}

}  // namespace protocol
}  // namespace esl
