#include <Core/Cached.hpp>

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
    Buffer* buf = nullptr;
    if (auto buf_it = _buffers.find(index); buf_it != _buffers.end()) {    // if exists buffer for this file
         buf = &buf_it->second;
         auto unread_buffer = buf->get_unread_bytes();
         if (pos >= buf->pos && pos + dst.size() <= buf->pos + unread_buffer.size()) {    // if needed data buffered
             std::copy_n(unread_buffer.begin(), dst.size(), dst.begin());
             buf->pos += dst.size();
             return dst.size();
         }
     }
     const std::size_t temp_buf_size = dst.size() + (block_length() - (pos + dst.size()) % block_length());  // dst.size + remaining bytes to the end of block
     auto temp_buf = std::vector<std::byte>(temp_buf_size);
     const std::size_t read_bytes = Default::read(index, pos, temp_buf);
     const std::size_t read_requested_bytes = std::min(dst.size(), read_bytes);
     std::copy_n(temp_buf.begin(), read_requested_bytes, dst.begin());

     const std::size_t new_buf_size = (pos + read_bytes) % block_length() == 0      // size of new buffer: <= size of block
                                       ? block_length()
                                       : (pos + read_bytes) % block_length();
     if (!buf) {    // buffering this file
         auto& elem = (_buffers[index] = Buffer{.pos = pos + read_requested_bytes, .data = std::vector<std::byte>(new_buf_size)});
         buf = &elem;
     } else {   // update old buffer
         buf->pos += read_requested_bytes;
         buf->data.resize(new_buf_size);
     }
     std::copy_n(temp_buf.begin() + read_bytes - new_buf_size, new_buf_size, buf->data.begin());
     return read_requested_bytes;
}

auto Cached::write(Directory::Entry::index_type index, std::size_t pos, std::span<const std::byte> src) -> std::size_t
{
    return Default::write(index, pos, src);
}

auto Cached::create(Directory::index_type dir, const File& file) -> Directory::Entry::index_type
{
    const auto res = Default::create(dir, file);
    if (auto cached_dir_it = _dir_cache.find(dir); cached_dir_it != _dir_cache.end()) {    // caching entry in this block
        auto& cached_entries = cached_dir_it->second.entries;
        cached_entries.insert(
                std::upper_bound(cached_entries.begin(), cached_entries.end(), file,
                                 [&](const auto &lhs, const auto &rhs) { return lhs.name < rhs.name; }),
                Directory::Entry{file, res}
        );
    } else {    // adding cache for this dir entries
        if (std::optional fetched_dir = get(dir)) {
            _dir_cache[dir] = std::move(*fetched_dir);
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
            _dir_cache[dir] = *directory;
        }
        return directory;
    }
}

auto Cached::Buffer::get_unread_bytes() -> std::span<std::byte>
{
    return {data.data() + data.size() - (pos % data.size()), data.data() + data.size()};
}
} // namespace fs::core
