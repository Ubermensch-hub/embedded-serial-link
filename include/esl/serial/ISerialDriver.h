#pragma once

#include <cstddef>
#include <cstdint>

namespace esl {
namespace serial {

using RxCallback = void (*)(void* context, const std::uint8_t* data, std::size_t len);

// Общий интерфейс для последовательных интерфейсов
// Интерфейс неблокирующий и асинхронный. write() только ставит байты в
// очередь, а принятые байты доставляются через коллбэк из setRxCallback()
// во время poll(). На реальном МК роль poll() играет прерывание/DMA,
// вызывающее тот же коллбэк;
class ISerialDriver {
public:
    virtual ~ISerialDriver() = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // Ставит до len байт на отправку
    virtual std::size_t write(const std::uint8_t* data, std::size_t len) = 0;

    // Регистрирует колбэк на приём данных
    virtual void setRxCallback(RxCallback callback, void* context) = 0;

    // Доставляет накопленные принятые байты в колбэк.
    virtual void poll() = 0;

    virtual const char* name() const = 0;
};

}  // namespace serial
}  // namespace esl
