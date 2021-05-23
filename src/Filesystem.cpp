#include <Filesystem.hpp>

#include <fmt/format.h>

namespace {

    auto findFile (const fs::Directory& dir, const std::string_view filename) -> std::optional<fs::Directory::Entry>
    {
        auto file = std::lower_bound(dir.entries.begin(), dir.entries.end(), filename,
                                       [&] (const auto& file, const auto& filename) { return file.name < filename; });
        return (file != dir.entries.end() && file->name == filename) ? std::optional{*file} : std::nullopt;
    }
}
namespace fs {

Filesystem::Filesystem(core::Interface::Ptr core) noexcept :
    _core{std::move(core)}
{ }

void Filesystem::create(const std::string_view name)
{
    if (findFile(*_core->get(_cd), name).has_value()) {
        throw Error{R"(file with name "{}" already exists)", name};
    }
    _core->create(_cd, File{.size = 0, .name = std::string{name}});
}

void Filesystem::destroy(const std::string_view name)
{
    if (auto file = findFile(*_core->get(_cd), name); !file.has_value()) {
        throw Error{R"(file with name "{}" does not exist)", name};
    }
    else {
        _core->remove(_cd, file->index);
        _oft.erase(file->index);
    }
}

auto Filesystem::open(const std::string_view name) -> Directory::Entry::index_type
{
    if (auto file = findFile(*_core->get(_cd), name); file.has_value()) {
        if (_oft.contains(file->index)) {
            throw Error{"file is already open."};
        } else {
            _core->open(file->index);
            _oft[file->index] = 0;
            return file->index;
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
        it->second = _core->read(index, it->second, dst);
        return it->second;
    }
    throw Error{"file is not opened"};
};

auto Filesystem::write(const Directory::Entry::index_type index, const std::span<const std::byte> src) -> std::size_t
{
    if (auto it = _oft.find(index); it != _oft.end()) {
        it->second = _core->write(index, it->second, src);
        return it->second;
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
    std::transform(entries.begin(), entries.end(), res.begin(), [] (auto& entry) {return std::move(entry);});
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
