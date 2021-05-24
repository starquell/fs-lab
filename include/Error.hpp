#pragma once

#include <fmt/format.h>
#include <stdexcept>

namespace fs {
/**
 * @brief Dedicated exception for errors occurred in filesystem functions.
 */
    struct Error final : std::runtime_error
    {
        template<typename F, typename... Args>
        explicit(sizeof...(Args) == 0) Error(F&& format, Args&&... args) :
                runtime_error{fmt::format(std::forward<F>(format), std::forward<Args>(args)...)}
        { }
    };
}
