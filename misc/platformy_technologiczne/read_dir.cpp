#include <dirent.h> // for readdir_r(3), fdopendir(3), closedir(3)
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>


template<typename T>
auto memset(T* const s, int const c = 0) -> T*
{
    return reinterpret_cast<dirent*>(
        memset(reinterpret_cast<void*>(s), c, sizeof(T)));
}


auto main(int argc, char** argv) -> int
{
    auto const dir_name = std::string{argc > 1 ? argv[1] : "."};
    auto const dir_fd = open(dir_name.c_str(), O_RDONLY);
    std::cerr << "open(\"" << dir_name << "\"): " << dir_fd << "\n";

    auto const dir = fdopendir(dir_fd);

    dirent entry;
    memset(&entry);
    dirent* dummy = nullptr;
    auto const ret = readdir_r(dir, &entry, &dummy);

    closedir(dir);

    return 0;
}
