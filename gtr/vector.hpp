#pragma once
#ifndef VECTOR_HPP
#define VECTOR_HPP
#include <memory>
#include "allocator.hpp"
#include "assert.hpp"
#include "container_base.hpp"

namespace gtr {
template <class T, class Allocator = c_allocator<T>> struct vector : container_base<Allocator> {
    using value_type = typename c_allocator<T>::value_type;
    using iterator = typename c_allocator<T>::pointer_type;
    using const_iterator = const value_type *;
    using allocator_type = Allocator;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = typename c_allocator<T>::size_type;
    using difference_type = typename c_allocator<T>::difference_type;
    static constexpr size_type npos = static_cast<size_type>(-1);

    T *data{nullptr};         ///< Pointer to the beginning of the allocated storage.
    T *data_end{nullptr};     ///< Pointer to one past the last constructed element.
    T *capacity_end{nullptr}; ///< Pointer to one past the end of the allocated storage.

    const Allocator &allocator() const { return container_base<Allocator>::allocator(); }
    Allocator &allocator() { return container_base<Allocator>::allocator(); }

    /**
     * @brief Returns the current number of elements in the vector.
     *
     * This function computes the size by subtracting the `data` pointer from the `data_end` pointer.
     *
     * @return size_type The number of elements in the vector.
     */
    [[nodiscard]] size_type size() const { return data_end - data; }

    /**
     * @brief Returns the capacity of the vector.
     *
     * This function computes the capacity by subtracting the `data` pointer from the `capacity_end` pointer.
     *
     * @return size_type The capacity of the vector.
     */
    [[nodiscard]] size_type capacity() const { return capacity_end - data; }

    /**
     * @brief Returns the size of the vector in bytes.
     *
     * This function calculates the total memory size occupied by the elements
     * of the vector by multiplying the number of elements (size) by the size
     * of each element (sizeof(T)).
     *
     * @return size_type The size of the vector in bytes.
     */
    [[nodiscard]] size_type size_in_bytes() const { return size() * sizeof(T); }

    /**
     * @brief Destroys all elements in the container.
     *
     * This function iterates over all elements in the container and explicitly
     * calls their destructor. It ensures that all resources held by the elements
     * are properly released. This function should be used with caution as it
     * manually invokes destructors, which is generally handled automatically
     * by the container's destructor.
     */
    template <typename U = T>
        requires(std::is_trivially_destructible_v<U>)
    static void destroy_all() {
        /** No-op for trivially destructible types}*/
    }

    // For non-trivially destructible types: call destructors.
    template <typename U = T>
        requires(!std::is_trivially_destructible_v<U>)
    void destroy_all() {
        if (data) {
            U *ptr = data;
            for (U *it = ptr; it != data_end; ++it) { it->~U(); }
        }
    }

    /**
     * @brief Frees all allocated memory for the vector.
     *
     * This function destroys all elements in the vector and then frees the memory
     * allocated for the vector's data.
     */
    void free_all() {
        destroy_all();
        this->deallocate(data, capacity());
        data = nullptr;
    }

    /**
     * @brief Default constructor for the vector class.
     *
     * This constructor initializes an empty vector with no elements.
     * It sets the internal data pointers to nullptr.
     * The allocator is also initialized using the default allocator.
     */
    vector() = default;

    /**
     * @brief Constructs a vector with a specified initial capacity.
     *
     * This constructor initializes a vector with a given capacity, allocating
     * memory for the elements but not initializing them. The size of the vector
     * is set to 0.
     *
     * @param Capacity The initial capacity of the vector.
     */
    explicit vector(size_type Capacity) : container_base<Allocator>() { resize(Capacity); }

    /**
     * @brief Constructs a vector with a specified initial capacity and value.
     *
     * This constructor initializes a vector with a given capacity and value.
     * It allocates memory for the elements, sets the size to the capacity, and
     * constructs each element in the vector using the provided value.
     *
     * @param Size The initial capacity of the vector.
     * @param value The value to initialize each element with.
     */
    vector(size_type Size, const T &value) : container_base<Allocator>(), data(allocate(Size)) {
        data_end = data + Size;
        capacity_end = data_end;
        for (size_type i = 0; i < Size; ++i) { new (data + i) T(value); }
    }

    /**
     * @brief Constructs a vector from an initializer list.
     *
     * This constructor initializes the vector with the elements provided in the
     * initializer list. It allocates memory for the elements, sets the initial
     * size to 0, and the capacity to the size of the initializer list. Each
     * element from the initializer list is then added to the vector.
     *
     * @param init_list The initializer list containing elements to initialize the vector with.
     */
    vector(std::initializer_list<T> init_list) : container_base<Allocator>(), data(allocate(init_list.size())) {
        data_end = data;
        capacity_end = data + init_list.size();
        for (const auto &item : init_list) {
            new (data_end) T(item);
            ++data_end;
        }
    }

    /**
     * @brief Copy constructor for the vector class.
     *
     * This constructor creates a new vector by copying the contents of another vector.
     * It allocates memory for the new vector, copies the size and capacity, and
     * constructs each element in the new vector by copying the corresponding element
     * from the other vector.
     *
     * @param other The vector to be copied.
     */
    vector(const vector &other)
        : container_base<Allocator>(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.allocator())), data(this->allocate(other.capacity())),
          data_end(data + other.size()), capacity_end(data + other.capacity()) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(data, other.data, other.size() * sizeof(T));
        } else {
            for (size_type i = 0; i < other.size(); ++i) new (data + i) T(other.data[i]);
        }
    }

    vector(vector &&other) noexcept(std::allocator_traits<Allocator>::is_always_equal::value || std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value)
        : container_base<Allocator>((std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<Allocator>::is_always_equal::value)
                                        ? std::move(other.allocator())
                                        : Allocator{}),
          data{other.data}, data_end{other.data_end}, capacity_end{other.capacity_end} {
        if constexpr (!std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value && !std::allocator_traits<Allocator>::is_always_equal::value) {
            T *dst = data;
            for (T *it = other.data; it != other.data_end; ++it, ++dst) new (dst) T(std::move(*it));
            other.destroy_all();
        }
        other.data = other.data_end = other.capacity_end = nullptr;
    }

    vector &operator=(const vector &other) {
        if (this == &other)
            return *this;

        using ATraits = std::allocator_traits<Allocator>;
        using Base = container_base<Allocator>;

        // 1) propagate allocator on copy‐assignment?
        if constexpr (ATraits::propagate_on_container_copy_assignment::value) {
            // call base’s copy‐op to copy the allocator
            static_cast<Base &>(*this) = other;
        }

        // 2) ensure capacity
        if (other.size() > capacity()) {
            free_all();
            data = allocate(other.capacity());
            capacity_end = data + other.capacity();
        }

        // 3) copy‐construct or memcpy elements
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(data, other.data, other.size() * sizeof(T));
        } else {
            // destroy any old elements
            destroy_all();
            // copy‐construct new ones
            for (size_type i = 0; i < other.size(); ++i) new (data + i) T(other.data[i]);
        }

        data_end = data + other.size();
        return *this;
    }

    vector &operator=(vector &&other) noexcept {
        if (this == &other)
            return *this;

        using ATraits = std::allocator_traits<Allocator>;
        using Base = container_base<Allocator>;

        // 1) can we steal allocator+buffer?
        if constexpr (ATraits::propagate_on_container_move_assignment::value || ATraits::is_always_equal::value) {
            free_all();
            // move‐assign the allocator
            static_cast<Base &>(*this) = std::move(other);
            // steal the buffer
            data = other.data;
            data_end = other.data_end;
            capacity_end = other.capacity_end;
            // leave other empty
            other.data = other.data_end = other.capacity_end = nullptr;
        } else {
            // 2) deep‐move into our own storage
            if (other.size() > capacity()) {
                free_all();
                data = allocate(other.capacity());
                capacity_end = data + other.capacity();
            }
            // destroy any old elements
            destroy_all();
            // move‐construct new ones
            T *dst = data;
            for (T *src = other.data; src != other.data_end; ++src, ++dst) new (dst) T(std::move(*src));
            data_end = data + (other.data_end - other.data);

            // clear out the source
            other.destroy_all();
            other.data = other.data_end = other.capacity_end = nullptr;
        }

        return *this;
    }

    /**
     * @brief Destructor for the vector class.
     *
     * This destructor is responsible for cleaning up the resources used by the vector.
     * If the vector contains data, it calls the _free_all() method to release all allocated memory.
     */
    ~vector() {
        if (data) {
            free_all();
        }
        data = nullptr;
        data_end = nullptr;
        capacity_end = nullptr;
    }

    /**
     * @brief Adds a new element to the end of the vector.
     *
     * This function appends the given value to the end of the vector. If the
     * current size of the vector is equal to its capacity, the capacity is
     * increased (doubled or set to 1 if it was 0) to accommodate the new element.
     *
     * @param value The value to be added to the vector.
     */
    void push_back(const T &value) {
        if (data_end == capacity_end) {
            reserve(capacity() ? capacity() * 2 : 1);
        }
        new (data_end) T(value);
        ++data_end;
    }

    /**
     * @brief Adds a new element to the end of the vector using move semantics.
     *
     * This function appends the given value to the end of the vector by moving it.
     * If the current size of the vector is equal to its capacity, the capacity is
     * increased (doubled or set to 1 if it was 0) to accommodate the new element.
     *
     * @param value The value to be moved into the vector.
     */
    void push_back(T &&value) {
        if (data_end == capacity_end) {
            reserve(capacity() ? capacity() * 2 : 1);
        }
        new (data_end) T(std::move(value));
        ++data_end;
    }

    /**
     * @brief Appends the elements of another vector to the end of the current vector.
     *
     * This function takes another vector as an argument and appends its elements
     * to the end of the current vector. If the combined size of the current vector
     * and the other vector exceeds the current capacity, the function will reserve
     * additional space to accommodate the new elements.
     *
     * @param other The vector whose elements are to be appended to the current vector.
     */
    void push_back(const vector &other) {
        if (size() + other.size() > capacity()) {
            reserve(size() + other.size());
        }
        for (size_type i = 0; i < other.size(); ++i) {
            new (data_end) T(other.data[i]);
            ++data_end;
        }
    }

    /**
     * @brief Appends the elements from another vector to the end of the current vector.
     *
     * This function takes a rvalue reference to another vector and appends its elements
     * to the current vector. If the combined size of the current vector and the other vector
     * exceeds the current capacity, the function reserves enough space to accommodate all elements.
     *
     * @param other The vector whose elements are to be appended. This vector will be moved from.
     */
    void push_back(vector &&other) {
        if (size() + other.size() > capacity()) {
            reserve(size() + other.size());
        }
        for (size_type i = 0; i < other.size(); ++i) {
            new (data_end) T(std::move(other.data[i]));
            ++data_end;
        }
    }

    /**
     * @brief Removes the last element from the vector, reducing the size by one.
     *
     * This function calls the destructor of the last element and then decreases
     * the size of the vector. It is a no-op if the vector is already empty.
     */
    void pop_back() {
        if (data_end > data) {
            --data_end;
            data_end->~T();
        }
    }

    /**
     * @brief Removes the specified number of elements from the end of the vector.
     *
     * This function destructs the elements at the end of the vector and reduces the size of the vector by the specified count.
     * If the count is greater than the current size of the vector, it will remove all elements.
     *
     * @param count The number of elements to remove from the end of the vector.
     */
    void pop_back(size_type count) {
        if (count > size()) {
            count = size();
        }
        for (size_type i = 0; i < count; ++i) {
            --data_end;
            data_end->~T();
        }
    }

    /**
     * @brief Clears the vector by destructing all elements and resetting the size to zero.
     *
     * This function iterates through the vector, calling the destructor for each element,
     * and then sets the size of the vector to zero. After calling this function, the vector
     * will be empty.
     */
    void clear() {
        while (data_end != data) {
            --data_end;
            data_end->~T();
        }
    }

    /**
     * @brief Reserves storage for at least the specified number of elements.
     *
     * This function increases the capacity of the container to at least the specified
     * number of elements. If the current capacity is already greater than or equal to
     * the specified number, no action is taken. Otherwise, the storage is reallocated
     * to accommodate the new capacity.
     *
     * @param new_capacity The minimum number of elements to reserve storage for.
     */
    void reserve(size_type new_capacity) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (new_capacity > capacity()) {
                size_type sz = size();
                data = this->reallocate(data, new_capacity, capacity());
                data_end = data + sz;
                capacity_end = data + new_capacity;
            }
        } else {
            if (new_capacity > capacity()) {
                T *new_data = this->allocate(new_capacity);
                T *new_data_end = new_data;
                for (T *ptr = data; ptr != data_end; ++ptr, ++new_data_end) {
                    new (new_data_end) T(std::move(*ptr));
                    ptr->~T();
                }
                if (data) {
                    this->deallocate(data, capacity());
                }
                data = new_data;
                data_end = new_data_end;
                capacity_end = data + new_capacity;
            }
        }
    }

    /**
     * @brief Resizes the container to contain the specified number of elements.
     *
     * If the new size is greater than the current size, additional default-constructed elements are appended.
     * If the new size is less than the current size, elements are destroyed to reduce the size.
     *
     * @param new_size The new size of the container.
     */
    void resize(size_type new_size) {
        if (new_size > size()) {
            if (new_size > capacity()) {
                // Start with the current capacity (or 1 if capacity is zero)
                size_type new_cap = (capacity() > 0) ? capacity() : 1;
                // Double the capacity until it meets or exceeds new_size
                while (new_cap < new_size) { new_cap *= 2; }
                reserve(new_cap);
            }
            // Construct new elements until the size reaches new_size
            while (data_end != data + new_size) {
                new (data_end) T();
                ++data_end;
            }
        } else if (new_size < size()) {
            // Destroy elements to reduce the container size
            while (data_end != data + new_size) {
                --data_end;
                data_end->~T();
            }
        }
    }

    /**
     * @brief Resizes the container to contain the specified number of elements.
     *
     * If the new size is greater than the current size, additional copies of the specified value are appended.
     * If the new size is less than the current size, elements are destroyed to reduce the size.
     *
     * @param new_size The new size of the container.
     * @param value The value to be appended to the container.
     */
    void resize(size_type new_size, const T &value) {
        if (new_size > size()) {
            reserve(new_size);
            while (data_end != data + new_size) {
                new (data_end) T(value);
                ++data_end;
            }
        } else if (new_size < size()) {
            while (data_end != data + new_size) {
                --data_end;
                data_end->~T();
            }
        }
    }

    /**
     * @brief Accesses the element at the specified index.
     *
     * This operator provides direct access to the element at the given index
     * in the container. It does not perform bounds checking.
     *
     * @param index The position of the element to access.
     * @return T& Reference to the element at the specified index.
     */
    T &operator[](size_type index) {
        Assert(index < size(), "Index out of bounds.");
        return data[index];
    }

    const T &operator[](size_type index) const {
        Assert(index < size(), "Index out of bounds.");
        return data[index];
    }

    /**
     * @brief Returns a pointer to the beginning of the container.
     *
     * This function provides access to the first element in the container.
     *
     * @return T* Pointer to the first element in the container.
     */
    T *begin() { return data; }
    /**
     * @brief Returns a pointer to the end of the container.
     *
     * This function provides a pointer to the element following the last element
     * of the container. This pointer can be used to iterate over the container
     * in a range-based for loop or other iteration constructs.
     *
     * @return T* Pointer to the element following the last element of the container.
     */
    T *end() { return data_end; }
    /**
     * @brief Returns a constant pointer to the beginning of the vector.
     *
     * This function provides a way to access the first element of the vector
     * in a read-only manner. It returns a constant pointer to the data stored
     * in the vector.
     *
     * @return const T* Pointer to the first element of the vector.
     */
    const T *begin() const { return data; }
    const T *end() const { return data_end; }

    /**
     * @brief Returns a reference to the first element in the vector.
     *
     * This function asserts that the vector is not empty and then returns
     * a reference to the first element in the underlying data array.
     *
     * @return T& Reference to the first element in the vector.
     * @throws Assertion failure if the vector is empty.
     */
    T &front() {
        Assert(size() > 0, "Vector is empty.");
        return *data;
    }

    /**
     * @brief Returns a reference to the last element in the vector.
     *
     * This function asserts that the vector is not empty and then returns
     * a reference to the last element in the vector.
     *
     * @return T& Reference to the last element in the vector.
     * @throws Assertion failure if the vector is empty.
     */
    T &back() {
        Assert(size() > 0, "Vector is empty.");
        return *(data_end - 1);
    }

    /**
     * @brief Returns a constant reference to the first element in the vector.
     *
     * This function asserts that the vector is not empty and then returns a
     * constant reference to the first element in the vector.
     *
     * @return A constant reference to the first element in the vector.
     * @throws Assertion failure if the vector is empty.
     */
    const T &front() const {
        Assert(size() > 0, "Vector is empty.");
        return *data;
    }

    /**
     * @brief Returns a constant reference to the last element in the vector.
     *
     * This function asserts that the vector is not empty and then returns a
     * constant reference to the last element in the vector.
     *
     * @return A constant reference to the last element in the vector.
     * @throws Assertion failure if the vector is empty.
     */
    const T &back() const {
        Assert(size() > 0, "Vector is empty.");
        return *(data_end - 1);
    }

    /**
     * @brief Constructs an element in-place at the end of the container.
     *
     * This function constructs a new element at the end of the container using the provided arguments.
     * If the current size of the container is equal to its capacity, the capacity is increased (doubled or set to 1 if it was 0).
     * The new element is constructed in-place using placement new and the provided arguments.
     *
     * @tparam Args Types of the arguments to be forwarded to the constructor of the element.
     * @param MyArgs Arguments to be forwarded to the constructor of the element.
     */
    template <typename... Args> void emplace_back(Args &&...MyArgs) {
        if (data_end == capacity_end) {
            reserve(capacity() ? capacity() * 2 : 1);
        }
        new (data_end) T(std::forward<Args>(MyArgs)...);
        ++data_end;
    }

    /**
     * @brief Reduces the capacity of the container to fit its size.
     *
     * This function reduces the capacity of the container to match its current size.
     * If the current size is less than the capacity, it reallocates the internal
     * storage to free up unused memory.
     */
    void shrink_to_fit() {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (capacity() > size()) {
                size_type sz = size();
                data = reallocate(data, sz);
                data_end = data + sz;
                capacity_end = data_end;
            }
        } else {
            if (size() < capacity()) {
                T *new_data = allocate(size());
                T *new_data_end = new_data;
                for (T *ptr = data; ptr != data_end; ++ptr, ++new_data_end) {
                    new (new_data_end) T(std::move(*ptr));
                    ptr->~T();
                }
                if (data) {
                    deallocate(data, capacity());
                }
                data = new_data;
                data_end = new_data_end;
                capacity_end = data + size();
            }
        }
    }

    /**
     * @brief Erases the element at the specified index from the vector.
     *
     * This function removes the element at the given index by calling its destructor,
     * then moves the last element to the position of the erased element. The size of the
     * vector is decreased by one. This operation is faster than erase() but does not
     * preserve the order of elements.
     *
     * @param index The index of the element to be erased.
     * @note If the index is out of bounds (greater than or equal to the current size),
     *       the function does nothing.
     */
    void erase_unordered(size_type index) {
        if (index < size()) {
            data[index].~T();
            data[index] = std::move(*(--data_end));
        }
    }

    /**
     * @brief Erases the element at the specified iterator position.
     *
     * This function removes the element at the specified iterator position by calling
     * its destructor, then moves the last element to the position of the erased element.
     * The size of the vector is decreased by one. This operation is faster than erase()
     * but does not preserve the order of elements.
     *
     * @param it The iterator pointing to the element to be erased.
     */
    void erase_unordered(iterator it) { erase_unordered(it - begin()); }

    /**
     * @brief Erases the element at the specified index from the vector.
     *
     * This function removes the element at the given index by calling its destructor,
     * then shifts all following elements one position to the left to fill the gap.
     * The size of the vector is decreased by one.
     *
     * @param index The index of the element to be erased.
     * @note If the index is out of bounds (greater than or equal to the current size),
     *       the function does nothing.
     */
    void erase(size_type index) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (index < size()) {
                memmove(data + index, data + index + 1, (size() - index - 1) * sizeof(T));
                --data_end;
            }
        } else {
            if (index < size()) {
                data[index].~T();
                for (size_type i = index; i < size() - 1; ++i) {
                    new (data + i) T(std::move(data[i + 1]));
                    data[i + 1].~T();
                }
                --data_end;
            }
        }
    }

    /**
     * @brief Erases elements from the vector within the specified range.
     *
     * This function removes elements in the range [start_index, end_index) from the vector.
     * The elements in the specified range are destroyed, and the remaining elements
     * are shifted to fill the gap. The size of the vector is reduced accordingly.
     *
     * @param start_index The starting index of the range to erase (inclusive).
     * @param end_index The ending index of the range to erase (exclusive).
     */
    template <typename U = T>
        requires(std::is_trivially_destructible_v<U>)
    void erase(size_type start_index, size_type end_index) {
        if (start_index < size() && end_index <= size() && start_index < end_index) {
            memmove(data + start_index, data + end_index, (size() - end_index) * sizeof(T));
            data_end -= end_index - start_index;
        }
    }

    template <typename U = T>
        requires(!std::is_trivially_destructible_v<U>)
    void erase(size_type start_index, size_type end_index) {
        if (start_index < size() && end_index <= size() && start_index < end_index) {
            for (size_type i = start_index; i < end_index; ++i) { data[i].~T(); }
            size_type move_count = size() - end_index;
            for (size_type i = 0; i < move_count; ++i) {
                new (data + start_index + i) T(std::move(data[end_index + i]));
                data[end_index + i].~T();
            }
            data_end -= (end_index - start_index);
        }
    }

    /**
     * @brief Erases the element at the specified iterator position.
     *
     * This function removes the element at the specified iterator position by calling
     * its destructor, then shifts all following elements one position to the left to fill the gap.
     * The size of the vector is decreased by one.
     *
     * @param it The iterator pointing to the element to be erased.
     */
    void erase(iterator it) { erase(it - begin()); }

    /**
     * @brief Erases elements from the vector within the specified range.
     *
     * This function removes elements in the range [start_it, end_it) from the vector.
     * The elements in the specified range are destroyed, and the remaining elements
     * are shifted to fill the gap. The size of the vector is reduced accordingly.
     *
     * @param start_it The starting iterator of the range to erase (inclusive).
     * @param end_it The ending iterator of the range to erase (exclusive).
     */
    void erase(iterator start_it, iterator end_it) { erase(start_it - begin(), end_it - begin()); }

    /**
     * @brief Adds a range of elements to the vector.
     *
     * This function inserts elements from the range defined by the pointers
     * `start_ptr` and `end_ptr` into the vector. If the current capacity of the vector
     * is not enough to hold the new elements, the capacity is increased.
     *
     * @param start_ptr Pointer to the beginning of the range of elements to be added.
     * @param end_ptr Pointer to the end of the range of elements to be added.
     */
    void push_range(const T *start_ptr, const T *end_ptr) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            size_type count = end_ptr - start_ptr;
            if (size() + count > capacity()) {
                reserve(size() + count);
            }
            memcpy(data_end, start_ptr, count * sizeof(T));
            data_end += count;
        } else {
            if (size_type count = end_ptr - start_ptr; size() + count > capacity()) {
                reserve(size() + count);
            }
            for (const T *ptr = start_ptr; ptr != end_ptr; ++ptr) {
                new (data_end) T(*ptr);
                ++data_end;
            }
        }
    }

    /**
     * @brief Inserts an element at the specified index in the vector.
     *
     * This function inserts the given value at the specified index in the vector.
     * If the vector's size is equal to its capacity, the capacity is doubled (or set to 1 if it was 0).
     * Elements from the specified index to the end of the vector are shifted one position to the right
     * to make space for the new element.
     *
     * @param index The position at which to insert the new element.
     * @param value The value to be inserted.
     */
    void insert(size_type index, const T &value) {
        if (index <= size()) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                push_back(value);
                std::memmove(data + index + 1, data + index, (size() - index - 1) * sizeof(T));
                data[index] = value;
            } else {
                if (data_end == capacity_end) {
                    reserve(capacity() ? capacity() * 2 : 1);
                }
                ++data_end;
                for (size_type i = size() - 1; i > index; --i) { data[i] = std::move(data[i - 1]); }
                new (&data[index]) T(value);
            }
        }
    }

    /**
     * @brief Inserts an element at the specified iterator position in the vector.
     *
     * This function inserts the given value at the specified iterator position in the vector.
     * If the vector's size is equal to its capacity, the capacity is doubled (or set to 1 if it was 0).
     * Elements from the specified iterator position to the end of the vector are shifted one position
     * to the right to make space for the new element.
     *
     * @param it The iterator position at which to insert the new element.
     * @param value The value to be inserted.
     */
    void insert(iterator it, const T &value) { insert(it - begin(), value); }

    /**
     * @brief Inserts an element at the specified index in the vector using move semantics.
     *
     * This function inserts the given value at the specified index in the vector using move semantics.
     * If the vector's size is equal to its capacity, the capacity is doubled (or set to 1 if it was 0).
     * Elements from the specified index to the end of the vector are shifted one position to the right
     * to make space for the new element.
     *
     * @param index The position at which to insert the new element.
     * @param value The value to be inserted using move semantics.
     */
    void insert(size_type index, T &&value) {
        if (index <= size()) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                push_back(std::move(value));
                std::memmove(data + index + 1, data + index, (size() - index - 1) * sizeof(T));
                data[index] = std::move(value);
            } else {
                if (data_end == capacity_end) {
                    reserve(capacity() ? capacity() * 2 : 1);
                }
                ++data_end;
                for (size_type i = size() - 1; i > index; --i) { data[i] = std::move(data[i - 1]); }
                new (&data[index]) T(std::move(value));
            }
        } else {
            push_back(std::move(value));
        }
    }

    /**
     * @brief Inserts an element at the specified iterator position in the vector using move semantics.
     *
     * This function inserts the given value at the specified iterator position in the vector using move semantics.
     * If the vector's size is equal to its capacity, the capacity is doubled (or set to 1 if it was 0).
     * Elements from the specified iterator position to the end of the vector are shifted one position
     * to the right to make space for the new element.
     *
     * @param it The iterator position at which to insert the new element.
     * @param value The value to be inserted using move semantics.
     */
    void insert(iterator it, T &&value) { insert(it - begin(), std::move(value)); }

    /**
     * @brief Checks if the vector is empty.
     *
     * This function returns true if the vector is empty (i.e., it contains no elements),
     * and false otherwise.
     *
     * @return bool True if the vector is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const { return data == data_end; }

    /**
     * @brief Overloaded equality operator for comparing vectors.
     *
     * This operator compares the elements of the current vector with those of another vector
     * for equality. It returns true if the size and elements of both vectors are equal, and
     * false otherwise.
     *
     * @param other The vector to compare with.
     * @return bool True if the vectors are equal, false otherwise.
     */
    bool operator==(const vector &other) const {
        if (size() != other.size()) {
            return false;
        }
        for (size_type i = 0; i < size(); ++i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Overloaded inequality operator for comparing vectors.
     *
     * This operator compares the elements of the current vector with those of another vector
     * for inequality. It returns true if the size or elements of the vectors are not equal,
     * and false otherwise.
     *
     * @param other The vector to compare with.
     * @return bool True if the vectors are not equal, false otherwise.
     */
    bool operator!=(const vector &other) const { return !(*this == other); }

    /**
     * @brief Compares this vector with another vector for ordering.
     *
     * This operator performs a lexicographical comparison between the elements
     * of this vector and another vector. It first compares the sizes of the vectors.
     * If the sizes are different, the comparison is based on the sizes. If the sizes
     * are the same, it compares the elements one by one using the three-way comparison
     * operator (<=>). The comparison stops as soon as a difference is found.
     *
     * @param other The vector to compare with.
     * @return std::strong_ordering The result of the comparison:
     *         - std::strong_ordering::less if this vector is less than the other vector,
     *         - std::strong_ordering::equal if this vector is equal to the other vector,
     *         - std::strong_ordering::greater if this vector is greater than the other vector.
     */
    auto operator<=>(const vector &other) const {
        if (size() != other.size()) {
            return size() <=> other.size();
        }
        for (size_type i = 0; i < size(); ++i) {
            if (data[i] != other.data[i]) {
                return data[i] <=> other.data[i];
            }
        }
        return std::strong_ordering::equal;
    }
};
} // namespace gtr

#endif
