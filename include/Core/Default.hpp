#pragma once

#include <Core/Interface.hpp>
#include <IO.hpp>
#include <Error.hpp>

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
     * @brief Open file for further work.
     */
    auto open(Directory::Entry::index_type index) -> Directory::Entry override;

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

    /**
     * @brief Search file by name in directory.
     */
    [[nodiscard]]
    auto search(Directory::index_type dir, std::string_view name) const
        -> std::optional<Directory::Entry::index_type> override;

    /**
     * @brief Remove file from the directory.
     */
    void remove(Directory::index_type dir, Directory::Entry::index_type index) override;

    /**
     * @brief List all entries in directory.
     */
    [[nodiscard]]
    auto get(Directory::index_type dir) const -> std::optional<Directory> override;

    /**
     * @brief Save content for further restoring into specified file.
     */
    void save(std::string_view path) const final;

protected:
    [[nodiscard]]
    auto block_length() const noexcept -> std::size_t;

    /**
     * @brief Initialize root directory
     */
    void init_root();

    struct IOPosition {
        std::size_t block;
        std::size_t byte;

        static auto fromIndex(std::size_t index, std::size_t block_length) noexcept -> IOPosition;
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
     * @brief Reads @a bytes from a sequence of blocks
     *
     * @param value value to read
     * @param begin,end range of disc block indexes to examine
     * @param position position to read the @a value
     * @return number of bytes read
     */
    template <class InputIt>
    auto read_bytes_from_disk_blocks(std::span<std::byte> bytes, InputIt begin, InputIt end, IOPosition position) const
    -> std::size_t;

    /**
     * @brief Reads value from a sequence of blocks
     *
     * @param begin,end range of disc block indexes to examine
     * @param position position to read the @a value
     * @return read value if read completed successfully, nullopt if not
     */
    template <class Type, class InputIt>
    auto read_value_from_disk_blocks(InputIt begin, InputIt end, IOPosition position) const -> std::optional<Type>;

    /**
     * @brief Writes @a bytes to a sequence of blocks
     *
     * @param bytes bytes to write
     * @param begin,end range of disc block indexes to examine
     * @param position position to write @a bytes
     * @return number of bytes written
     */
    template <class InputIt>
    auto write_bytes_to_disk_blocks(std::span<const std::byte> bytes, InputIt begin, InputIt end, IOPosition position)
        -> std::size_t;

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
     * @brief allocate new blocks
     * @param blocks_ref container with block references
     * @param blocks_allocated number of allocated blocks
     * @param blocks_to_allocate number of blocks to be allocated
     * @param block_length length in bytes of one block
     * @return number of blocks allocated
     */
    auto allocate_blocks(
            std::span<std::size_t> blocks_ref,
            std::size_t blocks_allocated,
            std::size_t blocks_to_allocate,
            std::size_t block_length) -> std::size_t;
    /**
     * @brief Calculate number of blocks for metadata
     * @return number of metadata blocks
     */
    auto calculate_k() const -> std::size_t;
private:
    std::unique_ptr<IO> _io;                              // I/O system
    const std::size_t _k;                                 // Size of metadata (naming according to task)
    mutable std::vector<std::byte> _block_buffer;         // Buffer used to write/read to/from I/O
    std::vector<std::size_t> _descriptor_blocks_indexes;  // Indexes of blocks with descriptors
};

template <class Type, class InputIt, class UnaryPredicate>
auto Default::find_value_on_disk_blocks_if(InputIt begin, InputIt end, UnaryPredicate predicate) const
    -> std::optional<Default::IOPosition>
{
    static constexpr auto type_size = sizeof(Type);
    const std::size_t block_length = _io->block_length();

    std::deque<std::byte> accumulating_buffer;
    std::size_t values_serialized = 0u;

    for (auto it = begin; it != end; ++it) {
        _io->read_block(*it, _block_buffer); // read a new block from disk
        std::copy(
                _block_buffer.begin(), _block_buffer.end(), std::back_inserter(accumulating_buffer)); // accumulate read buffer

        while (type_size <= accumulating_buffer.size()) {
            Type value{};
            memcpy(&value, &*accumulating_buffer.begin(), type_size); // pain

            if (predicate(value)) {
                return {IOPosition::fromIndex(values_serialized * type_size, block_length)};
            }
            ++values_serialized;
            accumulating_buffer.erase(
                    accumulating_buffer.begin(),
                    accumulating_buffer.begin() + type_size);
        }
    }

    return std::nullopt;
}

template <class InputIt>
auto Default::read_bytes_from_disk_blocks(
        std::span<std::byte> bytes, InputIt begin, InputIt end, IOPosition position) const -> std::size_t
{
    if (end - begin < position.block + 1) { // invalid input data case
        return 0u;
    }

    auto it = begin + position.block;
    _io->read_block(*(it++), _block_buffer);
    std::size_t bytes_read = std::min(bytes.size(), _block_buffer.size() - position.byte);

    std::copy_n(_block_buffer.begin() + position.byte,
                bytes_read,
                bytes.begin()); // put bytes from a first read block

    while (it != end && bytes.size() > bytes_read) {
        _io->read_block(*(it++), _block_buffer);
        std::size_t bytes_to_read = std::min(bytes.size() - bytes_read, _block_buffer.size());
        std::copy_n(_block_buffer.begin(),
                    bytes_to_read,
                    bytes.begin() + bytes_read); // continue filling the tail of bytes from read block
        bytes_read += bytes_to_read;
    }
    return bytes_read;
}

template <class Type, class InputIt>
auto Default::read_value_from_disk_blocks(InputIt begin, InputIt end, Default::IOPosition position) const
        -> std::optional<Type>
{
    static constexpr auto type_size = sizeof(Type);
    std::vector<std::byte> serialized_value(type_size);

    const auto bytes_read = read_bytes_from_disk_blocks(serialized_value, begin, end, position);

    if (bytes_read < type_size) {
        return std::nullopt;
    }

    Type value{};
    memcpy(&value, &*serialized_value.begin(), type_size); // pain again
    return {value};
}

template <class InputIt>
auto Default::write_bytes_to_disk_blocks(std::span<const std::byte> bytes, InputIt begin, InputIt end, Default::IOPosition position) -> std::size_t {
    if (end - begin < position.block + 1) { // invalid input data case
        return 0u;
    }

    auto it = begin + position.block;
    _io->read_block(*it, _block_buffer);
    std::size_t bytes_written = std::min(bytes.size(), _block_buffer.size() - position.byte);
    std::copy_n(bytes.begin(),
                bytes_written,
                _block_buffer.begin() + position.byte); // fill the first block of a slot for a value
    _io->write_block(*(it++), std::span{_block_buffer});

    while (it != end && bytes.size() > bytes_written) {
        _io->read_block(*it, _block_buffer);
        const auto bytes_to_rewrite = std::min(bytes.size() - bytes_written, _block_buffer.size());
        std::copy_n(bytes.begin() + bytes_written,
                    bytes_to_rewrite,
                    _block_buffer.begin()); // continue filling the tail of serialized value into its slot
        bytes_written += bytes_to_rewrite;
        _io->write_block(*(it++), _block_buffer);
    }

    return bytes_written;
}

template <class Type, class InputIt>
auto Default::write_value_to_disk_blocks(Type value, InputIt begin, InputIt end, Default::IOPosition position)
    -> std::size_t
{
    if (end - begin < position.block + 1) { // invalid input data case
        return 0u;
    }

    static constexpr auto type_size = sizeof(Type);

    std::vector<std::byte> serialized_value(type_size);
    memcpy(&*serialized_value.begin(), &value, type_size); // and again

    return write_bytes_to_disk_blocks(serialized_value, begin, end, position);
}
} // namespace fs::core
