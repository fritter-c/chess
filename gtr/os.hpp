#pragma once
namespace gtr {
enum class OS { WINDOWS, LINUX, MACOS };
consteval OS get_os() {
#if defined(_MSC_VER) || defined(_WIN64) || defined(_WIN32)
    return OS::WINDOWS;
#elif defined(__linux__)
    return OS::LINUX;
#elif defined(__APPLE__)
    return OS::MACOS;
#endif
}
} // namespace gtr