#ifndef GTRSTRING_H
#define GTRSTRING_H
#include <algorithm>
#include <bit>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <emmintrin.h>
#include <memory>
#include "allocator.hpp"
#include "assert.hpp"
#include "container_base.hpp"
#include "os.hpp"
#define PARANOID_ASSERT(x) ((void)0)
namespace gtr {
/**
 * @brief A text container with a fixed-size local buffer and dynamic allocation support.
 *
 * This struct provides a text container that can store text data in a fixed-size local buffer.
 * If the text data exceeds the capacity of the local buffer, it dynamically allocates memory
 * on the heap. The text container supports various operations such as formatting, appending,
 * inserting, and comparing text data. It also provides methods for converting text to different
 * types, splitting text, and checking if the text is a number.
 *
 * @tparam N The size of the local buffer.
 * @tparam Allocator The allocator type used for dynamic memory allocation.
 */
template <int32_t N = 64, class Allocator = c_allocator<char>> struct char_string : container_base<Allocator> {
    using value_type = c_allocator<char>::value_type;
    using iterator = c_allocator<char>::pointer_type;
    using const_iterator = const value_type *;
    using allocator_type = Allocator;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = c_allocator<char>::size_type;
    using difference_type = c_allocator<char>::difference_type;
    static constexpr size_type npos = static_cast<size_type>(-1);

    static_assert(N > 3 * sizeof(uint64_t), "Text N must be bigger 3 * sizeof(uint64_t)");
    static_assert(N % 16 == 0, "Buffer size N must be a multiple of 16 for safe SIMD loads.");

  private:
    alignas(c_allocator<char>::pointer_type) char data[N]{}; // Either a local buffer or [0] pointer, [1] size, [2] capacity and the last byte the heap flag
                                                             // Data is a union {char[N]; {char *, uint64_t, uint64_t}; }
    uint64_t strlen() const {
        PARANOID_ASSERT(!local_data());
        if (!data[0])
            return 0;
        const char *s = data;
        const __m128i zero = _mm_setzero_si128();
        while (true) {
            const __m128i chunk = _mm_loadu_si128(std::bit_cast<const __m128i *>(s));
            const __m128i cmp = _mm_cmpeq_epi8(chunk, zero);
            if (const uint32_t mask = _mm_movemask_epi8(cmp); mask != 0) {
                return (s - data) + std::countr_zero(mask);
            }
            s += 16;
        }
        Unreachable("strlen() failed");
    }

    static int32_t sse_strncmp(const char_string &a, const char_string &b) {
        const char *s1 = a.c_str();
        const char *s2 = b.c_str();
        const auto diff = static_cast<int32_t>(a.size() - b.size());
        if (!diff) {
            for (uint64_t i = 0; i < a.size(); i += 16) {
                const __m128i chunk1 = _mm_loadu_si128(std::bit_cast<const __m128i *>(s1 + i));
                const __m128i chunk2 = _mm_loadu_si128(std::bit_cast<const __m128i *>(s2 + i));
                const __m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);
                if (const int32_t mask = _mm_movemask_epi8(cmp); mask != 0xFFFF) {
                    const int32_t diffIndex = std::countr_zero(static_cast<uint32_t>(~mask));
                    const uint64_t index = i + diffIndex;
                    return static_cast<unsigned char>(s1[index]) - static_cast<unsigned char>(s2[index]);
                }
            }
        }
        return diff;
    }
    static uint64_t next_multiple_of_16(const uint64_t n) { return (n + 15) & ~15; }

  public:
    static constexpr char path_separator() {
        if constexpr (get_os() == OS::WINDOWS) {
            return '\\';
        } else {
            return '/';
        }
    }

    Allocator &allocator() { return container_base<Allocator>::allocator(); }
    const Allocator &allocator() const { return container_base<Allocator>::allocator(); }

    /**
     * @brief Retrieves a reference to the pointer to the dynamically allocated data.
     *
     * This function returns a reference to the pointer that points to the dynamically
     * allocated data on the heap. It asserts that the data is not stored in the local
     * buffer when TEXT_PARANOID is defined.
     *
     * @return char*& A reference to the pointer to the dynamically allocated data.
     */
    value_type *&get_pointer() {
        PARANOID_ASSERT(!local_data());
        return *reinterpret_cast<value_type **>(data);
    }

    /**
     * @brief Retrieves a constant reference to the pointer to the dynamically allocated data.
     *
     * This function returns a constant reference to the pointer that points to the dynamically
     * allocated data on the heap. It asserts that the data is not stored in the local
     * buffer when TEXT_PARANOID is defined.
     *
     * @return const char *const& A constant reference to the pointer to the dynamically allocated data.
     */
    const value_type *const &get_pointer() const {
        PARANOID_ASSERT(!local_data());
        return *reinterpret_cast<const value_type *const *>(data);
    }

    /**
     * @brief Retrieves a pointer to the plain data.
     *
     * This function returns a pointer to the plain data that is either a string or
     *
     * @return char* A pointer to the plain data.
     */
    value_type *get_plain_data() { return data; }

    /**
     * @brief Retrieves a pointer to the plain data.
     *
     * This function returns a pointer to the plain data that is either a string or
     *
     * @return char* A pointer to the plain data.
     */
    const value_type *get_plain_data() const { return data; }

    /**
     * @brief Checks if the data is stored in the local buffer.
     *
     * This function checks the last byte of the local buffer to determine if the data
     * is stored in the local buffer or on the heap. If its local the byte is the null terminator.
     *
     * @return true if the data is stored in the local buffer, false otherwise.
     */
    bool local_data() const { return !data[N - 1]; }

    /**
     * @brief Marks the data as stored in the heap
     *
     * This function sets the last byte of the local buffer to 1 to indicate
     * that the data is stored in the heap
     */
    void set_heap() { data[N - 1] = 1; }

    /**
     * @brief Constructs an  text object.
     *
     * Initializes the text object with a zeroed local buffer
     */
    constexpr char_string() = default;

    /**
     * @brief Returns the size of the text container.
     *
     * This function determines the size of the text container by checking if the
     * data is stored in the local buffer or not. If the data is in the local buffer,
     * it calculates the size using `strlen`. Otherwise, it retrieves the size
     * from the second element of the local buffer, interpreted as a `size_type`.
     *
     * @return The size of the text container.
     */
    uint64_t size() const {
        if (local_data()) {
            return strlen();
        }
        return std::bit_cast<const size_type *>(&data[0])[1];
    }

    /**
     * @brief Returns a pointer to the C-style string.
     *
     * This function checks if the data is stored in the local buffer. If so, it returns
     * the local buffer pointer. Otherwise, it retrieves the pointer from the local buffer.
     *
     * @return char* A pointer to the C-style string.
     */
    value_type *c_str() {
        if (local_data()) {
            return data;
        }
        return get_pointer();
    }

    /**
     * @brief Returns a constant pointer to the C-style string.
     *
     * This function checks if the data is stored in the local buffer. If so, it returns
     * the local buffer pointer. Otherwise, it retrieves the pointer from the local buffer.
     *
     * @return const char* A constant pointer to the C-style string.
     */
    const value_type *c_str() const {
        if (local_data()) {
            return data;
        }
        return get_pointer();
    }

    /**
     * @brief Returns the capacity of the container.
     *
     * This function checks if the data is stored in the local buffer. If so, it returns
     * the fixed capacity (N - 1). Otherwise, it retrieves the capacity from the local buffer.
     *
     * @return uint64_t The capacity of the container.
     */
    uint64_t capacity() const {
        if (local_data()) {
            return N - 1;
        }
        return std::bit_cast<const size_type *>(&data[0])[2];
    }

    /**
     * @brief Sets the size of the container to the specified new size.
     *
     * This function updates the size of the container. If the data is not
     * stored in the local buffer, it sets the size in the local buffer.
     *
     * @param new_size The new size to set for the container.
     */
    void set_size(const size_type new_size) {
        if (!local_data()) {
            std::bit_cast<size_type *>(&data[0])[1] = new_size;
        }
    }

    /**
     * @brief Sets the capacity of the container.
     *
     * This function updates the capacity of the container to the specified
     * new capacity. If the data is not stored in the local buffer, it
     * modifies the capacity stored in the local buffer.
     *
     * @param new_capacity The new capacity to set for the container.
     */
    void set_capacity(const size_type new_capacity) {
        if (!local_data()) {
            std::bit_cast<size_type *>(&data[0])[2] = new_capacity;
        }
    }

    /**
     * @brief Reserves memory for the container.
     *
     * This function ensures that the container has at least the specified capacity.
     * If the requested capacity is greater than the current capacity, it allocates
     * new memory and copies the existing data to the new memory location.
     * @note This is the only place where the container can change from local to heap.
     * @note For Reserve less than N this is no-op.
     *
     * @param Reserve The desired capacity to reserve.
     */
    void reserve(uint64_t Reserve) {
        if (Reserve > capacity()) {
            Reserve = next_multiple_of_16(Reserve + 1);
            if (local_data()) {
                const size_type old_size = size();
                value_type temp[N];
                std::memcpy(temp, data, N);
                set_heap();
                get_pointer() = allocator().malloc(Reserve);
                std::memset(get_pointer(), 0, Reserve);
                std::memcpy(get_pointer(), temp, old_size);
                set_size(old_size);
            } else {
                get_pointer() = allocator().realloc(get_pointer(), Reserve, capacity() + 1);
                std::memset(get_pointer() + capacity(), 0, Reserve - capacity());
            }
            set_capacity(Reserve - 1);
        }
    }

    /**
     * @brief Resizes the container to the specified new size.
     *
     * This function resizes the container to the specified new size. If the new size
     * is greater than the current size, it appends null characters to the container.
     * If the new size is less than the current size, it truncates the container.
     *
     * @param new_size The new size to set for the container.
     */
    void resize(size_type new_size) {
        if (const size_type old_size = size(); new_size > old_size) {
            reserve(new_size);
            std::memset(c_str() + old_size, 0, new_size - old_size);
        } else {
            std::memset(c_str() + new_size, 0, old_size);
        }
        set_size(new_size);
    }

    /**
     * @brief Constructs a text object from a C-style string.
     *
     * This constructor initializes the text object using the provided C-style string.
     * It sets the internal buffer to the local buffer, calculates the length of the
     * input string, reserves the necessary space, copies the string into the buffer,
     * and updates the size of the text object.
     *
     * @param str The C-style string to initialize the text object with.
     */
    constexpr char_string(const value_type *str) : container_base<Allocator>() {
        std::memset(data, 0, N);
        if (!str) {
            return;
        }
        const uint64_t len = std::strlen(str);
        reserve(len);
        if (local_data()) {
            std::memcpy(data, str, len + 1);
        } else {
            std::memcpy(get_pointer(), str, len + 1);
        }
        set_size(len);
    }

    /**
     * @brief Constructs a text object from a single character.
     *
     * This constructor initializes the text object with a single character.
     * It sets the internal buffer to the local buffer, reserves the necessary space,
     * copies the character into the buffer, and updates the size of the text object.
     *
     * @param c The character to initialize the text object with.
     */
    constexpr char_string(const value_type c) : container_base<Allocator>() {
        std::memset(data, 0, N);
        data[0] = c;
    }

    /**
     * @brief Destructor for the text class.
     *
     * This inline destructor releases the allocated memory for the text object.
     * If the text object does not use local data, it frees the allocated memory
     * using the allocator's free method.
     */
    ~char_string() {
        if (!local_data()) {
            allocator().free(get_pointer(), capacity() + 1);
        }
        std::memset(data, 0, N);
    }

    /**
     * @brief Constructs a text object from another text object.
     *
     * This constructor initializes the text object using another text object.
     * It sets the internal buffer to the local buffer, reserves the necessary
     * space, copies the string into the buffer, and updates the size of the text object.
     *
     * @param str The text object to initialize the text object with.
     */
    constexpr char_string(const char_string &str) : container_base<Allocator>(std::allocator_traits<Allocator>::select_on_container_copy_construction(str.allocator())) {
        std::memset(data, 0, N);
        reserve(str.size());
        value_type *data_ptr = local_data() ? data : get_pointer();
        const value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
        std::memcpy(data_ptr, src_ptr, str.size() + 1);
        set_size(str.size());
    }

    /**
     * @brief Constructs a text object by moving another text object.
     *
     * This constructor initializes the text object by moving another text object.
     * It sets the internal buffer to the local buffer, copies the string into the buffer,
     * and updates the size of the text object. It also updates the capacity and local_data
     * flags based on the input text object.
     *
     * @param str The text object to move from.
     */
    char_string(char_string &&str) noexcept (std::allocator_traits<Allocator>::is_always_equal::value ||
                                            std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value)
        : container_base<Allocator>((std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<Allocator>::is_always_equal::value)
                                        ? std::move(str.allocator())
                                        : Allocator{}) {
        using traits = std::allocator_traits<Allocator>;
        if (this != &str) {
            if constexpr (traits::propagate_on_container_move_assignment::value || traits::is_always_equal::value) {
                std::memcpy(data, str.data, N);
                std::memset(str.data, 0, N);
            } else {
                append(str.c_str());
            }
        }
    }

    /**
     * @brief Assigns a C-style string to the text object.
     *
     * This method assigns a C-style string to the text object. It calculates the length
     * of the input string, reserves the necessary space, copies the string into the buffer,
     * and updates the size of the text object.
     *
     * @param str The C-style string to assign to the text object.
     * @return text& A reference to the modified text object.
     */
    char_string &operator=(const char_string &str) {
        if (this != &str) {

            using ATraits = std::allocator_traits<Allocator>;
            using Base = container_base<Allocator>;

            if constexpr (ATraits::propagate_on_container_copy_assignment::value) {
                static_cast<Base &>(*this) = str;
            }

            if (!local_data()) {
                allocator().free(get_pointer(), capacity() + 1);
            }
            std::memset(data, 0, N); // unsets the heap flag
            resize(str.size());
            value_type *data_ptr = local_data() ? data : get_pointer();
            const value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
            std::memcpy(data_ptr, src_ptr, str.size() + 1);
        }
        return *this;
    }

    /**
     * @brief Converts a given value to a text representation with optional decimal places.
     *
     * This function template converts a value of type T to a text object. It supports
     * integral and floating-point types. For integral types, the value is formatted as
     * a long or unsigned long. For floating-point types, the value is formatted with
     * a specified number of decimal places.
     *
     * @tparam T The type of the value to be converted. Supported types include integral
     *           and floating-point types.
     * @param value The value to be converted to text.
     * @param decimal_places The number of decimal places to include in the text representation
     *                       for floating-point values. Default is 5.
     * @return text The text representation of the given value.
     */
    template <typename T> static char_string to_string(T value, int32_t decimal_places = 5) {
        char_string result;
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_signed_v<T>) {
                result.format("%ld", static_cast<long>(value));
            } else {
                result.format("%lu", static_cast<unsigned long>(value));
            }
        } else if constexpr (std::is_floating_point_v<T>) {
            if constexpr (std::is_same_v<T, float>) {
                result.format("%.*f", decimal_places, value);
            } else if constexpr (std::is_same_v<T, double>) {
                result.format("%.*lf", decimal_places, value);
            } else {
                result.format("%.*Lf", decimal_places, value);
            }
        }
        return result;
    }

    /**
     * @brief Assigns a C-style string to the text object.
     *
     * This method assigns a C-style string to the text object. It calculates the length
     * of the input string, reserves the necessary space, copies the string into the buffer,
     * and updates the size of the text object.
     *
     * @param str The C-style string to assign to the text object.
     * @return text& A reference to the modified text object.
     */
    char_string &operator=(char_string &&str) noexcept {
        if (this != &str) {
            if constexpr (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<Allocator>::is_always_equal::value) {
                allocator() = std::move(str.allocator());
                // This is a move
                std::memcpy(data, str.data, N);
                std::memset(str.data, 0, N);
            } else {
                allocator().free(get_pointer(), capacity() + 1);
                std::memset(data, 0, N); // unsets the heap flag
                // If the allocator is not propagate let the destructor free the old data
                append(str.c_str());
            }
        }
        return *this;
    }

    /**
     * @brief Appends a character to the end of the string.
     *
     * This function adds the specified character to the end of the string
     * by delegating the operation to the append method.
     *
     * @param c The character to append to the string.
     */
    void push_back(const value_type c) { append(c); }

    /**
     * @brief Assignment operator for the text class template.
     *
     * This operator assigns the contents of another text object to the current
     * text object. It handles memory allocation and deallocation as needed.
     *
     * @tparam U The size of the text object being assigned.
     * @param str The text object to be assigned to the current object.
     * @return A reference to the current text object after assignment.
     */
    template <int32_t U> char_string &operator=(const char_string<U> &str) {
        if (this != &str) {
            if (!local_data()) {
                allocator().free(get_pointer(), capacity() + 1);
            }
            std::memset(data, 0, N); // unsets the heap flag
            reserve(str.size());
            value_type *data_ptr = local_data() ? data : get_pointer();
            value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
            std::memcpy(data_ptr, src_ptr, str.size() + 1);
            set_size(str.size());
        }
        return *this;
    }

    /**
     * @brief Returns the size of the text object.
     *
     * This method returns the size of the text object, which is the number of characters
     * in the string without the null terminator.
     *
     * @return uint64_t The size of the text object.
     */
    bool empty() const { return size() == 0; }

    /**
     * @brief Clears the text object.
     *
     * This method clears the text object by setting the size to 0 and the first character
     * of the buffer to the null terminator.
     */
    void clear() {
        set_size(0);
        value_type *data_ptr = local_data() ? data : get_pointer();
        data_ptr[0] = '\0';
    }

    /**
     * @brief Appends a null-terminated string to the existing character sequence.
     *
     * This function appends the content of the provided null-terminated string to the
     * current object. It adjusts the size of the object accordingly and ensures the
     * appended data is properly copied into the underlying buffer.
     *
     * @param str Pointer to the null-terminated string to append.
     * @return size_type The new size of the object after the append operation.
     */
    size_type append(const value_type *str) {
        const uint64_t len = std::strlen(str);
        const uint64_t old_size = size();
        resize(size() + len);
        value_type *data_ptr = local_data() ? data : get_pointer();
        std::memcpy(data_ptr + old_size, str, len + 1);
        return size();
    }

    /**
     * @brief Appends a character to the end of the text container.
     *
     * This function reserves additional space if necessary, appends the given
     * character to the end of the container, updates the size, and ensures the
     * text is null-terminated.
     *
     * @param c The character to append.
     * @return The new size of the text container.
     */
    size_type append(value_type c) {
        reserve(size() + 1);
        const size_type old_size = size();
        value_type *data_ptr = local_data() ? data : get_pointer();
        data_ptr[old_size] = c;
        set_size(old_size + 1);
        data_ptr[old_size + 1] = '\0';
        return size();
    }

    /**
     * @brief Appends another text container to the end of this text container.
     *
     * This function reserves additional space if necessary, appends the given
     * text container to the end of this container, updates the size, and ensures
     * the text is null-terminated.
     *
     * @param str The text container to append.
     * @return The new size of the text container.
     */
    size_type append(char_string &str) {
        reserve(size() + str.size());
        value_type *data_ptr = local_data() ? data : get_pointer();
        value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
        std::memcpy(data_ptr + size(), src_ptr, str.size() + 1);
        set_size(size() + str.size());
        data_ptr[size()] = '\0';
        return size();
    }

    /**
     * @brief Appends another text container to the end of this text container.
     *
     * This function reserves additional space if necessary, appends the given
     * text container to the end of this container, updates the size, and ensures
     * the text is null-terminated.
     *
     * @param str The text container to append.
     * @return The new size of the text container.
     */
    template <int32_t U> size_type append(const char_string<U> &str) {
        reserve(size() + str.size());
        value_type *data_ptr = local_data() ? data : get_pointer();
        const value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
        std::memcpy(data_ptr + size(), src_ptr, str.size() + 1);
        set_size(size() + str.size());
        data_ptr[size()] = '\0';
        return size();
    }

    /**
     * @brief Returns the character at the specified index.
     *
     * @param i The index of the character.
     * @return The character at the specified index.
     */
    value_type operator[](size_type i) const { return local_data() ? data[i] : get_pointer()[i]; }

    /**
     * @brief Returns a reference to the character at the specified index.
     *
     * @param i The index of the character.
     * @return A reference to the character at the specified index.
     */
    value_type &operator[](size_type i) { return local_data() ? data[i] : get_pointer()[i]; }

    /**
     * @brief Extracts a substring from the text.
     *
     * This function creates a new text object containing the substring
     * from the specified start index to the end index.
     *
     * @param start The starting index of the substring (inclusive).
     * @param end The ending index of the substring (exclusive).
     * @return A new text object containing the specified substring.
     */
    char_string substr(const size_type start, const size_type end) const {
        char_string result;
        result.reserve(end - start);
        const value_type *data_ptr = local_data() ? data : get_pointer();
        value_type *result_ptr = result.local_data() ? result.data : result.get_pointer();
        std::memcpy(result_ptr, data_ptr + start, end - start);
        result.set_size(end - start);
        result_ptr[end - start] = '\0';
        return result;
    }

    /**
     * @brief Extracts a substring from the text.
     *
     * This function creates a new text object containing the substring
     * from the specified start index to the end index.
     *
     * @param start The starting index of the substring (inclusive).
     * @return A new text object containing the specified substring.
     */
    char_string substr(const size_type start) const {
        char_string result;
        result.reserve(size() - start);
        const value_type *data_ptr = local_data() ? data : get_pointer();
        value_type *result_ptr = result.local_data() ? result.data : result.get_pointer();
        std::memcpy(result_ptr, data_ptr + start, size() - start);
        result.set_size(size() - start);
        result_ptr[size() - start] = '\0';
        return result;
    }

    /**
     * @brief Finds the first occurrence of a substring in the text.
     *
     * This function searches for the first occurrence of the specified
     * substring in the text and returns the index of the first character
     * of the substring if found, or -1 if not found.
     *
     * @param str The substring to search for.
     * @return The index of the first occurrence of the substring, or -1 if not found.
     */
    size_type find(const value_type *str) const {
        if (const uint64_t len = std::strlen(str); len <= size()) {
            const value_type *data_ptr = local_data() ? data : get_pointer();
            for (uint64_t i = 0; i <= size() - len; ++i) {
                if (std::strncmp(data_ptr + i, str, len) == 0) {
                    return i;
                }
            }
        }
        return npos;
    }

    /**
     * @brief Finds the first occurrence of a character in the text.
     *
     * This function searches for the first occurrence of the specified character
     * in the text and returns the index of the first occurrence if found, or -1 if not found.
     *
     * @param c The character to search for.
     * @return The index of the first occurrence of the character, or -1 if not found.
     */
    size_type find(const value_type c) const {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        for (uint64_t i = 0; i <= size(); ++i) {
            if (data_ptr[i] == c) {
                return i;
            }
        }
        return npos;
    }

    /**
     * @brief Inserts a C-style string into the text at the specified index.
     *
     * This function inserts the specified C-style string into the text at the
     * specified index. It reserves additional space if necessary, moves the existing
     * characters to make room for the new string, copies the new string into the buffer,
     * updates the size, and ensures the text is null-terminated.
     *
     * @param index The index at which to insert the string.
     * @param str The C-style string to insert.
     */
    void insert(const int32_t index, const value_type *str) {
        const auto len = strlen(str);
        reserve(size() + len);
        value_type *data_ptr = local_data() ? data : get_pointer();
        std::memmove(data_ptr + index + len, data_ptr + index, size() - index);
        std::memcpy(data_ptr + index, str, len);
        set_size(size() + len);
        data_ptr[size()] = '\0';
    }

    /**
     * @brief Inserts a character into the text at the specified index.
     *
     * This function inserts the specified character into the text at the
     * specified index. It reserves additional space if necessary, moves the existing
     * characters to make room for the new character, inserts the new character into the buffer,
     * updates the size, and ensures the text is null-terminated.
     *
     * @param index The index at which to insert the character.
     * @param c The character to insert.
     */
    void insert(const int32_t index, value_type c) {
        reserve(size() + 1);
        value_type *data_ptr = local_data() ? data : get_pointer();
        std::memmove(data_ptr + index + 1, data_ptr + index, size() - index);
        data_ptr[index] = c;
        set_size(size() + 1);
        data_ptr[size()] = '\0';
    }

    /**
     * @brief Inserts a text object into the text at the specified index.
     *
     * This function inserts the specified text object into the text at the
     * specified index. It reserves additional space if necessary, moves the existing
     * characters to make room for the new text object, copies the new text object into the buffer,
     * updates the size, and ensures the text is null-terminated.
     *
     * @param index The index at which to insert the text object.
     * @param str The text object to insert.
     */
    void insert(const int32_t index, const char_string &str) {
        reserve(size() + str.size());
        const value_type *data_ptr = local_data() ? data : get_pointer();
        const value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
        std::memmove(data_ptr + index + str.size(), data_ptr + index, size() - index);
        std::memcpy(data_ptr + index, src_ptr, str.size());
        set_size(size() + str.size());
        data_ptr[size()] = '\0';
    }

    /**
     * @brief Inserts a text object into the text at the specified index.
     *
     * This function inserts the specified text object into the text at the
     * specified index. It reserves additional space if necessary, moves the existing
     * characters to make room for the new text object, copies the new text object into the buffer,
     * updates the size, and ensures the text is null-terminated.
     *
     * @param index The index at which to insert the text object.
     * @param str The text object to insert.
     */
    template <int32_t U> void insert(const int32_t index, const char_string<U> &str) {
        reserve(size() + str.size());
        const value_type *data_ptr = local_data() ? data : get_pointer();
        const value_type *src_ptr = str.local_data() ? str.data : str.get_pointer();
        std::memmove(data_ptr + index + str.size(), data_ptr + index, size() - index);
        std::memcpy(data_ptr + index, src_ptr, str.size());
        set_size(size() + str.size());
        data_ptr[size()] = '\0';
    }

    /**
     * @brief Finds the last occurrence of a character in the text.
     *
     * This function searches for the last occurrence of the specified character
     * in the text and returns the index of the last occurrence if found, or -1 if not found.
     *
     * @param c The character to search for.
     * @return The index of the last occurrence of the character, or -1 if not found.
     */
    size_type find_first_of(const value_type c) {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        for (size_type i = 0; i <= size(); ++i) {
            if (data_ptr[i] == c) {
                return i;
            }
        }
        return npos;
    }

    /**
     * @brief Finds the last occurrence of a character in the text.
     *
     * This function searches for the last occurrence of the specified character
     * in the text and returns the index of the last occurrence if found, or -1 if not found.
     *
     * @param c The character to search for.
     * @return The index of the last occurrence of the character, or -1 if not found.
     */
    size_type find_last_of(value_type c) {
        value_type const *data_ptr = local_data() ? data : get_pointer();
        for (uint64_t i = size() + 1; i > 0; --i) {
            if (data_ptr[i - 1] == c) {
                return i - 1;
            }
        }
        return npos;
    }

    /**
     * @brief Erases a portion of the text.
     *
     * This function erases a portion of the text starting at the specified index
     * and removing the specified number of characters. If the start index is greater
     * than or equal to the size of the text, or the start index plus the count is greater
     * than the size of the text, the function does nothing.
     *
     * @param start The starting index of the portion to erase.
     * @param count The number of characters to erase.
     */
    void erase(int32_t start, int32_t count) {
        if (start >= size()) {
            return;
        }
        if (start + count > size()) {
            count = size() - start;
        }
        value_type *data_ptr = local_data() ? data : get_pointer();
        std::memmove(data_ptr + start, data_ptr + start + count, size() - count - start);
        data_ptr[size() - count] = '\0';
        set_size(size() - count);
    }

    /**
     * @brief Concatenates a C-style string to the current text object.
     *
     * This operator allows for the addition of a C-style string to the current
     * text object, resulting in a new text object that contains the original
     * text followed by the provided string.
     *
     * @param str The C-style string to be concatenated to the current text object.
     * @return A new text object containing the concatenated result.
     */
    char_string operator+(const value_type *str) {
        char_string result = *this;
        result.append(str);
        return result;
    }

    /**
     * @brief Overloads the + operator to append a character to the text object.
     *
     * This function creates a new text object by appending the given character
     * to the current text object. The original text object remains unchanged.
     *
     * @param c The character to be appended to the text object.
     * @return A new text object with the character appended.
     */
    char_string operator+(value_type c) {
        char_string result = *this;
        result.append(c);
        return result;
    }

    char_string prepend(value_type c) {
        char_string result;
        result.reserve(size() + 1);
        result.append(c);
        result.append(*this);
        return result;
    }

    /**
     * @brief Concatenates the given text object with the current text object.
     *
     * This operator overloads the '+' operator to concatenate the current text
     * object with the provided text object. The result is a new text object
     * containing the combined contents of both text objects.
     *
     * @param str The text object to be concatenated with the current text object.
     * @return text A new text object containing the concatenated result.
     */
    char_string operator+(char_string &str) {
        char_string result = *this;
        result.append(str);
        return result;
    }

    /**
     * @brief Concatenates the given text object with the current text object.
     *
     * This operator overloads the '+' operator to concatenate the current text
     * object with the provided text object. The result is a new text object
     * containing the combined contents of both text objects.
     *
     * @param str The text object to be concatenated with the current text object.
     * @return text A new text object containing the concatenated result.
     */
    template <int32_t U> char_string operator+(const char_string<U> &str) {
        char_string result = *this;
        result.append(str);
        return result;
    }

    /**
     * @brief Appends a C-string to the current text object.
     *
     * This operator overload allows for the concatenation of a C-string to the
     * current text object using the += operator.
     *
     * @param str The C-string to be appended to the current text object.
     * @return A reference to the current text object after the C-string has been appended.
     */
    char_string &operator+=(const value_type *str) {
        append(str);
        return *this;
    }

    /**
     * @brief Overloads the operator to perform a specific operation.
     *
     * This method defines the behavior of an operator when applied to the class,
     * enabling customized operations suited to the class's functionality.
     *
     * @return ResultType The result of the operator operation.
     */
    template <int32_t U> char_string friend operator+(const value_type *str, const char_string<U> &txt) {
        char_string result(str);
        result.append(txt);
        return result;
    }

    /**
     * @brief Concatenates a text object with a C-style string.
     *
     * This function creates a new text object by appending a C-style string to an existing
     * text object. The result is returned as a new instance.
     *
     * @param txt The existing text object to concatenate.
     * @param str The C-style string to append to the text object.
     * @return text A new text object containing the concatenated result.
     */
    template <int32_t U> char_string friend operator+(const char_string<U> &txt, const value_type *str) {
        char_string result(txt);
        result.append(str);
        return result;
    }

    /**
     * @brief Concatenates a character with a text object.
     *
     * Creates a new text object by appending the given text object
     * to a text object initialized with the character.
     *
     * @param c The character to initialize the new text object.
     * @param txt The text object to be appended.
     * @return text A new text object representing the concatenation.
     */
    template <int32_t U> char_string friend operator+(value_type c, const char_string<U> &txt) {
        char_string result(c);
        result.append(txt);
        return result;
    }

    /**
     * @brief Concatenates a character to the end of a text object.
     *
     * Creates a new text object by appending the specified character to the provided text object.
     *
     * @param txt A constant reference to the text object to which the character will be added.
     * @param c The character to append to the text object.
     * @return text The newly created text object containing the original content with the appended character.
     */
    template <int32_t U> char_string friend operator+(const char_string<U> &txt, value_type c) {
        char_string result(txt);
        result.append(c);
        return result;
    }

    /**
     * @brief Appends a character to the text object.
     *
     * This operator overload allows for the addition of a single character
     * to the current text object. It appends the character to the end of the
     * text and returns a reference to the modified text object.
     *
     * @param c The character to be appended.
     * @return A reference to the modified text object.
     */
    char_string &operator+=(value_type c) {
        append(c);
        return *this;
    }

    /**
     * @brief Overloads the += operator to append the given text to the current text.
     *
     * This operator allows for the concatenation of another text object to the current
     * text object using the += syntax. It appends the content of the provided text
     * object to the current text object and returns a reference to the current text object.
     *
     * @param str The text object to be appended to the current text object.
     * @return text& A reference to the current text object after appending the given text.
     */
    char_string &operator+=(char_string &str) {
        append(str);
        return *this;
    }

    /**
     * @brief Counts the occurrences of a character in the text.
     *
     * This function counts the number of occurrences of the specified character
     * in the text and returns the count.
     *
     * @param c The character to count.
     * @return The number of occurrences of the character in the text.
     */
    uint64_t count(value_type c) {
        uint64_t count = 0;
        const value_type *data_ptr = local_data() ? data : get_pointer();
        for (uint64_t i = 0; i < size(); ++i) {
            if (data_ptr[i] == c) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Implicit conversion operator to a constant pointer of value_type.
     *
     * This operator allows the object to be implicitly converted to a constant
     * pointer of value_type, providing direct access to the underlying data.
     *
     * @return A constant pointer to the underlying data.
     */
    operator const value_type *() const {
        if (local_data()) [[likely]] {
            return data;
        }
        return get_pointer();
    }

    /**
     * @brief Returns an iterator to the beginning of the container.
     *
     * This function provides access to the first element in the container.
     * It returns a pointer to the first element, allowing iteration from the start.
     *
     * @return value_type* Pointer to the first element in the container.
     */
    value_type *begin() {
        if (local_data()) [[likely]] {
            return data;
        }
        return get_pointer();
    }

    /**
     * @brief Returns a pointer to the end of the container.
     *
     * This function provides an iterator-like pointer to the position
     * one past the last element in the container. It can be used to
     * iterate over the container in conjunction with the `begin()` function.
     *
     * @return A pointer to the position one past the last element in the container.
     */
    value_type *end() {
        if (local_data()) [[likely]] {
            return data + size();
        }
        return get_pointer() + size();
    }

    /**
     * @brief Returns a pointer to the beginning of the data.
     *
     * This function provides a constant pointer to the first element of the data array.
     *
     * @return A constant pointer to the first element of the data array.
     */
    const value_type *begin() const {
        if (local_data()) [[likely]] {
            return data;
        }
        return get_pointer();
    }

    /**
     * @brief Returns a pointer to the end of the text container.
     *
     * This function provides a pointer to the position just past the last
     * character in the text container. It is useful for iterating over the
     * container using pointer arithmetic.
     *
     * @return A pointer to the position just past the last character in the container.
     */
    const value_type *end() const {
        if (local_data()) [[likely]] {
            return data + size();
        }
        return get_pointer() + size();
    }

    /**
     * @brief Reverses the text container.
     *
     * This function creates a new text object with the characters of the
     * current text object in reverse order. It returns the reversed text object.
     *
     * @return A new text object with the characters in reverse order.
     */
    char_string reversed() {
        char_string result;
        result.resize(size());
        const value_type *data_ptr = local_data() ? data : get_pointer();
        const value_type *result_ptr = result.local_data() ? result.data : result.get_pointer();
        for (uint64_t i = 0; i < size(); ++i) { result_ptr[size() - i - 1] = data_ptr[i]; }
        result_ptr[size()] = '\0';
        return result;
    }

    /**
     * @brief Checks if the text is a number.
     *
     * This function checks if the text contains only numeric characters.
     * It returns true if all characters in the text are numeric, and false otherwise.
     *
     * @return true if the text is a number, false otherwise.
     */
    bool is_number() {
        auto is_digit_or_dot = [](const value_type c) { return c >= '0' && c <= '9' || c == '.'; };
        const value_type *data_ptr = local_data() ? data : get_pointer();
        for (uint64_t i = 0; i < size(); ++i) {
            if (!is_digit_or_dot(data_ptr[i])) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Converts the text to an integer.
     *
     * This function converts the text to an integer using `std::strtol`.
     * If the text is not a valid integer, it returns 0.
     *
     * @return The integer value of the text.
     */
    double to_double() {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        return std::strtod(data_ptr, nullptr);
    }

    /**
     * @brief Converts the text to an integer.
     *
     * This function converts the text to an integer using `std::strtol`.
     * If the text is not a valid integer, it returns 0.
     *
     * @return The integer value of the text.
     */
    int32_t to_int() const {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        const auto result = std::strtol(data_ptr, nullptr, 10);
        if (errno == ERANGE) {
            return static_cast<int32_t>(LONG_MAX);
        }
        return static_cast<int32_t>(result);
    }

    /**
     * @brief Converts the text to a long integer.
     *
     * This function converts the text to a long integer using `std::strtol`.
     * If the text is not a valid long integer, it returns 0.
     *
     * @return The long integer value of the text.
     */
    long to_long() {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        return std::strtol(data_ptr, nullptr, 10);
    }

    /**
     * @brief Converts the text to a long long integer.
     *
     * This function converts the text to a long long integer using `std::strtoll`.
     * If the text is not a valid long long integer, it returns 0.
     *
     * @return The long long integer value of the text.
     */
    long long to_long_long() {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        return std::strtoll(data_ptr, nullptr, 10);
    }

    /**
     * @brief Converts the text to a float.
     *
     * This function converts the text to a float using `std::strtof`.
     * If the text is not a valid float, it returns 0.
     *
     * @return The float value of the text.
     */
    float to_float() {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        return std::strtof(data_ptr, nullptr);
    }

    /**
     * @brief Converts all characters in the text to uppercase.
     *
     * This function creates a new text object with all characters converted to their
     * uppercase equivalents. The size of the new text object will be the same as the
     * original.
     *
     * @return A new text object with all characters in uppercase.
     */
    char_string upper() {
        char_string result;
        result.resize(size());
        value_type const *data_ptr = local_data() ? data : get_pointer();
        value_type *result_ptr = result.local_data() ? result.data : result.get_pointer();
        for (uint64_t i = 0; i < size(); ++i) { result_ptr[i] = static_cast<char>(std::toupper(data_ptr[i])); }
        result_ptr[size()] = '\0';
        return result;
    }

    /**
     * Converts all characters in the text to lowercase.
     *
     * @return A new text object with all characters converted to lowercase.
     */
    char_string lower() {
        char_string result;
        result.resize(size());
        value_type const *data_ptr = local_data() ? data : get_pointer();
        value_type *result_ptr = result.local_data() ? result.data : result.get_pointer();
        for (uint64_t i = 0; i < size(); ++i) { result_ptr[i] = static_cast<char>(std::tolower(data_ptr[i])); }
        result_ptr[size()] = '\0';
        return result;
    }

    /**
     * @brief Slices the text at the specified index.
     *
     * This function truncates the text by inserting a null terminator ('\0')
     * at the specified index, effectively reducing the size of the text to
     * the given index.
     *
     * @param index The position at which to slice the text. If the index is
     *              greater than or equal to the current size, the function
     *              does nothing.
     */
    void slice(const size_type index) {
        if (index != npos && index < size()) {
            resize(index);
        }
    }

    /**
     * @brief Returns the last character in the text.
     *
     * This function returns the last character in the text, or 0 if the text is empty.
     *
     * @return The last character in the text.
     */
    value_type last() const {
        if (size())
            return operator[](size() - 1);
        return 0;
    }

    /**
     * Appends a path component to the current text object.
     *
     * This operator appends the provided string to the text object, ensuring a proper path
     * format by inserting the path separator if it is not already present as the last character.
     * The process works as follows:
     *   1. If the current text does not end with the path separator, the separator is appended.
     *   2. The provided string is then appended to the text.
     *   3. The combined result is added to the text using the '+=' operator.
     *
     * @param str Pointer to a null-terminated string representing the path component to append.
     * @return The updated text object after appending the path separator (if needed) and the string.
     */
    char_string operator/=(const value_type *str) {
        char_string to_append;
        if (last() != path_separator()) {
            to_append.append(path_separator());
        }
        to_append.append(str);
        return operator+=(to_append);
    }

    /**
     * Operator/= appends the given text to the current text object, ensuring that a path separator
     * is present between the existing content and the new text.
     *
     * If the current text does not end with a path separator, one is added before appending the provided text.
     *
     * @param str The text to be appended.
     * @return The updated text object after appending, effectively combining the original text, the
     *         path separator (if needed), and the new text using the operator+=.
     */
    char_string operator/=(const char_string &str) {
        char_string to_append;
        if (last() != path_separator()) {
            to_append.append(path_separator());
        }
        to_append.append(str);
        return operator+=(to_append);
    }

    /**
     * @brief Appends a character to the text, inserting a path separator if necessary.
     *
     * This operator overload appends the provided character to the current text object.
     * If the last character of the text is not a path separator, a path separator is inserted before appending the character.
     * The result is then combined with the existing text using the operator+= mechanism.
     *
     * @param c The character to be appended to the text.
     * @return A new text object reflecting the updated content with the appended character.
     */
    char_string operator/=(value_type c) {
        char_string to_append;
        if (last() != path_separator()) {
            to_append.append(path_separator());
        }
        to_append.append(c);
        return operator+=(to_append);
    }

    /**
     * @brief Appends a C-style string to the text object with proper path separation.
     *
     * This operator takes a pointer to a null-terminated C-string and appends it to
     * the existing text object. If the current text does not end with a path separator,
     * one is appended before the given string to ensure correct path formatting.
     *
     * @param str Pointer to a null-terminated character array representing the string to be appended.
     * @return A new text object resulting from the concatenation of the original text and the given C-string.
     */
    char_string operator/(const value_type *str) {
        char_string to_append;
        if (last() != path_separator()) {
            to_append.append(path_separator());
        }
        to_append.append(str);
        return operator+(to_append);
    }

    /**
     * @brief Appends a given text to the current text ensuring a trailing path separator.
     *
     * If the current text does not end with a path separator, one is appended before
     * appending the user-supplied text. The operation is performed by internally calling
     * the operator+= after constructing the modified text.
     *
     * @param str The text to append after the path separator.
     * @return text The resulting text object after appending.
     */
    char_string operator/(const char_string &str) {
        char_string to_append;
        if (last() != path_separator()) {
            to_append.append(path_separator());
        }
        to_append.append(str);
        return operator+=(to_append);
    }

    /**
     * @brief Appends a character to the current text with proper path separation.
     *
     * This operator appends the character 'c' to the text instance. If the text
     * does not already end with the designated path separator, it appends the
     * separator first to ensure the text maintains proper file path formatting.
     *
     * @param c The character to append.
     * @return A new text instance with the appropriate path separator (if needed)
     *         followed by the character 'c'.
     */
    char_string operator/(value_type c) {
        char_string to_append;
        if (last() != path_separator()) {
            to_append.append(path_separator());
        }
        to_append.append(c);
        return operator+=(to_append);
    }

    /**
     * @brief Obtains the parent directory path.
     *
     * This function searches for the last occurrence of the path separator within the current text.
     * If the separator is found, it returns a substring that contains everything before this separator,
     * representing the parent directory. If no path separator is found, an empty text is returned,
     * indicating that no valid parent path exists.
     *
     * @return A text representing the parent directory, or an empty text if the path contains no separator.
     */
    char_string parent_path() {
        if (const size_type index = find_last_of(path_separator()); index != npos) {
            return substr(0, index);
        }
        return char_string();
    }

    /**
     * @brief Equality comparison operator for the text class.
     *
     * This operator compares the current text object with another text object
     * to determine if they are equal. It returns true if the two text objects
     * have the same size and contain the same characters, and false otherwise.
     *
     * @param str The text object to compare with.
     * @return true if the text objects are equal, false otherwise.
     */
    bool operator==(const char_string &str) const { return sse_strncmp(*this, str) == 0; }

    /**
     * @brief Equality comparison operator for the text class.
     *
     * This operator compares the current text object with a C-style string
     * to determine if they are equal. It returns true if the text object
     * has the same size and contains the same characters as the C-style string,
     * and false otherwise.
     *
     * @param str The C-style string to compare with.
     * @return true if the text object is equal to the C-style string, false otherwise.
     */

    bool operator==(const value_type *str) const {
        const value_type *data_ptr = local_data() ? data : get_pointer();
        return std::strncmp(data_ptr, str, std::max(std::strlen(str), size())) == 0;
    }

    /**
     * @brief Compares this text object with another text object using the three-way comparison operator.
     *
     * @param str The text object to compare with.
     * @return A std::strong_ordering value indicating the result of the comparison:
     *         - std::strong_ordering::less if this text is less than the other text.
     *         - std::strong_ordering::equal if this text is equal to the other text.
     *         - std::strong_ordering::greater if this text is greater than the other text.
     */
    auto operator<=>(const char_string &str) const { return sse_strncmp(*this, str) <=> 0; }
};

template <int32_t N = 64> char_string<N> format(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char_string<N> result;
    result.resize(N);
    vsnprintf(result.get_pointer(), N, format, args);
    va_end(args);
    return result;
}

using string = char_string<64>;
using large_string = char_string<256>;
} // namespace gtr
#endif