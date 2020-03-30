#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <iomanip>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>


enum class Key {
    Arrow_left,   // 1b 5b 44
    Arrow_right,  // 1b 5b 43
    Arrow_up,     // 1b 5b 41
    Arrow_down,   // 1b 5b 42
    Backspace,    // 7f
    Esc,          // 1b
};
auto is_escape(std::vector<char> const buf) -> bool
{
    return (buf.size() == 1 and buf.at(0) == 0x1b);
}
auto is_arrow(std::vector<char> const buf) -> bool
{
    return (buf.size() == 3 and buf.at(0) == 0x1b and buf.at(1) == 0x5b);
}
auto is_left_arrow(std::vector<char> const buf) -> bool
{
    return (is_arrow(buf) and buf.at(2) == 0x44);
}
auto is_right_arrow(std::vector<char> const buf) -> bool
{
    return (is_arrow(buf) and buf.at(2) == 0x43);
}
auto is_up_arrow(std::vector<char> const buf) -> bool
{
    return (is_arrow(buf) and buf.at(2) == 0x41);
}
auto is_down_arrow(std::vector<char> const buf) -> bool
{
    return (is_arrow(buf) and buf.at(2) == 0x42);
}

auto set_cursor_position(int const line, int const column) -> void
{
    std::cout << ("\033[" + std::to_string(line + 1) + ";" + std::to_string(column + 1) + "H");
}
auto get_cursor_position() -> std::pair<int, int>
{
    /*
     * This command will write the cursor position to the terminal.
     */
    std::cout << "\033[6n";

    std::array<char, 2> buf { 0 };

    read(0, buf.data(), 2);  // read the \033[
    std::cout << std::oct << static_cast<int>(buf.at(0)) << std::dec;
    std::cout << buf.at(1);

    int line = 0;
    int column = 0;

    auto const read_digits = []() -> std::string
    {
        std::ostringstream o;
        auto c = getchar();
        while ((c >= '0') and (c <= '9')) {
            o << c;
            c = getchar();
        }

        auto buf = static_cast<char>(c);
        write(0, &buf, 1);

        return o.str();
    };
    if (auto const r = read_digits(); not r.empty()) {
        line = std::stoi(r);
    } else {
        std::cerr << "no digits!\n";
    }

    read(0, buf.data(), 1);  // read the ;

    if (auto const r = read_digits(); not r.empty()) {
        column = std::stoi(r);
    } else {
        std::cerr << "no digits!\n";
    }

    read(0, buf.data(), 1);  // read the R

    return { line, column };
}

auto main(int argc, char** argv) -> int
{
    auto const path = std::string{argc > 1 ? argv[1] : "."};

    for (auto const& entry : std::filesystem::directory_iterator{path}) {
        std::cout << entry.path() /* << " " << entry.status */ << "\n";
    }

    system("stty raw");
    system("stty -echo");

    /*
     * Set standard-input to non-blocking mode.
     */
    fcntl(0, fcntl(0, F_GETFD) | O_NONBLOCK);

    set_cursor_position(0, 0);
    std::cout << "at 0,0\n";
    set_cursor_position(0, 0);

    struct cursor_position_t {
        int line = 0;
        int column = 0;
    } cursor_position;

    while (true) {
        std::array<char, 3> buf { 0, 0, 0 };
        auto const r = read(0, buf.data(), 3);
        if (r == -1 and errno == EAGAIN) {
            continue;
        }
        if (r == 0) {
            continue;
        }

        auto const v = std::vector<char>{buf.begin(), buf.begin() + r};
        if (is_escape(v)) {
            break;
        }

        /*
        if (is_left_arrow(v)) {
            --cursor_position.column;
            set_cursor_position(0, 0);
            std::cout << "[" << cursor_position.line << "/" << cursor_position.column << "]\n";
            set_cursor_position(cursor_position.line - 1, cursor_position.column);
            std::cout << "\n";
            continue;
        }
        if (is_right_arrow(v)) {
            ++cursor_position.column;
            set_cursor_position(0, 0);
            std::cout << "[" << cursor_position.line << "/" << cursor_position.column << "]\n";
            set_cursor_position(cursor_position.line - 1, cursor_position.column);
            std::cout << "\n";
            continue;
        }
        */
        if (is_down_arrow(v)) {
            ++cursor_position.line;
            std::cout << "\033[1BHello";
            set_cursor_position(cursor_position.line, 0);
            /* set_cursor_position(cursor_position.line, cursor_position.column); */
            /* std::cout << "[" << cursor_position.line << "/" << cursor_position.column << "]\033["; */
            continue;
        }
        if (is_up_arrow(v)) {
            --cursor_position.line;
            std::cout << "\033[1AHello";
            set_cursor_position(cursor_position.line, 0);
            /* set_cursor_position(cursor_position.line, cursor_position.column); */
            /* std::cout << "[" << cursor_position.line << "/" << cursor_position.column << "]\033["; */
            continue;
        }

        std::cout << std::hex;
        for (auto const each : v) {
            std::cout << "0x" << std::setw(2) << static_cast<int>(each);
        }
        std::cout << std::dec << "\n";
    }

    system("stty echo");
    system("stty cooked");

    return 0;
}
