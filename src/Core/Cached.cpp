#include <Core/Cached.hpp>

namespace fs::core {

Cached::Cached(std::unique_ptr<IO> io) noexcept :
    Default{std::move(io)}
{}

void Cached::close(Directory::Entry::index_type index)
{
    _buffers.erase(index);
    Default::close(index);
}

auto Cached::read(Directory::Entry::index_type index, std::size_t pos, std::span<std::byte> dst) const -> std::size_t
{
    if (auto buf_it = _buffers.find(index); buf_it != _buffers.end()) {    // if exists buffer for this file
         const Buffer& buf = buf_it->second;
         if (pos >= buf.buf_start_pos && pos + dst.size() <= buf.buf_start_pos + buf.data.size()) {    // if needed data is buffered
             std::copy_n(buf.data.begin() + (pos - buf.buf_start_pos), dst.size(), dst.begin());
             return dst.size();
         }
     }
     const std::size_t temp_buf_size = dst.size() + (block_length() - (pos + dst.size()) % block_length());  // dst.size + remaining bytes to the end of block
     auto temp_buf = std::vector<std::byte>(temp_buf_size);
     const std::size_t read_bytes = Default::read(index, pos, temp_buf);
     const std::size_t read_requested_bytes = std::min(dst.size(), read_bytes);
     std::copy_n(temp_buf.begin(), read_requested_bytes, dst.begin());

     auto& buf = _buffers[index];
     buf = Buffer{.buf_start_pos = pos, .data = std::move(temp_buf)};
     buf.data.resize(read_bytes);
     return read_requested_bytes;
}

auto Cached::write(Directory::Entry::index_type index, std::size_t pos, std::span<const std::byte> src) -> std::size_t
{
    const std::size_t bytes_written = Default::write(index, pos, src);
    if (auto cached_file = _entry_info_cache.find(index); cached_file != _entry_info_cache.end()) {
        auto& entries = _dir_cache[cached_file->second.first].entries;
        const auto file = std::lower_bound(entries.begin(), entries.end(), cached_file->second.second,
                                       [&] (const auto& file, const auto& name) { return file.name < name; });

        file->size = std::max(file->size, pos + bytes_written);    // updating file size in cache
    }
    return bytes_written;
}

auto Cached::create(Directory::index_type dir, const File& file) -> Directory::Entry::index_type
{
    const auto res = Default::create(dir, file);
    if (auto cached_dir_it = _dir_cache.find(dir); cached_dir_it != _dir_cache.end()) {    // caching entry in this block
        auto& cached_entries = cached_dir_it->second.entries;
        auto inserted = cached_entries.insert(
                std::upper_bound(cached_entries.begin(), cached_entries.end(), file,
                                 [&](const auto &lhs, const auto &rhs) { return lhs.name < rhs.name; }),
                Directory::Entry{file, res}
        );
        _entry_info_cache[inserted->index] = {dir, file.name};
    } else {    // adding cache for this dir
        if (auto fetched_dir = Default::get(dir); fetched_dir.has_value()) {
            const auto& cached_dir = _dir_cache[dir] = std::move(*fetched_dir);
            for (const auto& cached_entry : cached_dir.entries) {
                _entry_info_cache[cached_entry.index] = {dir, cached_entry.name};
            }
        }
    }
    return res;
}

auto Cached::search(Directory::index_type dir, std::string_view name) const -> std::optional<Directory::Entry::index_type>
{
    if (auto found_dir = _dir_cache.find(dir); found_dir != _dir_cache.end()) {
        const auto file = std::lower_bound(found_dir->second.entries.begin(), found_dir->second.entries.end(), name,
                                       [&] (const auto& file, const auto& name) { return file.name < name; });
        if (file != found_dir->second.entries.end() && file->name == name) {
            return file->index;
        }
    }
    return std::nullopt;
}

void Cached::remove(Directory::index_type dir, Directory::Entry::index_type index)
{
    Default::remove(dir, index);
    _buffers.erase(index);
    if (auto cached_dir_it = _dir_cache.find(dir); cached_dir_it != _dir_cache.end()) {   // cleanup cache in this block
        auto& cached_entries = cached_dir_it->second.entries;
        cached_entries.erase(std::find_if(cached_entries.begin(), cached_entries.end(),
                                          [&](const auto &file) { return file.index == index; }));
        _entry_info_cache.erase(index);
    }
}

auto Cached::get(Directory::index_type dir) const -> std::optional<Directory>
{
    if (auto cached_dir_it = _dir_cache.find(dir); cached_dir_it != _dir_cache.end()) {     // if dir is present in cache
        return cached_dir_it->second;
    }
    else {    // adding cache for this dir entries
        std::optional directory = Default::get(dir);
        if (directory.has_value()) {
             const auto& cached_dir = _dir_cache[dir] = *directory;
             for (const auto& cached_entry : cached_dir.entries) {
                 _entry_info_cache[cached_entry.index] = {dir, cached_entry.name};
            }
        }
        return directory;
    }
}
} // namespace fs::core
