#include <IO.hpp>

#include <algorithm>
#include <fstream>

fs::IO::IO(std::size_t nblocks, std::size_t block_length)
    : _disk(nblocks, std::vector<std::byte>(block_length))
{}

auto fs::IO::get_block(std::size_t n) const -> std::span<const std::byte> {
    return {_disk[n].begin(), _disk[n].size()};
}

auto fs::IO::read_block(std::size_t n, std::span<std::byte> to) const -> std::size_t {
    const auto bytes_read = std::min(_disk[n].size(), to.size());
    std::copy_n(_disk[n].begin(), bytes_read, to.begin());
    return bytes_read;
}

auto fs::IO::write_block(std::size_t n, std::span<const std::byte> bytes) -> std::size_t {
    const auto bytes_written = std::min(_disk[n].size(), bytes.size());
    std::copy_n(bytes.begin(), bytes_written, _disk[n].begin());
    return bytes_written;
}

auto fs::IO::blocks_number() const noexcept -> std::size_t {
    return _disk.size();
}

auto fs::IO::block_length() const noexcept -> std::size_t {
    return _disk.front().size();
}

void fs::save(const fs::IO& io, std::string_view path)
{
    std::ofstream file{path.data(), std::ostream::binary | std::ostream::trunc};
    auto nblocks = static_cast<uint64_t>(io.blocks_number());
    auto block_length = static_cast<uint64_t>(io.block_length());
    file.write(reinterpret_cast<const char*>(&nblocks), sizeof(nblocks));
    file.write(reinterpret_cast<const char*>(&block_length), sizeof(block_length));
    for (std::size_t i = 0; i < nblocks; ++i) {
        file.write(reinterpret_cast<const char*>(io.get_block(i).data()), sizeof(block_length));
    }
}

auto fs::loadIO(std::string_view path) -> fs::IO {
    std::ifstream file{path.data(), std::ifstream::binary};
    auto nblocks = [&] {
        uint64_t n;
        file.read(reinterpret_cast<char*>(&n), sizeof(n));
        return static_cast<std::size_t>(n);
    }();
    auto block_length = [&] {
        uint64_t n;
        file.read(reinterpret_cast<char*>(&n), sizeof(n));
        return static_cast<std::size_t>(n);
    }();
    fs::IO io(nblocks, block_length);
    for (std::size_t i = 0; i < nblocks; ++i) {
        std::vector<std::byte> bytes(block_length);
        file.read(reinterpret_cast<char*>(bytes.data()), sizeof(block_length));
        io.write_block(i, bytes);
    }
    return io;
}
