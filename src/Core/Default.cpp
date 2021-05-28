#include <Core/Default.hpp>
#include <climits>
#include <array>
#include <numeric>
#include <cstddef>
#include <optional>

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

auto count_free_bits(std::span<const std::byte> bitmap, std::size_t max_bits) -> std::size_t {
    std::size_t free_bits = 0u;
    for (std::size_t i = 0u; i < std::min(bitmap.size() * CHAR_BIT, max_bits); ++i) {
        if (!get_bit(bitmap, i)) {
            ++free_bits;
        }
    }
    return free_bits;
}

struct UtilityStruct {
    bool is_occupied = true;
};

struct Descriptor : UtilityStruct {
    static constexpr std::size_t max_blocks_for_file = 3u;
    std::size_t length = 0u;
    std::array<std::size_t, max_blocks_for_file> blocks;

    [[nodiscard]]
    auto blocks_allocated(std::size_t block_size) const noexcept -> std::size_t {
        return (length + block_size - 1) / block_size;
    }

    [[nodiscard]]
    auto free_bytes(std::size_t block_size, std::size_t pos) const noexcept -> std::size_t {
        return blocks_allocated(block_size) * block_size - pos;
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
    , _descriptor_blocks_indexes(_k-1)
{
    std::iota(_descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(), first_descriptor_block); // set indexes of descriptor blocks
    init_root();
}

auto Default::block_length() const noexcept -> std::size_t {
    return _io->block_length();
}

void Default::init_root() {
    if (write_value_to_disk_blocks(
            Descriptor{},
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            IOPosition{.block = 0u, .byte = 0u})
        < sizeof(Descriptor))
    {
        throw Error{"not enough disk space to initialize root directory"};
    }
}

auto Default::calculate_k() const -> std::size_t {
    const auto block_length_to_descriptor_size_ratio =
            static_cast<double>(_io->block_length()) / static_cast<double>(sizeof(Descriptor));
    const auto disk_blocks = _io->blocks_number();

    if (disk_blocks <= 3) {
        throw Error{"I/O system has not enough logic blocks: at least 4 needed, got {}", disk_blocks};
    }

    const std::size_t k = static_cast<std::size_t>(
            (static_cast<double>(disk_blocks) - static_cast<double>(Descriptor::max_blocks_for_file)
                + block_length_to_descriptor_size_ratio) /
            (1. + block_length_to_descriptor_size_ratio)
    );

    if (k < 2 || _io->blocks_number() - k < 2) {
        throw Error{"I/O system has unusable parameters"};
    }

    return k;
}

auto Default::data_blocks_count() const noexcept -> std::size_t {
    return _io->blocks_number() - _k;
}

auto Default::IOPosition::fromIndex(std::size_t index, std::size_t block_length) noexcept -> IOPosition {
    return {.block = index / block_length,
            .byte = index % block_length};
}

auto Default::allocate_blocks(std::span<std::size_t> blocks_ref, std::size_t blocks_allocated,
                              std::size_t blocks_to_allocate, std::size_t block_length) -> std::size_t
{
    std::size_t current_block_index = blocks_allocated;
    for (std::size_t i = 0u;
         i < std::min(block_length * CHAR_BIT, data_blocks_count())
            && current_block_index < std::min(
                 blocks_allocated + blocks_to_allocate,
                 blocks_ref.size());
         ++i)
    {
        if (!get_bit(_block_buffer, i)) { // free block found
            blocks_ref[current_block_index++] = _k + i;
            set_bit(_block_buffer, i, true);
        }
    }
    return current_block_index - blocks_allocated;
}

auto Default::create(Directory::index_type dir, const File &file) -> Directory::Entry::index_type {
    if (std::size_t actual = file.name.size(), max = DirectoryEntry::max_filename_length; actual > max) {
        throw Error{"filename is too long: maximal length is {} symbols, but given is {} symbols", max, actual};
    }
    const auto free_descriptor_pos = find_value_on_disk_blocks_if<Descriptor>(
            _descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(),
            [](const auto& descriptor) {
                return !descriptor.is_occupied;
            }); // find a free descriptor
    if (!free_descriptor_pos) {
        throw Error{"not enough space to create file"};
    }

    const auto directory_position = IOPosition{.block = 0, .byte = 0};
    auto directory_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(),
            directory_position).value();

    const auto block_length = _io->block_length();
    std::size_t bytes_processed = 0u;
    auto free_entry_slot = find_value_on_disk_blocks_if<DirectoryEntry>(
            directory_descriptor.blocks.begin(),
            directory_descriptor.blocks.begin() + directory_descriptor.blocks_allocated(block_length),
            [&bytes_processed, length = directory_descriptor.length](const auto& directory_entry) {
                if (bytes_processed < length && !directory_entry.is_occupied) {
                    return true;
                }
                bytes_processed += sizeof(directory_entry);
                return false;
            }); // trying to find free slot position

    if (!free_entry_slot) { // trying to allocate new blocks
        const auto blocks_allocated = directory_descriptor.blocks_allocated(block_length);
        const auto bytes_available = directory_descriptor.free_bytes(block_length, directory_descriptor.length);
        const auto blocks_to_allocate = ((sizeof(Descriptor) - bytes_available) + block_length - 1) / block_length;
        if (blocks_to_allocate + blocks_allocated > directory_descriptor.blocks.size()) {
            throw Error("not enough space in directory to create a new file");
        }

        _io->read_block(bitmap_block_number, _block_buffer); // reading bitmap in main memory
        if (blocks_to_allocate <= count_free_bits(_block_buffer, data_blocks_count())) { // allocate new blocks in a cached bitmap
            allocate_blocks(directory_descriptor.blocks, blocks_allocated, blocks_to_allocate, block_length);
        } else {
            throw Error("not enough space on disk to create a new file");
        }
        _io->write_block(bitmap_block_number, _block_buffer); // writing updated bitmap


        free_entry_slot.emplace(IOPosition::fromIndex(directory_descriptor.length, block_length));

        directory_descriptor.length += sizeof(DirectoryEntry); // append directory entry to directory file
    }

    const auto descriptor_index =
            (free_descriptor_pos.value().block * block_length + free_descriptor_pos.value().byte) / sizeof(Descriptor);

    auto directory_entry = DirectoryEntry{
        .name_length = file.name.size(),
        .descriptor_index = descriptor_index
    }; // new directory entry

    std::copy(file.name.begin(), file.name.end(), directory_entry.name.begin()); // set filename to a directory entry

    write_value_to_disk_blocks(directory_entry,
                               directory_descriptor.blocks.begin(),
                               directory_descriptor.blocks.begin() + directory_descriptor.blocks_allocated(block_length),
                               free_entry_slot.value()); // write a new file entry

    write_value_to_disk_blocks(directory_descriptor,
                               _descriptor_blocks_indexes.begin(),
                               _descriptor_blocks_indexes.end(),
                               directory_position); // update directory descriptor

    write_value_to_disk_blocks(Descriptor{},
                               _descriptor_blocks_indexes.begin(),
                               _descriptor_blocks_indexes.end(),
                               free_descriptor_pos.value()); // write a new descriptor

    return descriptor_index;
}

auto Default::search(Directory::index_type dir, std::string_view name)
    const -> std::optional<Directory::Entry::index_type>
{
    const auto directory_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            IOPosition{.block = 0u, .byte = 0u}).value();

    std::optional<Directory::Entry::index_type> found_index{};

    find_value_on_disk_blocks_if<DirectoryEntry>(
            directory_descriptor.blocks.begin(),
            directory_descriptor.blocks.end(),
            [&found_index, name](const auto& entry) {
                if (entry.is_occupied
                    && std::equal(
                        entry.name.begin(), entry.name.begin() + entry.name_length,
                        name.begin(),       name.end())) {
                    found_index = entry.descriptor_index;
                    return true;
                }
                return false;
            }
        );

    return found_index;
}

void Default::remove(Directory::index_type dir, Directory::Entry::index_type index) {
    const auto directory_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            IOPosition{.block = 0u, .byte = 0u}).value();

    const auto block_length = _io->block_length();

    const auto entry_position = find_value_on_disk_blocks_if<DirectoryEntry>(
            directory_descriptor.blocks.begin(),
            directory_descriptor.blocks.begin() + directory_descriptor.blocks_allocated(block_length),
            [index](const auto& entry) {
                return entry.is_occupied && entry.descriptor_index == index;
            }).value(); // get file entry position

    const auto descriptor_position = IOPosition::fromIndex(index * sizeof(Descriptor), block_length);
    const auto descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            descriptor_position).value(); // read file descriptor

    _io->read_block(bitmap_block_number, _block_buffer); // read bitmap
    for (auto it = descriptor.blocks.begin();
            it != descriptor.blocks.begin() + descriptor.blocks_allocated(block_length); ++it) // free bitmap entries
    {
        set_bit(_block_buffer, *it - _k, false);
    }
    _io->write_block(bitmap_block_number, _block_buffer); // write updated bitmap

    write_value_to_disk_blocks(
            DirectoryEntry{{.is_occupied = false}},
            directory_descriptor.blocks.begin(),
            directory_descriptor.blocks.begin() + directory_descriptor.blocks_allocated(block_length),
            entry_position); // remove directory entry

    write_value_to_disk_blocks(
            Descriptor{{.is_occupied = false}},
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            descriptor_position); // free the descriptor
}

auto Default::open(Directory::Entry::index_type index) -> Directory::Entry {
    // no additional logic on file opening
    return {};
}

void Default::close(Directory::Entry::index_type index) {
    // no additional logic on file closing
}

auto Default::write(Directory::Entry::index_type index, std::size_t pos,
                    std::span<const std::byte> src) -> std::size_t
{
    const auto block_length = _io->block_length();
    const auto descriptor_pos = IOPosition::fromIndex(index * sizeof(Descriptor), block_length);
    auto entry_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            descriptor_pos).value();

    std::size_t new_blocks_allocated = 0u;
    if (entry_descriptor.free_bytes(block_length, pos) < src.size()) { // allocating new blocks
        const auto blocks_allocated = entry_descriptor.blocks_allocated(block_length);
        const auto bytes_available = entry_descriptor.free_bytes(block_length, pos);
        const auto blocks_to_allocate = (src.size() - bytes_available) + block_length / block_length;

        _io->read_block(bitmap_block_number, _block_buffer); // read bitmap from disk
        new_blocks_allocated = allocate_blocks(
                entry_descriptor.blocks,
                blocks_allocated,
                blocks_to_allocate,
                block_length); // allocating as much blocks as possible
        _io->write_block(bitmap_block_number, _block_buffer); // write bitmap to disk
    }

    entry_descriptor.length = std::min(
            (entry_descriptor.blocks_allocated(block_length) + new_blocks_allocated) * block_length,
            entry_descriptor.length + src.size()); // extending file length

    write_value_to_disk_blocks(
            entry_descriptor,
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            descriptor_pos); // write updated descriptor

    return write_bytes_to_disk_blocks(
            src,
            entry_descriptor.blocks.begin(),
            entry_descriptor.blocks.begin() + entry_descriptor.blocks_allocated(block_length),
            IOPosition::fromIndex(pos, block_length));
}

auto Default::read(Directory::Entry::index_type index, std::size_t pos, std::span<std::byte> dst) const -> std::size_t {
    const auto block_length = _io->block_length();
    const auto entry_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            IOPosition::fromIndex(index * sizeof(Descriptor), block_length)).value();

    if (pos >= entry_descriptor.length) {
        return 0u;
    }

    return read_bytes_from_disk_blocks(
            std::span(dst.begin(), dst.begin() + std::min(dst.size(), entry_descriptor.length - pos)),
            entry_descriptor.blocks.begin(),
            entry_descriptor.blocks.begin() + entry_descriptor.blocks_allocated(block_length),
            IOPosition::fromIndex(pos, block_length));
}

auto Default::get(Directory::index_type dir) const -> std::optional<Directory> {
    auto directory = std::optional{Directory{.index = dir}};
    const auto directory_descriptor = read_value_from_disk_blocks<Descriptor>(
            _descriptor_blocks_indexes.begin(),
            _descriptor_blocks_indexes.end(),
            IOPosition{.block = 0u, .byte = 0u}).value(); // read directory descriptor

    find_value_on_disk_blocks_if<DirectoryEntry>(
            directory_descriptor.blocks.begin(),
            directory_descriptor.blocks.begin() + directory_descriptor.blocks_allocated(_io->block_length()),
            [&directory](const auto& entry) {
                if (entry.is_occupied) {
                    directory->entries.push_back({
                            0u,
                            std::string{entry.name.begin(), entry.name.begin() + entry.name_length},
                            static_cast<Directory::index_type>(entry.descriptor_index)}); // add a new file entry
                }
                return false;
            });

    for (auto& entry : directory->entries) {
        entry.size = read_value_from_disk_blocks<Descriptor>(
                _descriptor_blocks_indexes.begin(),
                _descriptor_blocks_indexes.end(),
                IOPosition::fromIndex(entry.index * sizeof(Descriptor), _io->block_length())
            )->length; // get file length from descriptor
    }

    return directory;
}

void Default::save(const std::string_view path) const
{
    _io->save(path);
}

} // namespace fs::core
