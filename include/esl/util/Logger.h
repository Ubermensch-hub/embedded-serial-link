#pragma once

// Лёгкое логирование вызовов драйверов
// Без хипа, исключений и std::string, сообщение форматируется в буфер на
// стеке (vsnprintf) и передаётся в подменяемый sink, на реальном железе
// это будет отладочный UART вместо stdio. Полностью отключается
// флагом -DESL_LOG_ENABLED=0 для релизной прошивки.

#ifndef ESL_LOG_ENABLED
#define ESL_LOG_ENABLED 1
#endif

namespace esl {
namespace util {

using LogSink = void (*)(const char* message);

// Устанавливает свой sink (запись в отладочный UART).
// nullptr возвращает sink по умолчанию (вывод в stderr).
void setLogSink(LogSink sink);

// Формирует "[component] message" и передаёт в текущий sink.
void logCall(const char* component, const char* format, ...);

}  // namespace util
}  // namespace esl

#if ESL_LOG_ENABLED
#define ESL_LOG_CALL(component, ...) ::esl::util::logCall(component, __VA_ARGS__)
#else
#define ESL_LOG_CALL(component, ...) \
    do {                             \
    } while (0)
#endif
