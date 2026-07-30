#include "esl/protocol/Frame.h"

#include <cstring>

#include "doctest/doctest.h"

using namespace esl::protocol;

namespace {

// Пропускает закодированный кадр через новый декодер, возвращает true при
// успешном декодировании и заполняет out/outLen данными payload.
bool decodeOne(const std::uint8_t* encoded, std::size_t encodedLen, std::uint8_t* out, std::size_t& outLen) {
    FrameDecoder decoder;
    for (std::size_t i = 0; i < encodedLen; ++i) {
        FrameDecoder::Result r = decoder.feed(encoded[i]);
        if (r == FrameDecoder::Result::kFrameReady) {
            outLen = decoder.payloadLen();
            std::memcpy(out, decoder.payload(), outLen);
            return true;
        }
        if (r == FrameDecoder::Result::kError) {
            return false;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("Frame: encode/decode round trip for a typical message") {
    const char* text = "hello protocol";
    auto len = static_cast<std::size_t>(std::strlen(text));

    std::uint8_t encoded[kMaxEncodedFrameSize];
    std::size_t encodedLen = encodeFrame(reinterpret_cast<const std::uint8_t*>(text), len, encoded, sizeof(encoded));
    REQUIRE(encodedLen > 0);
    CHECK(encoded[0] == kSof);
    CHECK(encoded[encodedLen - 1] == kEof);

    std::uint8_t decoded[kMaxPayloadSize];
    std::size_t decodedLen = 0;
    REQUIRE(decodeOne(encoded, encodedLen, decoded, decodedLen));
    REQUIRE(decodedLen == len);
    CHECK(std::memcmp(decoded, text, len) == 0);
}

TEST_CASE("Frame: zero-length and max-length payloads round-trip") {
    std::uint8_t encoded[kMaxEncodedFrameSize];
    std::uint8_t decoded[kMaxPayloadSize];
    std::size_t decodedLen = 0;

    // сообщение нулевой длины
    std::size_t encodedLen = encodeFrame(nullptr, 0, encoded, sizeof(encoded));
    REQUIRE(encodedLen > 0);
    REQUIRE(decodeOne(encoded, encodedLen, decoded, decodedLen));
    CHECK(decodedLen == 0);

    // сообщение максимальной длины
    std::uint8_t maxPayload[kMaxPayloadSize];
    for (std::size_t i = 0; i < kMaxPayloadSize; ++i) {
        maxPayload[i] = static_cast<std::uint8_t>(i);
    }
    encodedLen = encodeFrame(maxPayload, kMaxPayloadSize, encoded, sizeof(encoded));
    REQUIRE(encodedLen > 0);
    REQUIRE(decodeOne(encoded, encodedLen, decoded, decodedLen));
    REQUIRE(decodedLen == kMaxPayloadSize);
    CHECK(std::memcmp(decoded, maxPayload, kMaxPayloadSize) == 0);
}

TEST_CASE("Frame: oversized payload is rejected by the encoder") {
    std::uint8_t payload[kMaxPayloadSize + 1] = {};
    std::uint8_t encoded[kMaxEncodedFrameSize];
    CHECK(encodeFrame(payload, sizeof(payload), encoded, sizeof(encoded)) == 0);
}

TEST_CASE("Frame: payload containing reserved bytes is escaped and decodes back exactly") {
    std::uint8_t payload[] = {kSof, kEof, kEsc, 0x00, 0xFF, kSof, kEsc};
    std::uint8_t encoded[kMaxEncodedFrameSize];
    std::size_t encodedLen = encodeFrame(payload, sizeof(payload), encoded, sizeof(encoded));
    REQUIRE(encodedLen > 0);

    // Служебные байты не должны встречаться неэкранированными внутри кадра.
    for (std::size_t i = 1; i < encodedLen - 1; ++i) {
        if (encoded[i] == kSof || encoded[i] == kEof) {
            FAIL("внутри кадра найден неэкранированный служебный байт");
        }
    }

    std::uint8_t decoded[kMaxPayloadSize];
    std::size_t decodedLen = 0;
    REQUIRE(decodeOne(encoded, encodedLen, decoded, decodedLen));
    REQUIRE(decodedLen == sizeof(payload));
    CHECK(std::memcmp(decoded, payload, sizeof(payload)) == 0);
}

TEST_CASE("Frame: corrupted CRC is rejected") {
    const char* text = "abc";
    std::uint8_t encoded[kMaxEncodedFrameSize];
    std::size_t encodedLen =
        encodeFrame(reinterpret_cast<const std::uint8_t*>(text), 3, encoded, sizeof(encoded));
    REQUIRE(encodedLen > 0);

    // Портим бит в payload (не в разделителях), чтобы сломать проверку CRC.
    encoded[2] ^= 0x01;

    std::uint8_t decoded[kMaxPayloadSize];
    std::size_t decodedLen = 0;
    CHECK_FALSE(decodeOne(encoded, encodedLen, decoded, decodedLen));
}

TEST_CASE("Frame: decoder resynchronises on the next SOF after noise/garbage") {
    const char* text = "resync-me";
    auto len = static_cast<std::size_t>(std::strlen(text));
    std::uint8_t encoded[kMaxEncodedFrameSize];
    std::size_t encodedLen = encodeFrame(reinterpret_cast<const std::uint8_t*>(text), len, encoded, sizeof(encoded));
    REQUIRE(encodedLen > 0);

    FrameDecoder decoder;
    // Помехи на линии перед валидным кадром.
    const std::uint8_t noise[] = {0x11, 0x22, 0x33, 0x44};
    for (std::uint8_t b : noise) {
        CHECK(decoder.feed(b) != FrameDecoder::Result::kFrameReady);
    }

    bool gotFrame = false;
    for (std::size_t i = 0; i < encodedLen; ++i) {
        if (decoder.feed(encoded[i]) == FrameDecoder::Result::kFrameReady) {
            gotFrame = true;
        }
    }
    REQUIRE(gotFrame);
    REQUIRE(decoder.payloadLen() == len);
    CHECK(std::memcmp(decoder.payload(), text, len) == 0);
}

TEST_CASE("Frame: two consecutive frames on the same stream both decode") {
    std::uint8_t encodedA[kMaxEncodedFrameSize];
    std::uint8_t encodedB[kMaxEncodedFrameSize];
    std::size_t lenA = encodeFrame(reinterpret_cast<const std::uint8_t*>("first"), 5, encodedA, sizeof(encodedA));
    std::size_t lenB = encodeFrame(reinterpret_cast<const std::uint8_t*>("second-msg"), 10, encodedB, sizeof(encodedB));
    REQUIRE(lenA > 0);
    REQUIRE(lenB > 0);

    FrameDecoder decoder;
    int framesReceived = 0;
    std::size_t lastLen = 0;
    std::uint8_t lastPayload[kMaxPayloadSize] = {};

    auto feedAll = [&](const std::uint8_t* buf, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) {
            if (decoder.feed(buf[i]) == FrameDecoder::Result::kFrameReady) {
                ++framesReceived;
                lastLen = decoder.payloadLen();
                std::memcpy(lastPayload, decoder.payload(), lastLen);
            }
        }
    };

    feedAll(encodedA, lenA);
    CHECK(framesReceived == 1);
    feedAll(encodedB, lenB);
    CHECK(framesReceived == 2);
    REQUIRE(lastLen == 10);
    CHECK(std::memcmp(lastPayload, "second-msg", 10) == 0);
}
