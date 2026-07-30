#include "esl/util/RingBuffer.h"

#include <cstdint>

#include "doctest/doctest.h"

using esl::util::RingBuffer;

TEST_CASE("RingBuffer: push/pop preserves FIFO order") {
    RingBuffer<int, 4> buf;
    CHECK(buf.push(1));
    CHECK(buf.push(2));
    CHECK(buf.push(3));

    int value = 0;
    REQUIRE(buf.pop(value));
    CHECK(value == 1);
    REQUIRE(buf.pop(value));
    CHECK(value == 2);
}

TEST_CASE("RingBuffer: rejects push when full, empty pop fails") {
    RingBuffer<int, 2> buf;
    CHECK(buf.push(10));
    CHECK(buf.push(20));
    CHECK_FALSE(buf.push(30));
    CHECK(buf.full());

    int value = 0;
    REQUIRE(buf.pop(value));
    REQUIRE(buf.pop(value));
    CHECK(buf.empty());
    CHECK_FALSE(buf.pop(value));
}

TEST_CASE("RingBuffer: wraps around correctly across many cycles") {
    RingBuffer<int, 3> buf;
    for (int cycle = 0; cycle < 5; ++cycle) {
        CHECK(buf.push(cycle * 10));
        CHECK(buf.push(cycle * 10 + 1));
        int a = 0;
        int b = 0;
        REQUIRE(buf.pop(a));
        REQUIRE(buf.pop(b));
        CHECK(a == cycle * 10);
        CHECK(b == cycle * 10 + 1);
        CHECK(buf.empty());
    }
}

TEST_CASE("RingBuffer: pushBulk/popBulk handle partial capacity") {
    RingBuffer<std::uint8_t, 4> buf;
    std::uint8_t in[6] = {1, 2, 3, 4, 5, 6};
    std::size_t accepted = buf.pushBulk(in, 6);
    CHECK(accepted == 4);
    CHECK(buf.full());

    std::uint8_t out[6] = {};
    std::size_t popped = buf.popBulk(out, 6);
    CHECK(popped == 4);
    CHECK(out[0] == 1);
    CHECK(out[3] == 4);
}
