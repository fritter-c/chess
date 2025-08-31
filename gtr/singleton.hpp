#pragma once
#ifndef SINGLETON_HPP
#define SINGLETON_HPP
#include "nocopy.hpp"
namespace gtr {
template <typename T> struct singleton : nocopy {
    static T &instance() {
        static T instance;
        return instance;
    }
    singleton() = default;
};
} // namespace gtr
#endif
