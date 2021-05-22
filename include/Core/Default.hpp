#pragma once

#include <Core/Interface.hpp>
#include <IO.hpp>

#include <memory>

namespace fs::core {

/**
 * @brief Default implementation of interface between FS and I/O.
 */
class Default : public Interface
{
public:
    /**
     * @brief Initialize with pointer to I/O system.
     */
    explicit Default(std::unique_ptr<IO> io) noexcept;

    /**
     * @brief Save content for further restoring into specified file.
     */
    void save(std::string_view path) const final;

private:
    std::unique_ptr<IO> _io;
};

} // namespace fs::core
