#pragma once

#include <Core/Interface.hpp>
#include <Entity.hpp>
#include <Error.hpp>

#include <unordered_map>
#include <utility>
#include <span>

namespace fs {

/**
 * @brief Old good file system with UNIX-like interface.
 */
class Filesystem
{
public:
    using file_index_type = std::size_t;

    /**
     * @brief Construct filesystem from pointer to underlying interface.
     */
    explicit Filesystem(core::Interface::Ptr core) noexcept;

    /**
     * @brief Provide new underlying core interface. E. g. to update caching
     *        policy or to update underlying I/O system properties.
     */
    void update(core::Interface::Ptr core) noexcept;

    /**
     * @brief Creates file with name @a name
     */
    void create(std::string_view name);

    /**
     * @brief Removes file with name @a name
     */
    void destroy(std::string_view name);

    /**
     * @brief Opens file with name @a name
     * @return Index of opened file
     */
    [[nodiscard]]
    auto open(std::string_view name) -> file_index_type;

    /**
     * @brief Closes file with index @a index
     */
    void close(file_index_type index);

    /**
     * @brief Reads dst.size() bytes from file with index @a index to @a dst
     * @return Amount of read bytes (<= dst.size())
     */
    [[nodiscard]]
    auto read(file_index_type index, std::span<std::byte> dst) const -> std::size_t;

    /**
     * @brief Writes src.size() bytes from @a dst to file with index @a index
     * @return Amount of written bytes (<= src.size())
     */
    [[nodiscard]]
    auto write(file_index_type index, std::span<const std::byte> src) -> std::size_t;

    /**
     * @brief Changes current position to @a pos in file with index @a index
     */
    void lseek(file_index_type index, std::size_t pos);

    /**
     * @brief Returns all files in directory
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
