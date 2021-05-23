#pragma once

#include <Core/Interface.hpp>
#include <Entity.hpp>

#include <unordered_map>
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
    [[nodiscard]]
    auto read(Directory::Entry::index_type index, std::span<std::byte> dst) const -> std::size_t;

    /**
     * @brief TODO:
     */
    [[nodiscard]]
    auto write(Directory::Entry::index_type index, std::span<const std::byte> src) -> std::size_t;

    /**
     * @brief TODO:
     */
    void lseek(Directory::Entry::index_type index, std::size_t pos);

    /**
     * @brief TODO:
     */
    [[nodiscard]]
    auto directory() const -> std::vector<File>;

    /**
     * @brief Save filesystem content for further restoring into specified file.
     */
    void save(std::string_view path);

private:
    core::Interface::Ptr _core;
    const Directory::index_type _cd = core::Interface::kRoot;
    mutable std::unordered_map<Directory::Entry::index_type, std::size_t> _oft;  // maps file indices to current positions in file
};

} // namespace fs
