#pragma once
#include <array>
#include <cstdint>
#include <vector.hpp>

namespace game {
template <typename T, uint64_t N = 0> struct history {
    std::array<T, N> data{};
    uint64_t size{};
    uint64_t read_index{};

    T &operator[](uint64_t index) { return data[index]; }

    const T &operator[](uint64_t index) const { return data[index]; }

    bool push(const T &value) {
        // truncate future timeline
        if (read_index + 1 < size) {
            size = read_index + 1;
        }

        if (size >= N) {
            return false;
        }

        data[size] = value;
        ++size;
        read_index = size - 1;
        return true;
    }

    bool pop() {
        if (size == 0)
            return false;
        --size;
        if (size == 0) {
            read_index = 0;
        } else if (read_index >= size) {
            read_index = size - 1;
        }
        return true;
    }

    bool undo() {
        if (size == 0 || read_index == 0)
            return false;
        --read_index;
        return true;
    }

    bool redo() {
        if (read_index + 1 >= size)
            return false;
        ++read_index;
        return true;
    }

    T *current() {
        if (size == 0)
            return nullptr;
        return &data[read_index];
    }

    void clear() { size = read_index = 0; }

    bool empty() const { return size == 0; }
};

template <class T> struct history<T, 0> {
    gtr::vector<T> data;
    uint64_t read_index{};

    T &operator[](uint64_t index) { return data[index]; }

    const T &operator[](uint64_t index) const { return data[index]; }

    bool push(const T &value) {
        // truncate future timeline
        if (read_index + 1 < data.size()) {
            data.resize(read_index + 1);
        }

        // append and advance read_index to the new end
        data.push_back(value);
        read_index = data.size() - 1;
        return true;
    }

    bool pop() {
        if (!data.empty()) {
            data.pop_back();
            read_index = data.empty() ? 0 : data.size() - 1;
            return true;
        }
        return false;
    }

    bool undo() {
        if (data.empty() || read_index == 0)
            return false;
        --read_index;
        return true;
    }

    bool redo() {
        if (read_index + 1 >= data.size())
            return false;
        ++read_index;
        return true;
    }

    T *current() {
        if (data.empty())
            return nullptr;
        return &data[read_index];
    }

    const T *current() const {
        if (data.empty())
            return nullptr;
        return &data[read_index];
    }

    void clear() {
        data.resize(0);
        read_index = 0;
    }

    bool empty() const {
        return data.empty();
    }

};
} // namespace game
