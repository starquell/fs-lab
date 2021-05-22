#pragma once

#include <vector>
#include <span>
#include <optional>
#include <string_view>

namespace fs {

    class IO {
    public:
        /**
         *  @brief Constructs IO with disk, where #ncyl is the number of cylinders, #ntracks is the number of tracks per cylinder,
         *         #nsectors is the number of sectors(physical blocks) per track and #sector_length is the number of bytes per sector
         */
        IO (std::size_t ncyl, std::size_t ntracks, std::size_t nsectors, std::size_t block_length);

        IO (std::size_t nblocks, std::size_t block_length);

        /**
         *  @brief Reads data from nth disk block to writes to #to
         *  @return number of bytes read
         */
        auto read_block(std::size_t n, std::span<std::byte> to) const -> std::size_t;

        /**
         * @brief Writes #bytes to nth disk block
         * @return number of bytes written
         */
        auto write_block(std::size_t n, std::span<const std::byte> bytes) -> std::size_t;

        [[nodiscard]]
        auto blocks_number() const noexcept -> std::size_t;

        [[nodiscard]]
        auto block_length() const noexcept -> std::size_t;

        auto save(std::string_view path) const -> void;
        static auto load(std::string_view path) -> std::optional<IO>;

    private:
        std::vector<std::vector<std::byte>> _disk;
    };

} // namespace fs
