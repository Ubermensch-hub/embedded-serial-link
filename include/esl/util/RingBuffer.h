#pragma once

#include <cstddef>

namespace esl {
namespace util {

// Кольцевой буфер фиксированной ёмкости без динамической аллокации
template <typename T, std::size_t Capacity>
class RingBuffer {
public:
    static_assert(Capacity > 0, "RingBuffer capacity must be > 0");

    RingBuffer() : head_(0), tail_(0), size_(0) {}

    bool push(const T& value) {
        if (full()) {
            return false;
        }
        data_[tail_] = value;
        tail_ = advance(tail_);
        ++size_;
        return true;
    }

    bool pop(T& out) {
        if (empty()) {
            return false;
        }
        out = data_[head_];
        head_ = advance(head_);
        --size_;
        return true;
    }

    // Копирует сколько влезет из [data, data+len); возвращает число принятых байт.
    std::size_t pushBulk(const T* data, std::size_t len) {
        std::size_t accepted = 0;
        while (accepted < len && push(data[accepted])) {
            ++accepted;
        }
        return accepted;
    }

    // Извлекает до len элементов в out; возвращает число извлечённых.
    std::size_t popBulk(T* out, std::size_t len) {
        std::size_t popped = 0;
        while (popped < len && pop(out[popped])) {
            ++popped;
        }
        return popped;
    }

    void clear() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == Capacity; }
    std::size_t size() const { return size_; }
    static constexpr std::size_t capacity() { return Capacity; }

private:
    std::size_t advance(std::size_t index) const {
        ++index;
        return index == Capacity ? 0 : index;
    }

    T data_[Capacity];
    std::size_t head_;
    std::size_t tail_;
    std::size_t size_;
};

}  // namespace util
}  // namespace esl
