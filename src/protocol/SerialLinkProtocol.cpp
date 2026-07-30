#include "esl/protocol/SerialLinkProtocol.h"

#include "esl/util/Logger.h"

namespace esl {
namespace protocol {

namespace {
constexpr const char* kLogTag = "protocol";
}

SerialLinkProtocol::SerialLinkProtocol(serial::ISerialDriver* const* drivers, std::size_t channelCount)
    : channelCount_(0), messageCallback_(nullptr), messageContext_(nullptr) {
    if (channelCount < kMinChannels) {
        ESL_LOG_CALL(kLogTag, "channelCount %zu below minimum, clamped to %zu", channelCount, kMinChannels);
        channelCount = kMinChannels;
    } else if (channelCount > kMaxChannels) {
        ESL_LOG_CALL(kLogTag, "channelCount %zu above maximum, clamped to %zu", channelCount, kMaxChannels);
        channelCount = kMaxChannels;
    }
    channelCount_ = channelCount;

    for (std::size_t i = 0; i < channelCount_; ++i) {
        drivers_[i] = drivers[i];
    }
}

bool SerialLinkProtocol::start() {
    bool allOpened = true;
    for (std::size_t i = 0; i < channelCount_; ++i) {
        rxContexts_[i].self = this;
        rxContexts_[i].channel = i;

        drivers_[i]->setRxCallback(&SerialLinkProtocol::onDriverRx, &rxContexts_[i]);
        if (!drivers_[i]->open()) {
            ESL_LOG_CALL(kLogTag, "channel %zu (%s) failed to open", i, drivers_[i]->name());
            allOpened = false;
        }
    }
    return allOpened;
}

void SerialLinkProtocol::poll() {
    for (std::size_t i = 0; i < channelCount_; ++i) {
        drivers_[i]->poll();
    }
}

void SerialLinkProtocol::setMessageCallback(MessageCallback callback, void* context) {
    messageCallback_ = callback;
    messageContext_ = context;
}

bool SerialLinkProtocol::send(std::size_t channel, const std::uint8_t* payload, std::size_t payloadLen) {
    if (channel >= channelCount_) {
        ESL_LOG_CALL(kLogTag, "send() rejected: channel %zu out of range", channel);
        return false;
    }

    std::uint8_t frameBuffer[kMaxEncodedFrameSize];
    std::size_t encodedLen = encodeFrame(payload, payloadLen, frameBuffer, sizeof(frameBuffer));
    if (encodedLen == 0) {
        ESL_LOG_CALL(kLogTag, "send() rejected: payloadLen %zu invalid or exceeds capacity", payloadLen);
        return false;
    }

    std::size_t written = drivers_[channel]->write(frameBuffer, encodedLen);
    if (written != encodedLen) {
        ESL_LOG_CALL(kLogTag, "channel %zu: driver only accepted %zu/%zu encoded bytes", channel, written,
                     encodedLen);
        return false;
    }
    return true;
}

void SerialLinkProtocol::onDriverRx(void* context, const std::uint8_t* data, std::size_t len) {
    auto* ctx = static_cast<RxContext*>(context);
    ctx->self->handleRx(ctx->channel, data, len);
}

void SerialLinkProtocol::handleRx(std::size_t channel, const std::uint8_t* data, std::size_t len) {
    FrameDecoder& decoder = decoders_[channel];
    for (std::size_t i = 0; i < len; ++i) {
        FrameDecoder::Result result = decoder.feed(data[i]);
        if (result == FrameDecoder::Result::kFrameReady) {
            if (messageCallback_) {
                messageCallback_(messageContext_, channel, decoder.payload(), decoder.payloadLen());
            }
        } else if (result == FrameDecoder::Result::kError) {
            ESL_LOG_CALL(kLogTag, "channel %zu: malformed frame discarded, resynchronising", channel);
        }
    }
}

}  // namespace protocol
}  // namespace esl
