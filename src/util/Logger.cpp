#include "esl/util/Logger.h"

#include <cstdarg>
#include <cstdio>

namespace esl {
namespace util {

namespace {

void defaultSink(const char* message) { std::fprintf(stderr, "%s\n", message); }

LogSink g_sink = &defaultSink;

}  // namespace

void setLogSink(LogSink sink) { g_sink = sink ? sink : &defaultSink; }

void logCall(const char* component, const char* format, ...) {
    char buffer[160];
    int prefixLen = std::snprintf(buffer, sizeof(buffer), "[%s] ", component);
    if (prefixLen < 0) {
        return;
    }
    if (static_cast<std::size_t>(prefixLen) >= sizeof(buffer)) {
        prefixLen = static_cast<int>(sizeof(buffer)) - 1;
    }

    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer + prefixLen, sizeof(buffer) - static_cast<std::size_t>(prefixLen), format, args);
    va_end(args);

    g_sink(buffer);
}

}  // namespace util
}  // namespace esl
