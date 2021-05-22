#include <Core/Cached.hpp>

namespace fs::core {

struct Cached::State
{

};

Cached::Cached(std::unique_ptr<IO> io) noexcept :
    Default{std::move(io)},
    _state{std::make_unique<State>()}
{ }

// TODO(starquell): buffering (map[file index][current position + read buffer + write buffer]),
// and other entities that can be cached

} // namespace fs::core
