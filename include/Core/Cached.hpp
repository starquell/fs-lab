#pragma once

#include <Core/Default.hpp>
#include <memory>
#include <unordered_map>

namespace fs::core {

/**
 * @brief Implementation of communication with I/O subsystem that
 *        caches some data.
 */
class Cached final : public Default
{
public:
    /**
     * @brief Initialize with pointer to I/O system.
     */
    explicit Cached(std::unique_ptr<IO> io) noexcept;

    /**
     * @brief Close file and possibly free all associated resources.
     */
    void close(Directory::Entry::index_type index) override;

    /**
     * @brief Read data into @a dst start from provided @a pos.
     */
    [[nodiscard]]
    auto read(Directory::Entry::index_type index,
              std::size_t pos,
              std::span<std::byte> dst) const -> std::size_t override;

    /**
     * @brief Write data from @a dst starting at position @a pos.
     */
    [[nodiscard]]
    auto write(Directory::Entry::index_type index,
               std::size_t pos,
               std::span<const std::byte> src) -> std::size_t override;

    /**
     * @brief Create new file in directory.
     */
    auto create(Directory::index_type dir, const File& file) -> Directory::Entry::index_type override;

    [[nodiscard]]
    auto search(Directory::index_type dir, std::string_view name) const
     -> std::optional<Directory::Entry::index_type> override;

    /**
     * @brief Remove file from the directory.
     */
    void remove(Directory::index_type dir, Directory::Entry::index_type index) override;

    /**
     * @brief List all entries in directory sorted by name.
     */
    [[nodiscard]]
    auto get(Directory::index_type dir) const -> std::optional<Directory> override;


private:
    mutable std::unordered_map<Directory::index_type, Directory> _dir_cache;
    mutable std::unordered_map<Directory::Entry::index_type, std::pair<Directory::index_type, std::string>> _entry_info_cache;  // used for updating file`s sizes

    struct Buffer {
        std::size_t buf_start_pos;
        std::vector<std::byte> data;
    };
    mutable std::unordered_map<Directory::Entry::index_type, Buffer> _buffers;
};

} // namespace fs::core
