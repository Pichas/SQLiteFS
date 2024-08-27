#pragma once

#include <optional>
#include <spdlog/spdlog.h>


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