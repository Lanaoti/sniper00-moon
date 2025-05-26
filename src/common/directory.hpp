#pragma once
#include "common.hpp"
#include "common/string.hpp"
#include <filesystem>

#if TARGET_PLATFORM == PLATFORM_MAC
    #include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace moon {
class directory {
public:
    inline static fs::path working_directory;

    static std::string current_directory() {
        std::error_code ec;
        auto p = fs::current_path(ec);
        return p.string();
    }

    static fs::path module_path() {
#if TARGET_PLATFORM == PLATFORM_WINDOWS
        char temp[MAX_PATH];
        auto len = GetModuleFileName(NULL, temp, MAX_PATH);
        if (0 == len)
            throw std::runtime_error(moon::format("module_path error: %u", GetLastError()));
        fs::path p = fs::path(std::string_view(temp, len));
#elif TARGET_PLATFORM == PLATFORM_MAC
        char temp[1024];
        uint32_t len = sizeof(temp);
        if (_NSGetExecutablePath(temp, &len) != 0)
            throw std::runtime_error("module_path error: buffer too small");
        fs::path p = fs::canonical(fs::path(temp));
#else
        char temp[1024];
        auto len = readlink("/proc/self/exe", temp, sizeof(temp) - 1);
        if (len<0 || (len >= static_cast<ssize_t>(sizeof(temp) - 1)))
            throw std::runtime_error(moon::format("module_path error %d", errno));
        temp[len] = '\0';
        fs::path p = fs::canonical(fs::path(temp));
#endif

        if (!p.has_filename() || p.filename().empty()) {
            p = p.parent_path();
        }
        return p.parent_path();
    }

    static bool exists(const std::string& path) {
        std::error_code ec;
        auto b = fs::exists(path, ec);
        return (!ec) && b;
    }

    //THandler bool(const fs::path& path,bool dir)
    template<typename Handler>
    static void scan_dir(const std::string_view& dir, int max_depth, const Handler& handler) {
        struct do_scan {
            int depth = 0;
            void operator()(const fs::path& path, const Handler& handler) {
                if (depth < 0) {
                    return;
                }

                std::error_code ec;
                if (!fs::exists(path, ec)) {
                    return;
                }

                depth--;

                for (auto& p: fs::directory_iterator(path)) {
                    if (!handler(p.path(), fs::is_directory(p, ec))) {
                        break;
                    }
                    if (fs::is_directory(p, ec)) {
                        operator()(p.path(), handler);
                    }
                }
            }
        };

        do_scan d { max_depth };
        d(dir, handler);
    }

    static bool create_directory(const std::string& dir) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        return !ec;
    }

    static bool remove(const std::string& dir) {
        std::error_code ec;
        fs::remove(dir, ec);
        return !ec;
    }

    static bool remove_all(const std::string& dir) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        return !ec;
    }

    static std::string find(const std::string& path, const std::string& filename, int depth = 10) {
        std::string result;
        std::vector<std::string> searchdir = moon::split<std::string>(path, ";");
        for (const auto& v: searchdir) {
            scan_dir(v, depth, [&result, &filename](const fs::path& p, bool) {
                if (p.filename().string() == filename) {
                    result = fs::absolute(p).string();
                    return false;
                }
                return true;
            });

            if (!result.empty()) {
                return result;
            }
        }
        return result;
    }
};
} // namespace moon