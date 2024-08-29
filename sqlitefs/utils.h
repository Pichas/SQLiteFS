#pragma once

#include <optional>
#include <spdlog/spdlog.h>

consteval std::size_t operator""_KiB(unsigned long long x) {
    return 1024ULL * x;
}

consteval std::size_t operator""_MiB(unsigned long long x) {
    return 1024_KiB * x;
}

consteval std::size_t operator""_GiB(unsigned long long x) {
    return 1024_MiB * x;
}

consteval std::size_t operator""_TiB(unsigned long long x) {
    return 1024_GiB * x;
}

consteval std::size_t operator""_PiB(unsigned long long x) {
    return 1024_TiB * x;
}


struct SQLExceptionCatcher {
    template<typename Func, class R = std::invoke_result_t<Func>>
    std::optional<R> operator+(Func&& function) && {
        try {
            return function();
        } catch (std::exception& e) { spdlog::critical("SQL Error: {} ", e.what()); }
        return {};
    }

    template<typename Func, class R = std::invoke_result_t<Func>>
    requires std::is_void_v<R>
    void operator+(Func&& function) && {
        try {
            function();
        } catch (std::exception& e) { spdlog::critical("SQL Error: {} ", e.what()); }
    }
};

#define TRY SQLExceptionCatcher() + [&]()