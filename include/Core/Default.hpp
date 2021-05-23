#pragma once

#include <Core/Interface.hpp>
#include <IO.hpp>

#include <memory>
#include <deque>
#include <iterator>
#include <span>
#include <cstring>

namespace fs::core {

/**
 * @brief Default implementation of interface between FS and I/O.
 */
class Default : public Interface
{
public:
    /**
     * @brief Initialize with pointer to I/O system.
     */
    explicit Default(std::unique_ptr<IO> io);

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
    [[nodiscard]]
    auto create(Directory::index_type dir, const File& file) -> Directory::Entry::index_type override;

    /**
     * @brief Remove file from the directory.
     */
    void remove(Directory::index_type dir, Directory::Entry::index_type index) override;

    /**
     * @brief List all entries in directory.
     */
    [[nodiscard]]
    auto get(Directory::index_type dir) const -> std::optional<Directory> override;
protected:
    /**
     * @brief Get const reference to I/O pointer
     * @return const reference to I/O pointer
     */
    auto get_io() const noexcept -> const std::unique_ptr<IO>&;

    /**
     * @brief Initialize root directory
     */
    void init_root();

    struct IOPosition {
        std::size_t block;
        std::size_t byte;
    };

    /**
     * @brief Finds position of a structure satisfying the @a predicate in a sequence of blocks (TODO: concepts?)
     *
     * @param begin,end range of disc block indexes to examine
     * @param predicate unary predicate
     * @return position of a structure or nullopt if no such structure was found
     */
    template <class Type, class InputIt, class UnaryPredicate>
    auto find_value_on_disk_blocks_if(InputIt begin, InputIt end, UnaryPredicate predicate) const
        -> std::optional<IOPosition>;

    /**
     * @brief Writes the given @a value to a sequence
     *
     * @param value value to write
     * @param begin,end range of disc block indexes to examine
     * @param position position to write the @a value
     * @return number of bytes written
     */
    template <class Type, class InputIt>
    auto write_value_to_disk_blocks(Type value, InputIt begin, InputIt end, IOPosition position) -> std::size_t;

    /**
     * @brief Calculate number of blocks for metadata
     * @return number of metadata blocks
     */
    auto calculate_k() const -> std::size_t;
private:
    std::unique_ptr<IO> _io;                              ///< I/O system
    const std::size_t _k;                                 ///< Size of metadata (naming according to task)
    mutable std::vector<std::byte> _block_buffer;         ///< Buffer used to write/read to/from I/O
    std::vector<std::size_t> _descriptor_blocks_indexes;  ///< Indexes of blocks with descriptors
};

template <class Type, class InputIt, class UnaryPredicate>
auto Default::find_value_on_disk_blocks_if(InputIt begin, InputIt end, UnaryPredicate predicate) const
    -> std::optional<Default::IOPosition>
{
    static constexpr auto type_size = sizeof(Type);
    const std::size_t block_length = _io->block_length();

    std::deque<std::byte> accumulating_buffer;
    std::size_t current_offset = 0u;
    std::size_t blocks_read = 0u;
    std::size_t values_serialized = 0u;

    for (auto it = begin; it != end; ++it) {
        while (type_size <= accumulating_buffer.size()) {
            Type value{};
            memcpy(&value, &*accumulating_buffer.begin(), type_size); ///< pain

            if (predicate(value)) {
                return {IOPosition{
                    .block = (values_serialized * type_size) / block_length,
                    .byte = (values_serialized * type_size) % block_length}};
            }
            ++values_serialized;
            accumulating_buffer.erase(
                    accumulating_buffer.begin(),
                    accumulating_buffer.begin() + type_size);
        }
        accumulating_buffer.resize(accumulating_buffer.size() + block_length); ///< allocate memory for a new block
        _io->read_block(*it, std::span{_block_buffer}); ///< read a new block from disk
        accumulating_buffer.insert(std::back_inserter(accumulating_buffer), buffer.begin(), buffer.end()); ///< accumulate read buffer
        ++blocks_read;
    }

    return std::nullopt;
}

template <class Type, class InputIt>
auto Default::write_value_to_disk_blocks(Type value, InputIt begin, InputIt end, IOPosition position) -> std::size_t {
    if (end - begin < position.block + 1) { ///< invalid input data case
        return 0u;
    }

    static constexpr auto type_size = sizeof(Type);
    const std::size_t block_length = _io->block_length();

    std::vector<std::byte> serialized_value(type_size);
    memcpy(&*serialized_value.begin(), &value, type_size); ///< pain again

    std::size_t bytes_written = 0u;
    auto it = begin + position.block;
    _io->read_block(*it, std::span{_block_buffer});
    std::copy_n(serialized_value.begin(),
                std::min(serialized_value.size(), _block_buffer.size() - position.byte),
                _block_buffer.begin() + position.byte); ///< fill the first block of a slot for a value
    bytes_written += _io->write_block(*(it++), std::span{_block_buffer});
    while (it != end && serialized_value.size() > bytes_written) {
        _io->read_block(*it, std::span{_block_buffer});
        std::copy_n(serialized_value.begin() + bytes_written,
                    std::min(serialized_value.size() - bytes_written, _block_buffer.size()),
                    _block_buffer.begin()); ///< continue filling the tail of serialized value into its slot
        bytes_written += _io->write_block(*(it++), std::span{_block_buffer});
    }

    return bytes_written;
}
} // namespace fs::core
