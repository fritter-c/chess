#ifndef UTILS_HPP
#define UTILS_HPP
namespace gtr {
constexpr bool is_numeric(const char c) { return c >= '0' && c <= '9'; }
constexpr bool is_alpha(const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
} // namespace gtr
#endif // UTILS_HPP
