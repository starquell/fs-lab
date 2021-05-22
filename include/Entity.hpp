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

    index_type index;
    std::vector<Entry> entries;
};

} // namespace fs
