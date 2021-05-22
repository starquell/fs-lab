#include <Filesystem.hpp>

namespace fs {

Filesystem::Filesystem(core::Interface::Ptr core) noexcept :
    _core{std::move(core)}
{ }

void Filesystem::update(core::Interface::Ptr core) noexcept
{
    _core = std::move(core);
}

void Filesystem::create(const std::string_view name)
{
    // TODO():
}

void Filesystem::destroy(const std::string_view name)
{
    // TODO:
}

auto Filesystem::open(const std::string_view name) -> Directory::Entry::index_type
{
    // TODO:
}

void Filesystem::close(const Directory::Entry::index_type index)
{
    // TODO:
}

void Filesystem::read(const Directory::Entry::index_type index, std::span<std::byte> dst)
{
    // TODO:
}

void Filesystem::write(const Directory::Entry::index_type index, const std::span<const std::byte> src)
{
    // TODO:
}

void Filesystem::lseek(const Directory::Entry::index_type index, const std::size_t pos)
{
    // TODO:
}

auto Filesystem::directory() const -> std::vector<File>
{
    // TODO:
}

} // namespace fs
