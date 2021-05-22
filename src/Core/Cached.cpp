#include <Core/Cached.hpp>

namespace {

    /// finds value in map in map, used for search in buffering cache
    template <typename Map, typename Key1, typename Key2>
    auto double_find(Map&& map, const Key1& key1, const Key2& key2) -> std::optional<typename std::remove_reference_t<Map>::mapped_type::mapped_type>
    {
        if (auto nested_map_it = map.find(key1); nested_map_it != map.end()) {
            if (auto it = nested_map_it->second.find(key2); it != nested_map_it->second.end()) {
                return it->second;
            }  else {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
    }
}
namespace fs::core {

Cached::Cached(std::unique_ptr<IO> io) noexcept :
    Default{std::move(io)}
{}

auto Cached::open(Directory::Entry::index_type index) -> Directory::Entry
{
    return Default::open(index);
}

void Cached::close(Directory::Entry::index_type index)
{
    _buffers.erase(index);
    Default::close(index);
}

auto Cached::read(Directory::Entry::index_type index, std::size_t pos, std::span<std::byte> dst) const -> std::size_t
{
    if (auto buf = double_find(_buffers, index, pos); buf.has_value()) {    // if buffered
         const auto bytes_read = std::min(dst.size(), buf->size());
         std::copy_n(buf->begin(), bytes_read, dst.begin());
         return bytes_read;
    }
    else {                                                                   // otherwise
        const auto bytes_read = Default::read(index, pos, dst);
        _buffers[index][pos] = std::vector(dst.begin(), dst.begin() + bytes_read);
        return bytes_read;
    }
}

auto Cached::write(Directory::Entry::index_type index, std::size_t pos, std::span<const std::byte> src) -> std::size_t
{
    const auto bytes_written = Default::write(index, pos, src);
    _buffers[index][pos] = std::vector(src.begin(), src.begin() + bytes_written);
    return bytes_written;
}

auto Cached::create(Directory::index_type dir, const File& file) -> Directory::Entry::index_type
{
    const auto res = Default::create(dir, file);
    if (_dir_been_listed[dir]) {    // caching in this block
        auto &cached_entries = _dir_cache[dir].entries;
        cached_entries.insert(
                std::upper_bound(cached_entries.begin(), cached_entries.end(), file,
                                 [&](const auto &lhs, const auto &rhs) { return lhs.name < rhs.name; }),
                Directory::Entry{file, res}
        );
    }
    return res;
}

void Cached::remove(Directory::index_type dir, Directory::Entry::index_type index)
{
    Default::remove(dir, index);
    _buffers.erase(index);
    if (_dir_been_listed[dir]) {  /// cleanup cache in this block
        auto &cached_entries = _dir_cache[dir].entries;
        cached_entries.erase(std::find_if(cached_entries.begin(), cached_entries.end(),
                                          [&](const auto &file) { return file.index == index; }));
    }
}

auto Cached::get(Directory::index_type dir) const -> std::optional<Directory>
{
    if (!_dir_been_listed[dir]) {
        std::optional directory = Default::get(dir);
        if (directory.has_value()) {
            _dir_cache[dir] = *directory;
            _dir_been_listed[dir] = true;
        }
        return directory;
    }
    else {
        return _dir_cache[dir];
    }
}


} // namespace fs::core
