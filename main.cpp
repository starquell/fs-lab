#include <iostream>

#include <IO.hpp>

auto main() -> int
{
    fs::IO io{10, 10};
    std::string_view a = "ABOBA";
    for (std::size_t i = 0; i < io.blocks_number(); ++i) {
        io.write_block(i, as_bytes(std::span{a}));
    };

    std::vector<std::vector<char>> vec(io.blocks_number(), std::vector<char>(io.block_length()));
    for (std::size_t i = 0; i < io.blocks_number(); ++i) {
        io.read_block(i, as_writable_bytes(std::span{vec[i]}));
    }

    return 0;
}