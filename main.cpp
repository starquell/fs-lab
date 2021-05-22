#include <Core/Cached.hpp>
#include <Filesystem.hpp>
#include <Util.hpp>
#include <IO.hpp>

#include <fmt/format.h>
#include <fmt/color.h>
#include <type_traits>
#include <string_view>
#include <iostream>
#include <charconv>
#include <string>
#include <tuple>

namespace {
namespace detail {

    template<typename T, typename = std::void_t<>>
    constexpr bool has_input = false;

    template<typename T>
    constexpr bool has_input<T, std::void_t<typename T::Input>> = true;

    template<typename T, typename... Args>
    constexpr bool has_result = !std::is_same_v<
        void, 
        decltype(std::declval<T>()(std::declval<Args>()...))
    >;

    [[nodiscard]]
    bool parse(const std::string_view str, char& result) noexcept
    {
        if (str.size() != 1) {
            return false;
        }

        result = str.front();
        return true;
    }

    [[nodiscard]]
    bool parse(const std::string_view str, std::string& result)
    {
        result.assign(str);
        return true;
    }

    template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    [[nodiscard]]
    bool parse(const std::string_view str, T& result) noexcept
    {
        const auto [p, ec] = std::from_chars(str.begin(), str.end(), result);
        return ec == std::errc{} && p == str.end();
    }

} // namespace detail

struct cd
{
    static constexpr std::string_view usage = "cd <name>";
    static constexpr std::string_view description = "create a new file with the name <name>";
    static constexpr std::string_view output = R"(file "{}" created)";
    static constexpr std::string_view cmd = "cd";

    struct Input
    {
        std::string name;

        static constexpr auto args = std::tuple{
            &Input::name
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        fs.create(in.name);
        return std::tuple{in.name};
    }
};

struct de
{
    static constexpr std::string_view usage = "de <name>";
    static constexpr std::string_view description = "destroy the named file <name>";
    static constexpr std::string_view output = R"(file "{}" destroyed)";
    static constexpr std::string_view cmd = "de";

    struct Input
    {
        std::string name;

        static constexpr auto args = std::tuple{
            &Input::name
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        fs.destroy(in.name);
        return std::tuple{in.name};
    }
};

struct op
{
    static constexpr std::string_view usage = "op <name>";
    static constexpr std::string_view description = "open the named file <name> for reading and writing; display an index value";
    static constexpr std::string_view output = R"(file "{}" opened, index={})";
    static constexpr std::string_view cmd = "op";

    struct Input
    {
        std::string name;

        static constexpr auto args = std::tuple{
            &Input::name
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        const auto index = fs.open(in.name);
        return std::tuple{in.name, index};
    }
};

struct cl
{
    static constexpr std::string_view usage = "cl <index>";
    static constexpr std::string_view description = "close the specified file <index>";
    static constexpr std::string_view output = "file {} closed";
    static constexpr std::string_view cmd = "cl";

    struct Input
    {
        size_t index;

        static constexpr auto args = std::tuple{
            &Input::index
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        fs.close(in.index);
        return std::tuple{in.index};
    }
};

struct rd
{
    static constexpr std::string_view usage = "rd <index> <count>";
    static constexpr std::string_view description = "sequentially read a number of bytes <count> from the specified file <index> and display them on the terminal";
    static constexpr std::string_view output = R"({} bytes read: "{}")";
    static constexpr std::string_view cmd = "rd";

    struct Input
    {
        size_t index;
        size_t count;

        static constexpr auto args = std::tuple{
            &Input::index,
            &Input::count
        };
    };

    auto operator()(const Input in, const fs::Filesystem& fs) const
    {
        std::string result;
        result.resize(in.count);
        const auto read = fs.read(in.index, {reinterpret_cast<std::byte*>(result.data()), result.size()});
        result.resize(read);
        return std::tuple{read, result};
    }
};

struct wr
{
    static constexpr std::string_view usage = "wr <index> <char> <count>";
    static constexpr std::string_view description = "sequentially write <count> number of <char>s into the specified file <index> at its current position";
    static constexpr std::string_view output = "{} bytes written";
    static constexpr std::string_view cmd = "wr";

    struct Input
    {
        size_t index;
        char ch;
        size_t count;

        static constexpr auto args = std::tuple{
            &Input::index,
            &Input::ch,
            &Input::count
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        std::string data;
        data.assign(in.count, in.ch);
        const auto written = fs.write(in.index, {reinterpret_cast<const std::byte*>(data.data()), data.size()});
        return std::tuple{written};
    }
};

struct sk
{
    static constexpr std::string_view usage = "sk <index> <pos>";
    static constexpr std::string_view description = "set the current position of the specified file <index> to <pos>";
    static constexpr std::string_view output = "current position is {}";
    static constexpr std::string_view cmd = "sk";

    struct Input
    {
        size_t index;
        size_t pos;

        static constexpr auto args = std::tuple{
            &Input::index,
            &Input::pos
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        fs.lseek(in.index, in.pos);
        return std::tuple{in.pos};
    }
};

struct dr
{
    static constexpr std::string_view usage = "dr";
    static constexpr std::string_view description = "directory: list the names of all files and their lengths";
    static constexpr std::string_view output = "{}";
    static constexpr std::string_view cmd = "dr";

    auto operator()(const fs::Filesystem& fs) const
    {
        std::string result;
        for (const auto& file : fs.directory()) {
            fmt::format_to(std::back_inserter(result), "{} {}, ", file.name, file.size);
        }

        /// Remove trailing ", "
        if (!result.empty()) {
            result.resize(result.size() - 2);
        }

        return std::tuple{result};
    }
};

struct in
{
    static constexpr std::string_view usage = "in <cylinders> <surfaces> <sectors> <block_size> <path>";
    static constexpr std::string_view description = "create a disk using the given dimension parameters and initialize it using the file";
    static constexpr std::string_view output = "disk {}";
    static constexpr std::string_view cmd = "in";

    struct Input
    {
        size_t cylinders;
        size_t surfaces;
        size_t sectors;
        size_t block_size;
        std::string path;

        static constexpr auto args = std::tuple{
            &Input::cylinders,
            &Input::surfaces,
            &Input::sectors,
            &Input::block_size,
            &Input::path
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        // TODO:
        return std::tuple{"uninitialized"};
    }
};

struct sv
{
    static constexpr std::string_view usage = "sv <path>";
    static constexpr std::string_view description = "close all files and save the contents of the disk in the file <path>";
    static constexpr std::string_view output = "disk saved";
    static constexpr std::string_view cmd = "sv";

    struct Input
    {
        std::string path;

        static constexpr auto args = std::tuple{
            &Input::path
        };
    };

    auto operator()(const Input in, fs::Filesystem& fs) const
    {
        // TODO:
    }
};

using Commands = std::tuple<cd, de, op, cl, rd, wr, sk, dr, in, sv>;

template<typename F, typename... Args>
void error(F&& format, Args&&... args)
{
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "error");
    fmt::print(": ");
    fmt::print(std::forward<F>(format), std::forward<Args>(args)...);
}

template<typename Cmd, typename... Args>
void process(Args&&... args)
{
    const auto invoke = [&] {
        return Cmd{}(std::forward<Args>(args)...);
    };

    if constexpr (detail::has_result<Cmd, Args...>) {
        const auto result = invoke();
        std::apply(
            [] (const auto&... args) {
                fmt::print(Cmd::output, args...);
            }, 
            result
        );
    } else {
        invoke();
        fmt::print(Cmd::output);
    }
}

void interact(fs::Filesystem& fs)
{
    /// Show shell usage message
    fmt::print("SHELL USAGE\n\n");
    fs::util::for_each_type_in<Commands>([] (auto t) {
        using cmd = typename decltype(t)::type;
        fmt::print(
            "* {} - {}\n"
            "     usage: {}\n"
            "\n", 
            cmd::cmd,
            cmd::description,
            cmd::usage
        );
    });

    /// Run main loop
    for (;;) {
        fmt::print("cmd> ");
        std::string line;
        std::getline(std::cin, line, '\n');

        try {
            bool found = false;
            fs::util::for_each_type_in<Commands>([&] (auto t) {
                using cmd = typename decltype(t)::type;
                if (!line.starts_with(cmd::cmd)) {
                    return;
                }

                found = true;
                if constexpr (detail::has_input<cmd>) {
                    /// Parse arguments
                    using Input = typename cmd::Input;
                    constexpr auto count = std::tuple_size_v<decltype(Input::args)> + 1;
                    const auto tokens = fs::util::split_as_array<count>(line);
                    if (!tokens) {
                        return error("invalid input");
                    }

                    Input input;
                    const bool valid = std::apply(
                        [&, idx = 0] (const auto... args) mutable {
                            return (detail::parse(tokens.value()[++idx], input.*args) && ...);
                        },
                        Input::args
                    );
                    if (!valid) {
                        return error("invalid arguments");
                    }

                    process<cmd>(input, fs);
                } else {
                    process<cmd>(fs);
                }
            });

            if (!found) {
                error("unknown command");
            }
        } catch(const fs::Error& e) {
            error(e.what());
        } catch (const std::exception& e) {
            error("unknown error: {}", e.what());
        } catch (...) {
            error("unknown internal error");
        }

        fmt::print("\n");
    }
}

} // namespace

int main()
{
    /// Create empty filesystem
    fs::Filesystem fs{nullptr};

    /// Run user interaction loop
    interact(fs);

    return 0;
}
