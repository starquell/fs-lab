#pragma once

#include <Entity.hpp>

#include <optional>
#include <memory>
#include <vector>
#include <span>

namespace fs::core {

/**
 * @brief Interface for I/O <-> FS communication.
 */
struct Interface
{
    /**
     * @brief Index of implicit root directory.
     */
    static constexpr Directory::index_type kRoot = 0;

    /**
     * @brief Pointer to interface.
     */
    using Ptr = std::unique_ptr<Interface>;

    /**
     * @brief Virtual destructor, as required.
     */
    virtual ~Interface() = default;

    /**
     * @brief Read data into @a dst start from provided @a pos.
     */
    [[nodiscard]]
    virtual auto read(Directory::Entry::index_type index, 
                      std::size_t pos, 
                      std::span<std::byte> dst) const -> std::size_t = 0;

    /**
     * @brief Write data from @a dst starting at position @a pos.
     */
    [[nodiscard]]
    virtual auto write(Directory::Entry::index_type index, 
                       std::size_t pos, 
                       std::span<const std::byte> src) -> std::size_t = 0;

    /**
     * @brief Create new file in directory.
     */
    [[nodiscard]]
    virtual auto create(Directory::index_type dir, const File& file) -> Directory::Entry::index_type = 0;

    /**
     * @brief Remove file from the directory.
     */
    virtual void remove(Directory::index_type dir, Directory::Entry::index_type index) = 0;

    /**
     * @brief List all entries in directory.
     */
    [[nodiscard]]
    virtual auto get(Directory::index_type dir) const -> std::optional<Directory> = 0;
};

} // namespace fs::core