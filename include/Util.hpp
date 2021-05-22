#pragma once

#include <string_view>
#include <utility>
#include <optional>
#include <array>
#include <limits>

namespace fs::util {
namespace detail {

    template<template<typename...> typename T, typename... Ts, typename F>
    constexpr void for_each_type_in(F && f, T<Ts...>*)
    {
        (std::forward<F>(f)(std::type_identity<Ts>{}), ...);
    }

} // namespace detail

/**
 * @brief Invoke functor for every type in a @tparam TypeList typelist.
 */
template <typename TypeList, typename F>
void for_each_type_in(F && f)
{
    detail::for_each_type_in(std::forward<F>(f), static_cast<TypeList*>(nullptr));
}

/**
 * @brief Split @a str by @a separator.
 */
template<typename OutIt>
constexpr auto split(OutIt output, 
                     std::string_view str,
                     const std::string_view separator, 
                     size_t max_parts = 0) -> size_t
{
    if (!max_parts) {
        max_parts = std::numeric_limits<size_t>::max();
    }

    size_t parts = 0;
    while (parts < max_parts && !str.empty()) {
        const auto pos = str.find(separator);
        if (pos != std::string_view::npos) {
            const auto tmp = str.substr(0, pos);
            if (!tmp.empty()) {
                *output++ = tmp;
                ++parts;
            }

            str.remove_prefix(pos + separator.length());
        } else {
            *output++ = str;
            str.remove_prefix(str.length());
            ++parts;
        }
    }

    return parts;
}

/**
 * @brief Split string @a str expecting exactly @tparam Size parts.
 */
template<size_t Size>
constexpr auto split_as_array(const std::string_view str, const std::string_view separator = " ") noexcept -> std::optional<std::array<std::string_view, Size>>
{
    std::array<std::string_view, Size> result;
    if (split(result.begin(), str, separator, result.size()) == result.size() && result.back().end() == str.end()) {
        return result;
    }

    return {};
}

} // namespace fs::util
