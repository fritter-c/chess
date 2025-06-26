#pragma once
#include <cstdint>
#include <cstdlib>

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
                    items = static_cast<T *>(std::realloc(items, sizeof(T) * new_capacity));
                } else {
                    items = static_cast<T *>(std::malloc(sizeof(T) * new_capacity));
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

    template<int Capacity = 32>
    struct text_buffer {
        char text[Capacity];

        char *operator()() {
            return text;
        }

        void push_back(const char ch) {
            for (uint32_t i = 0; i < Capacity - 2; ++i) {
                if (text[i] == '\0') {
                    text[i] = ch;
                    text[i + 1] = '\0';
                    break;
                }
            }
        }

        int32_t size() {
            int32_t size = 0;
            for (; size < Capacity && text[size] != '\0'; ++size){}
            return size;
        }
    };
}
