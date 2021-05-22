#pragma once

#include <Core/Interface.hpp>
#include <Entity.hpp>

#include <fmt/format.h>
#include <stdexcept>
#include <utility>
#include <span>

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

/**
 * @brief Old good file system with UNIX-like interface.
 */
class Filesystem
{
public:
    /**
     * @brief Construct filesystem from pointer to underlying interface.
     */
    explicit Filesystem(core::Interface::Ptr core) noexcept;

    /**
     * @brief Provide new underlying core interace. E. g. to update caching 
     *        policy or to update underlying I/O system properties.
     */
    void update(core::Interface::Ptr core) noexcept;

    /**
     * @brief TODO:
     */
    void create(std::string_view name);

    /**
     * @brief TODO:
     */
    void destroy(std::string_view name);

    /**
     * @brief TODO:
     */
    [[nodiscard]]
    auto open(std::string_view name) -> Directory::Entry::index_type;

    /**
     * @brief TODO:
     */
    void close(Directory::Entry::index_type index);

    /**
     * @brief TODO:
     */
    void read(Directory::Entry::index_type index, std::span<std::byte> dst);

    /**
     * @brief TODO:
     */
    void write(Directory::Entry::index_type index, std::span<const std::byte> src);

    /**
     * @brief TODO:
     */
    void lseek(Directory::Entry::index_type index, std::size_t pos);

    /**
     * @brief TODO:
     */
    [[nodiscard]]
    auto directory() const -> std::vector<File>;

private:
    core::Interface::Ptr _core;
    const Directory::index_type _cd = core::Interface::kRoot;
};

} // namespace fs
