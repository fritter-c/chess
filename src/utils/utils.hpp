#pragma once
#include <cstdint>
#include <cstdlib>
#include <malloc.h>

namespace utils {
    template<typename T>
    struct list {
        T *items;
        uint32_t size;
        uint32_t capacity;

        void push_back(const T &item) {
            if (size == capacity) {
                reserve(capacity ? capacity * 2 : 1);
            }
            items[size++] = item;
        }

        void reserve(const uint32_t new_capacity) {
            if (new_capacity > capacity) {
                if (items) {
                    items = static_cast<T *>(realloc(items, sizeof(T) * new_capacity));
                } else {
                    items = static_cast<T *>(malloc(sizeof(T) * new_capacity));
                }
                capacity = new_capacity;
                if (!items) {
                    exit(-1);
                }
            }
        }

        T &operator[](const uint32_t index) {
            return items[index];
        }

        void free() {
            if (items) {
                free(items);
                items = nullptr;
            }
            size = 0;
            capacity = 0;
        }

        void clear() {
            size = 0;
        }
    };
}
