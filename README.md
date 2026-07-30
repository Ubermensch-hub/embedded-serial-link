# embedded-serial-link

Протокол передачи сообщений поверх 1–5 последовательных интерфейсов
(UART / RS485 / USB CDC) с заглушками драйверов, логированием вызовов и
integration-тестами. Код без динамической аллокации, собирается и запускается на хосте

## Быстрый старт

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/app/esl_demo_app     # демо: 4 интерфейса, сообщения разной длины
./build/tests/esl_tests      # 21 тест, 140+ проверок
```

## Содержание

- [Требования и как они закрыты](#требования-и-как-они-закрыты)
- [Архитектура](#архитектура)
- [Пример использования](#пример-использования)
- [Ключевые решения](#ключевые-решения)
- [Сборка на хосте](#сборка-на-хосте)
- [Кросс-сборка под STM32](#кросс-сборка-под-stm32)
- [Структура репозитория](#структура-репозитория)
- [Как добавить реальный драйвер](#как-добавить-реальный-драйвер)
- [Дальнейшие шаги](#дальнейшие-шаги)

## Требования и как они закрыты

| # | Требование | Реализация |
|---|---|---|
| 1 | Stub-драйверы UART/RS485/USB CDC | [`UartStubDriver`](include/esl/serial/UartStubDriver.h), [`Rs485StubDriver`](include/esl/serial/Rs485StubDriver.h), [`UsbCdcStubDriver`](include/esl/serial/UsbCdcStubDriver.h) поверх общей in-memory loopback-реализации |
| 2 | Логирование вызовов драйверов | [`esl::util::logCall`](include/esl/util/Logger.h) — вызывается в каждом публичном методе драйверов и протокола, sink подменяемый, отключается макросом `ESL_LOG_ENABLED=0` |
| 3 | Протокол на 1–5 интерфейсов | [`SerialLinkProtocol`](include/esl/protocol/SerialLinkProtocol.h), число каналов ограничивается диапазоном `[1,5]` |
| 4 | Устойчивость на serial-линиях | CRC8 + byte-stuffing служебных байт + автоматическая ресинхронизация по следующему SOF после любой ошибки — см. [`Frame.h`](include/esl/protocol/Frame.h) |
| 5 | Сообщения разной длины | Поле LEN (0..64 байт) во фрейме — длина не фиксирована, любое сообщение в этом диапазоне валидно |
| 6 | Архитектура, сборка и запуск | Слоистая архитектура ниже, `esl_demo_app` рабочее демо-приложение |
| 7 | I-Tests на абстрактных данных | [`tests/test_protocol_integration.cpp`](tests/test_protocol_integration.cpp) сквозные тесты `SerialLinkProtocol` поверх stub-драйверов |

## Архитектура

```
ISerialDriver (интерфейс)
        │
        ├── InMemoryLoopbackDriver (общая in-memory реализация: open/close,
        │        неблокирующий write, rx-очередь, callback, poll())
        │        │
        │        ├── UartStubDriver     — полный дуплекс без особенностей
        │        ├── Rs485StubDriver    — полудуплекс, DE/RE через setDirection()
        │        └── UsbCdcStubDriver   — open() требует "подключения" (enumeration)
        │
SerialLinkProtocol
        — держит до 5 ISerialDriver*, у каждого канала свой FrameDecoder
        — send(channel, payload, len): кадрирует и пишет в драйвер
        — poll(): опрашивает все драйверы, скармливает байты декодерам,
                  на готовый кадр — дёргает MessageCallback(channel, payload, len)
```

## Пример использования

```cpp
#include "esl/protocol/SerialLinkProtocol.h"
#include "esl/serial/UartStubDriver.h"

using namespace esl;

void onMessage(void*, std::size_t channel, const std::uint8_t* payload, std::size_t len) {
    // channel — индекс интерфейса, payload/len — тело сообщения
}

serial::UartStubDriver uart0("UART0");
serial::UartStubDriver uart1("UART1");
serial::ISerialDriver* drivers[] = {&uart0, &uart1};

protocol::SerialLinkProtocol link(drivers, 2);
link.setMessageCallback(&onMessage, nullptr);
link.start();

const std::uint8_t msg[] = "PING";
link.send(/*channel=*/0, msg, sizeof(msg) - 1);

while (true) {
    link.poll();   // на МК сюда попадает вызов из ISR/DMA
}
```

## Ключевые решения

- **Асинхронность без потоков и без хипа.** Интерфейс `ISerialDriver`
  callback-based: `write()` не блокирует, а `poll()` вычитывает то, что
  накопилось, и вызывает зарегистрированный коллбэк. На хосте `poll()`
  дёргается из прикладного цикла, на МК его роль займёт
  ISR/DMA-complete handler
- **Без динамической памяти.**  массивы фиксированного
  размера в `SerialLinkProtocol` и во `FrameDecoder` 
  Опция CMake `ESL_STRICT_EMBEDDED=ON` дополнительно собирает
  `esl_core` с `-fno-exceptions -fno-rtti` даже на хосте, чтобы можно было
  проверить это ограничение без ARM-тулчейна.
- **Framing.** `SOF | LEN | PAYLOAD | CRC8 | EOF` с байт-стаффингом
  (аналог PPP/HDLC), любое появление SOF/EOF/ESC внутри LEN/PAYLOAD/CRC
  экранируется, поэтому декодер всегда может пересинхронизироваться по
  следующему SOF, даже если предыдущий кадр был битым или оборванным.
  Это самый дешёвый по сложности способ закрыть требование к
  устойчивости, не городя ретраи/ACK поверх ненадёжного канала.
- **Параллельная обработка каналов.** У каждого канала свой независимый
  `FrameDecoder`, потеря синхронизации или битые данные на одном
  интерфейсе не влияют на остальные 
- **Stub-драйверы = in-memory loopback.** `write()` по умолчанию сразу
  попадает в собственную rx-очередь того же драйвера (эмуляция шлейфа).
  Для интеграционных тестов, где нужно сымитировать данные от внешнего
  устройства, есть `injectRxData()` (и `setLoopbackEnabled(false)`, чтобы
  не мешал self-echo).

## Сборка на хосте

Требуется CMake ≥ 3.16 и компилятор с поддержкой C++14 (проверено на
GCC 13.1.0/Ninja).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Запуск демо-приложения (4 интерфейса: 2×UART, RS485, USB CDC; шлёт и
принимает сообщения длиной от 2 до 60 байт):

```bash
./build/app/esl_demo_app
```

Запуск тестов:

```bash
./build/tests/esl_tests
# или через ctest:
ctest --test-dir build
```

Опции CMake:

| Опция | По умолчанию | Назначение |
|---|---|---|
| `ESL_BUILD_APP` | `ON` | Собирать `esl_demo_app` |
| `ESL_BUILD_TESTS` | `ON` | Собирать `esl_tests` (doctest) |
| `ESL_STRICT_EMBEDDED` | `OFF` | Собирать `esl_core` с `-fno-exceptions -fno-rtti` |

doctest подтягивается через `FetchContent` в `third_party/doctest` при
первой конфигурации. Для офлайн-сборки достаточно вручную
положить `doctest.h` по пути `third_party/doctest/doctest/doctest.h`.

## Кросс-сборка под STM32

`esl_core` не использует ничего host-специфичного (in-memory
loopback-драйверы, демо-приложение и тесты не собираются в кросс-сборке —
они завязаны на stdio):

```bash
cmake -S . -B build-stm32 -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
      -DESL_BUILD_APP=OFF -DESL_BUILD_TESTS=OFF
cmake --build build-stm32
```

Требует установленный `arm-none-eabi-gcc`; в этой репе он не проверялся, но код ядра сознательно
написан так, чтобы кросс-сборка была вопросом добавления реального
UART/RS485/USB-CDC драйвера под `ISerialDriver`, а не переписывания
протокола.

## Структура репозитория

```
include/esl/        — публичные заголовки (serial, protocol, util)
src/                 — реализация esl_core
app/main.cpp         — демо-приложение
tests/               — doctest I-tests
third_party/         — doctest (fetch-only, в git не попадает)
cmake/               — toolchain-файл для STM32
```

