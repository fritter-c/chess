#pragma once
#ifndef NOCOPY_HPP
#define NOCOPY_HPP

namespace gtr {
struct nocopy {
    nocopy() = default;
    nocopy(const nocopy &) = delete;
    nocopy &operator=(const nocopy &) = delete;
    nocopy(nocopy &&other) noexcept = default;
    nocopy &operator=(nocopy &&other) noexcept = default;
};
} // namespace gtr
#endif
