#pragma once
#ifndef PAIR_HPP
#define PAIR_HPP
namespace gtr {
template <typename T1, typename T2> struct pair {
    T1 first;  ///< The first element of the pair.
    T2 second; ///< The second element of the pair.
};

template <typename T1, typename T2> struct compressed_pair : T2 {
    T1 first;                        ///< The first element of the pair.
};
}; // namespace gtr
#endif
