#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fs {

struct File
{
    std::size_t size;
    std::string name;
};

struct Directory : File
{
    using index_type = std::uint32_t;

    struct Entry : File
    {
        using index_type = std::uint32_t;

        index_type index;
    };

    static constexpr index_type index = 0;
    std::vector<Entry> entries;
};

} // namespace fs
