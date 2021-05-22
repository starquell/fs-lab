#include <Core/Default.hpp>

namespace fs::core {
namespace {

// TODO(carexoid): Place here all the gory details of implementation.
// Like bitmap, file represantion on and so on.

} // namespace

Default::Default(std::unique_ptr<IO> io) noexcept :
    _io{std::move(io)}
{ }


void Default::save(const std::string_view path) const
{
    _io->save(path);
}

} // namespace fs::core
