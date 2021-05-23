#include <Filesystem.hpp>

namespace fs {

Filesystem::Filesystem(core::Interface::Ptr core) noexcept :
    _core{std::move(core)}
{ }

void Filesystem::create(const std::string_view name)
{
    // TODO:
    throw Error{"not implemented"};
}

void Filesystem::destroy(const std::string_view name)
{
    // TODO:
    throw Error{"not implemented"};
}

auto Filesystem::open(const std::string_view name) -> Directory::Entry::index_type
{
    // TODO:
    throw Error{"not implemented"};
}

void Filesystem::close(const Directory::Entry::index_type index)
{
    // TODO:
    throw Error{"not implemented"};
}

auto Filesystem::read(const Directory::Entry::index_type index, std::span<std::byte> dst) const -> std::size_t
{
    // TODO:
    throw Error{"not implemented"};
}

auto Filesystem::write(const Directory::Entry::index_type index, const std::span<const std::byte> src) -> std::size_t
{
    // TODO:
    throw Error{"not implemented"};
}

void Filesystem::lseek(const Directory::Entry::index_type index, const std::size_t pos)
{
    // TODO:
    throw Error{"not implemented"};
}

auto Filesystem::directory() const -> std::vector<File>
{
    // TODO:
    throw Error{"not implemented"};
}

void Filesystem::save(const std::string_view path) const
{
    // TODO:
    throw Error{"not implemented"};

    /// Check for empty core before doing this
    _core->save(path);
}

} // namespace fs
