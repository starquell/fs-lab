#include <Core/Default.hpp>
#include <climits>
#include <array>
#include <numeric>

namespace fs::core {
namespace {

constexpr std::size_t bitmap_block_number = 0u;
constexpr std::size_t first_descriptor_block = bitmap_block_number + 1;

constexpr std::size_t max_filename_length = 20u;

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
};

struct DirectoryEntry : UtilityStruct {
    std::array<char, max_filename_length> name;
    std::size_t descriptor_index = 0u;
};

} // namespace

Default::Default(std::unique_ptr<IO> io)
    : _io{std::move(io)}
    , _k(calculate_k())
    , _block_buffer(_io->block_length())
    , _descriptor_blocks_indexes(_k-1) // TODO: add _k calculating
{
    std::iota(_descriptor_blocks_indexes.begin(), _descriptor_blocks_indexes.end(), first_descriptor_block); ///< set indexes of descriptor blocks
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
        throw std::runtime_error("not enough disk space to initialize root directory");
    }
}

auto Default::calculate_k() const -> std::size_t {
    return _io->blocks_number() / 5; // TODO: implement formula
}

auto Default::create(Directory::index_type dir, const File &file) -> Directory::Entry::index_type {

}

void Default::remove(Directory::index_type dir, Directory::Entry::index_type index) {

}

auto Default::write(Directory::Entry::index_type index, std::size_t pos,
                    std::span<const std::byte> src) -> std::size_t {

}

auto Default::read(Directory::Entry::index_type index, std::size_t pos, std::span<std::byte> dst) const {

}





} // namespace fs::core
