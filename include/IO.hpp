#pragma once

#include <vector>
#include <span>
#include <string_view>

namespace fs {

    class IO {
    public:
        IO (std::size_t nblocks, std::size_t block_length);

        /**
             * @brief Returns span of nth disk block
         */
        [[nodiscard]]
        auto get_block(std::size_t n) const -> std::span<const std::byte>;

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

    private:
        std::vector<std::vector<std::byte>> _disk;
    };

    auto save(const IO& io, std::string_view path) -> void;
    auto loadIO(std::string_view path) -> IO;

}