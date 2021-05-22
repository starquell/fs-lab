#pragma once

#include <Core/Default.hpp>
#include <memory>

namespace fs::core {

/**
 * @brief Implementation of communication with I/O subsystem that
 *        caches some data.
 */
class Cached final : public Default
{
public:
    /**
     * @brief Initialize with pointer to I/O system.
     */
    explicit Cached(std::unique_ptr<IO> io) noexcept;

private:
    struct State;

    std::unique_ptr<State> _state;
};

} // namespace fs::core
