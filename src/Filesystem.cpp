#include <Filesystem.hpp>

namespace fs {

Filesystem::Filesystem(core::Interface::Ptr core) noexcept :
    _core{std::move(core)}
{ }

void Filesystem::create(const std::string_view name)
{
    if (_core->search(_cd, name).has_value()) {
        throw Error{R"(file with name "{}" already exists)", name};
    }
    _core->create(_cd, File{.size = 0, .name = std::string{name}});
}

void Filesystem::destroy(const std::string_view name)
{
    if (auto file_index = _core->search(_cd, name); !file_index.has_value()) {
        throw Error{R"(file with name "{}" does not exist)", name};
    }
    else {
        _core->remove(_cd, *file_index);
        _oft.erase(*file_index);
    }
}

auto Filesystem::open(const std::string_view name) -> Directory::Entry::index_type
{
    if (auto file = _core->search(_cd, name); file.has_value()) {
        if (_oft.contains(*file)) {
            throw Error{"file is already open."};
        } else {
            _core->open(*file);
            _oft[*file] = 0;
            return *file;
        }
    } else {
        throw Error{"file with name {} is not found", name};
    }
}

void Filesystem::close(const Directory::Entry::index_type index)
{
    if (auto it = _oft.find(index); it != _oft.end()) {
        _core->close(index);
        _oft.erase(it);
        return;
    }
    throw Error{"file is not opened"};
}

auto Filesystem::read(const Directory::Entry::index_type index, std::span<std::byte> dst) const -> std::size_t
{
    if (auto it = _oft.find(index); it != _oft.end()) {
        const auto read = _core->read(index, it->second, dst);
        it->second += read;
        return read;
    }
    throw Error{"file is not opened"};
};

auto Filesystem::write(const Directory::Entry::index_type index, const std::span<const std::byte> src) -> std::size_t
{
    if (auto it = _oft.find(index); it != _oft.end()) {
        const auto written = _core->write(index, it->second, src);
        it->second += written;
        return written;
    }
    throw Error{"file is not opened"};
}

void Filesystem::lseek(const Directory::Entry::index_type index, const std::size_t pos)
{
    if (auto it = _oft.find(index); it != _oft.end()) {
        it->second = pos;
    }
    throw Error{"file is not opened"};
}

auto Filesystem::directory() const -> std::vector<File>
{
    auto res = std::vector<File>{};
    auto entries = _core->get(_cd)->entries;
    std::transform(entries.begin(), entries.end(), std::back_inserter(res),
                   [] (auto& entry) {return std::move(entry);}); // TODO: @starquell check this out. Some changes were required
    return res;
}

void Filesystem::save(const std::string_view path)
{
    for (const auto& [file, _] : _oft) {
        close(file);
    }
    _oft.clear();
    _core->save(path);
}

} // namespace fs
