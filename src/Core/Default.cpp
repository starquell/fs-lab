#include <Core/Default.hpp>
#include <climits>
#include <array>
#include <numeric>

namespace fs::core {
namespace {

constexpr std::size_t bitmap_block_number = 0u;
constexpr std::size_t first_descriptor_block = bitmap_block_number + 1;

void set_bit(std::span<std::byte> bitmap, std::size_t index, bool value) {
    const auto block_num = index / CHAR_BIT;
    const auto in_block_position = index % CHAR_BIT;
    const auto bitmask = (std::byte{1} << (CHAR_BIT - 1 - in_block_position));
    if (value) {
        bitmap[block_num] |= bitmask;
    } else {
        bitmap[block_num] &= (bitmask ^ std::byte{0});
    }
}

auto get_bit(std::span<const std::byte> bitmap, std::size_t index) -> bool {
    const auto block_num = index / CHAR_BIT;
    const auto in_block_position = index % CHAR_BIT;
    const auto bitmask = std::byte{1};

    return static_cast<bool>((bitmap[block_num] >> (CHAR_BIT - 1 - in_block_position)) & bitmask);
}

struct UtilityStruct {
    bool is_free = false;
};

struct Descriptor : UtilityStruct {
    std::size_t length = 0u;
    std::array<std::size_t, 3> blocks;

    auto blocks_allocated(std::size_t block_size) const noexcept -> std::size_t {
        return (length + block_size - 1) / block_size;
    }
};

struct DirectoryEntry : UtilityStruct {
    static constexpr std::size_t max_filename_length = 20u;

    std::array<char, max_filename_length> name;
    std::size_t name_length;
    std::size_t descriptor_index = 0u;
};

} // namespace

Default::Default(std::unique_ptr<IO> io)
    : _io{std::move(io)}
    , _k(calculate_k())
    , _block_buffer(_io->block_length())
    , _descriptor_blocks_indexes(_k-1) // TODO: add _k calculating
{
    std::iota(_descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(), first_descriptor_block); // set indexes of descriptor blocks
    init_root();
}

void Default::init_root() {
    if (write_value_to_disk_blocks(
            Descriptor{},
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            IOPosition{})
        < sizeof(Descriptor))
    {
        throw Error("not enough disk space to initialize root directory");
    }
}

auto Default::calculate_k() const -> std::size_t {
    return _io->blocks_number() / 5; // TODO: implement formula
}

auto Default::create(Directory::index_type dir, const File &file) -> Directory::Entry::index_type {
    const auto free_descriptor_pos = find_value_on_disk_blocks_if<Descriptor>(
            _descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(),
            [](const auto& descriptor) {
                return descriptor.is_free;
            }); // find a free descriptor

    auto directory_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(),
            IOPosition{.block = 0, .byte = 0}).value();

    const auto block_length = _io->block_length();
    const auto free_entry_slot = find_value_on_disk_blocks_if<DirectoryEntry>(
            directory_descriptor.blocks.begin(),
            directory_descriptor.blocks.begin() + directory_descriptor.blocks_allocated(block_length),
            [length = directory_descriptor.length](const auto& directory_entry) {
                static size_t bytes_processed = 0u;
                if (bytes_processed < length && directory_entry.is_free) {
                    return true;
                }
                bytes_processed += sizeof(directory_entry);
                return false;
            }); // trying to find free slot position

    if (!free_entry_slot) { // trying to allocate new blocks
        const auto blocks_allocated = directory_descriptor.blocks_allocated(block_length);
        const auto bytes_available =
                blocks_allocated * block_length
                - directory_descriptor.length;
        const auto blocks_to_allocate = (sizeof(Descriptor) - bytes_available) + block_length / block_length;
        if (blocks_to_allocate + blocks_allocated > directory_descriptor.blocks.size()) {
            throw Error("not enough space to create file");
        }

        _io->read_block(bitmap_block_number, std::span{_block_buffer}); // reading bitmap in main memory
        for (std::size_t i = 0u, current_block_index = blocks_allocated;
            i < block_length && current_block_index < directory_descriptor.blocks.size(); ++i)
        {
            if (!get_bit(std::span{_block_buffer}, i)) { // free block found
                directory_descriptor.blocks[current_block_index++] = i;
                set_bit(std::span{_block_buffer}, i, true);
            }
        }

        _io->write_block(bitmap_block_number, std::span{_block_buffer}); // writing updated bitmap

        free_entry_slot.emplace(.block = directory_descriptor.length / block_length,
                                .byte = directory_descriptor.length % block_length);

        directory_descriptor.length += sizeof(DirectoryEntry); // append directory entry to directory file
    }

    // TODO: writing a new file entry
}

void Default::remove(Directory::index_type dir, Directory::Entry::index_type index) {

}

auto Default::write(Directory::Entry::index_type index, std::size_t pos,
                    std::span<const std::byte> src) -> std::size_t {

}

auto Default::read(Directory::Entry::index_type index, std::size_t pos, std::span<std::byte> dst) const -> std::size_t {

}

auto Default::get(Directory::index_type dir) const -> std::optional<Directory> {

}

void Default::save(const std::string_view path) const
{
    _io->save(path);
}

} // namespace fs::core
