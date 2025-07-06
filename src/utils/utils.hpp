#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "vector.hpp"

namespace utils {
struct string_list {
    std::string data;
    uint32_t items;

    void append(const std::string &item) {
        if (!data.empty()) {
            data += ';';
        }
        data += item;
        items++;
    }

    void append(const char *item) { append(std::string(item)); }

    void cut_at(uint32_t index) {
        if (index < items) {
            size_t pos = 0;
            for (uint32_t i = 0; i < index; ++i) {
                pos = data.find(';', pos);
                if (pos == std::string::npos)
                    break;
                pos++;
            }
            data.erase(0, pos);
            items -= index;
        } else {
            data.clear();
            items = 0;
        }
    }

    const char *c_str() const { return data.c_str(); }

    void clear() {
        data.clear();
        items = 0;
    }
};

template <typename T> constexpr T abs(T value) { return (value < 0) ? -value : value; }
} // namespace utils
