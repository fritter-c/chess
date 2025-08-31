#include <array>
#include <compare>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include "vector.hpp"

struct Tracker {
    static inline int live = 0, ctor = 0, dtor = 0;
    static inline int copy_ctor = 0, move_ctor = 0, copy_assign = 0, move_assign = 0;

    int v{0};

    Tracker() {
        ++live;
        ++ctor;
    }
    explicit Tracker(int x) : v(x) {
        ++live;
        ++ctor;
    }
    Tracker(const Tracker &o) : v(o.v) {
        ++live;
        ++copy_ctor;
    }
    Tracker(Tracker &&o) noexcept : v(o.v) {
        ++live;
        ++move_ctor;
        o.v = -777;
    }

    Tracker &operator=(const Tracker &o) {
        v = o.v;
        ++copy_assign;
        return *this;
    }
    Tracker &operator=(Tracker &&o) noexcept {
        v = o.v;
        ++move_assign;
        o.v = -888;
        return *this;
    }

    ~Tracker() {
        --live;
        ++dtor;
    }

    auto operator<=>(const Tracker &) const = default;
    bool operator==(const Tracker &) const = default;

    static void reset() { live = ctor = dtor = copy_ctor = move_ctor = copy_assign = move_assign = 0; }
};

template <class T> static gtr::vector<T> make_with_capacity(std::size_t cap) {
    gtr::vector<T> v(cap);
    v.clear();
    return v;
}

TEST(Vector, Construct_Size_And_Fill) {
    Tracker::reset();
    gtr::vector<Tracker> v(3, Tracker(42));
    EXPECT_EQ(v.size(), 3u);
    EXPECT_GE(v.capacity(), 3u);
    EXPECT_EQ(v.front().v, 42);
    EXPECT_EQ(v.back().v, 42);
    EXPECT_EQ(Tracker::ctor + Tracker::copy_ctor + Tracker::move_ctor, Tracker::live + Tracker::dtor);
}

TEST(Vector, PushBack_Growth_MovesOldElements) {
    Tracker::reset();
    auto v = make_with_capacity<Tracker>(2);
    v.push_back(Tracker(1));
    v.push_back(Tracker(2));

    int before_moves = Tracker::move_ctor;
    v.push_back(Tracker(3));
    EXPECT_EQ(v.size(), 3u);
    EXPECT_GE(v.capacity(), 3u);
    EXPECT_GE(Tracker::move_ctor, before_moves + 2);
}

TEST(Vector, EmplaceBack_NoExtraCopy) {
    Tracker::reset();
    auto v = make_with_capacity<Tracker>(4);
    v.emplace_back(10);
    v.emplace_back(20);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].v, 10);
    EXPECT_EQ(v[1].v, 20);
    EXPECT_EQ(Tracker::copy_ctor, 0);
}

TEST(Vector, CopyConstructor_DeepCopy_ContentEqual) {
    Tracker::reset();
    gtr::vector<Tracker> a{Tracker(1), Tracker(2), Tracker(3)};
    auto b = a;
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) EXPECT_EQ(a[i].v, b[i].v);
    EXPECT_GE(Tracker::copy_ctor, 3);
}

TEST(Vector, MoveConstructor_ContentPreserved_SourceValidButEmptyOrUnspecified) {
    Tracker::reset();
    gtr::vector<Tracker> a{Tracker(7), Tracker(8), Tracker(9)};
    auto a_size = a.size();
    gtr::vector<Tracker> b(std::move(a));
    ASSERT_EQ(b.size(), a_size);
    EXPECT_EQ(b.front().v, 7);
    EXPECT_EQ(b.back().v, 9);
    EXPECT_TRUE(a.size() == 0 || a.begin() == a.end());
}

TEST(Vector, CopyAssign_And_MoveAssign) {
    Tracker::reset();
    gtr::vector<Tracker> a{Tracker(1), Tracker(2)};
    gtr::vector<Tracker> b{Tracker(3)};
    b = a;
    ASSERT_EQ(b.size(), a.size());
    EXPECT_EQ(b[0].v, 1);
    EXPECT_EQ(b[1].v, 2);

    gtr::vector<Tracker> c{Tracker(10), Tracker(11), Tracker(12)};
    b = std::move(c);
    ASSERT_EQ(b.size(), 3u);
    EXPECT_EQ(b[0].v, 10);
    EXPECT_EQ(b[2].v, 12);
}

TEST(Vector, Insert_Middle_And_End_MoveShift) {
    Tracker::reset();
    gtr::vector<Tracker> v{Tracker(1), Tracker(2), Tracker(4)};
    v.insert(2, Tracker(3));
    ASSERT_EQ(v.size(), 4u);
    EXPECT_EQ((std::array<int, 4>{v[0].v, v[1].v, v[2].v, v[3].v}), (std::array<int, 4>{1, 2, 3, 4}));
}

TEST(Vector, Erase_Ordered) {
    Tracker::reset();
    gtr::vector<Tracker> v{Tracker(1), Tracker(2), Tracker(3), Tracker(4)};
    v.erase(1);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ((std::array<int, 3>{v[0].v, v[1].v, v[2].v}), (std::array<int, 3>{1, 3, 4}));
}

TEST(Vector, Erase_Range_Ordered) {
    Tracker::reset();
    gtr::vector<Tracker> v{Tracker(1), Tracker(2), Tracker(3), Tracker(4), Tracker(5), Tracker(6)};
    v.erase(2, 5);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ((std::array<int, 3>{v[0].v, v[1].v, v[2].v}), (std::array<int, 3>{1, 2, 6}));
}

TEST(Vector, EraseUnordered) {
    Tracker::reset();
    gtr::vector<Tracker> v{Tracker(10), Tracker(20), Tracker(30), Tracker(40)};
    v.erase_unordered(1);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_TRUE(v[1].v == 40 || v[1].v == -777 || v[1].v == -888);
}

TEST(Vector, PushRange_And_Iterators) {
    Tracker::reset();
    gtr::vector<Tracker> v(2, Tracker(1));
    Tracker tmp[3] = {Tracker(2), Tracker(3), Tracker(4)};
    v.push_range(tmp, tmp + 3);
    ASSERT_EQ(v.size(), 5u);
    int sum = 0;
    for (auto &t : v) sum += t.v;
    EXPECT_EQ(sum, 1 + 1 + 2 + 3 + 4);
}

TEST(Vector, ShrinkToFit) {
    Tracker::reset();
    auto v = make_with_capacity<Tracker>(8);
    for (int i = 0; i < 5; ++i) v.emplace_back(i);
    auto cap_before = v.capacity();
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());
    EXPECT_GE(cap_before, v.capacity());
}

TEST(Vector, Comparisons) {
    Tracker::reset();
    gtr::vector<Tracker> a{Tracker(1), Tracker(2)};
    gtr::vector<Tracker> b{Tracker(1), Tracker(2)};
    gtr::vector<Tracker> c{Tracker(1), Tracker(3)};
    EXPECT_TRUE(a == b);
    EXPECT_TRUE((a <=> c) < 0);
    EXPECT_TRUE((c <=> a) > 0);
}

// ----------------------------- Tests (int) ------------------------------------

TEST(Vector, InitList_Copy_Equality) {
    gtr::vector<int> a{1, 2, 3, 4};
    gtr::vector<int> b = a;
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) EXPECT_EQ(a[i], b[i]);
    EXPECT_TRUE(a == b);
}

TEST(Vector, Insert_And_Erase_Ordered) {
    gtr::vector<int> v{1, 2, 4};
    v.insert(2, 3);
    EXPECT_EQ((std::array<int, 4>{v[0], v[1], v[2], v[3]}), (std::array<int, 4>{1, 2, 3, 4}));
    v.erase(1);
    EXPECT_EQ((std::array<int, 3>{v[0], v[1], v[2]}), (std::array<int, 3>{1, 3, 4}));
}

TEST(Vector, PushRange_And_Shrink) {
    auto v = make_with_capacity<int>(4);
    const int arr[3] = {7, 8, 9};
    v.push_range(arr, arr + 3);
    ASSERT_EQ(v.size(), 3u);
    const auto cap_before = v.capacity();
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());
    EXPECT_GE(cap_before, v.capacity());
}

template <class T> using A_NonProp = gtr::stateful_allocator<T, /*POCCA*/ false, /*POCMA*/ false, /*POCSW*/ false, /*SOCCC_SAME*/ true>;
template <class T> using A_MoveProp = gtr::stateful_allocator<T, /*POCCA*/ false, /*POCMA*/ true, /*POCSW*/ true, /*SOCCC_SAME*/ true>;
template <class T> using A_SOCCC_Fresh = gtr::stateful_allocator<T, /*POCCA*/ false, /*POCMA*/ false, /*POCSW*/ false, /*SOCCC_SAME*/ false>;

template <class Alloc> static gtr::vector<int, Alloc> make_vec(std::initializer_list<int> init, uint64_t tag) {
    gtr::vector<int, Alloc> v{init};
    v.allocator() = Alloc{tag};
    return v;
}

TEST(Vector, MoveCtor_StealsBuffer_When_Propagate) {
    using Alloc = A_MoveProp<int>;
    auto a = make_vec<Alloc>({1, 2, 3, 4}, /*tag*/ 111);
    int *old_ptr = a.data;
    auto old_state = a.allocator().st;

    gtr::vector<int, Alloc> b(std::move(a));

    ASSERT_EQ(b.size(), 4u);
    EXPECT_EQ(b.data, old_ptr) << "POCMA=true should steal buffer";
    EXPECT_EQ(a.data, nullptr) << "Moved-from should null pointers in steal path";
    EXPECT_EQ(b.allocator().st, old_state) << "Allocator state should propagate to the moved-to object";
    EXPECT_EQ((std::array{b[0], b[1], b[2], b[3]}), (std::array{1, 2, 3, 4}));
}

TEST(Vector, MoveCtor_NoSteal_When_NotPropagating) {
    using Alloc = A_NonProp<int>;
    auto a = make_vec<Alloc>({5, 6, 7}, /*tag*/ 222);
    int *old_ptr = a.data;
    auto old_state = a.allocator().st;

    gtr::vector<int, Alloc> b(std::move(a));

    ASSERT_EQ(b.size(), 3u);
    EXPECT_NE(b.data, old_ptr) << "POCMA=false should move-construct into fresh buffer";
    EXPECT_EQ(a.data, old_ptr) << "Non-propagating move leaves source as-is (your impl)";
    EXPECT_EQ(b.allocator().st.get(), nullptr == nullptr ? b.allocator().st.get() : b.allocator().st.get())
        << "Allocator for 'b' is default-constructed in your move-ctor else-branch; "
           "we only assert content below to keep behavior-agnostic.";
    EXPECT_EQ((std::array{b[0], b[1], b[2]}), (std::array{5, 6, 7}));
    EXPECT_EQ((std::array{a[0], a[1], a[2]}), (std::array{5, 6, 7})) << "Source still contains elements in your current design";
}

TEST(Vector, CopyCtor_KeepsAllocatorState_When_SOCCC_Same) {
    using Alloc = A_NonProp<int>; // SOCCC_SAME=true
    auto a = make_vec<Alloc>({1, 2, 3}, /*tag*/ 42);
    auto a_state = a.allocator().st;

    gtr::vector<int, Alloc> b(a);

    ASSERT_EQ(b.size(), a.size());
    EXPECT_EQ(b.allocator().st, a_state) << "select_on_container_copy_construction should keep the same state";
    for (size_t i = 0; i < a.size(); ++i) EXPECT_EQ(a[i], b[i]);
}

TEST(Vector, CopyCtor_FreshAllocatorState_When_SOCCC_Fresh) {
    using Alloc = A_SOCCC_Fresh<int>; // SOCCC_SAME=false
    auto a = make_vec<Alloc>({9, 8, 7}, /*tag*/ 77);
    auto a_state = a.allocator().st;

    gtr::vector<int, Alloc> b(a);

    ASSERT_EQ(b.size(), a.size());
    EXPECT_NE(b.allocator().st, a_state) << "SOCCC=false should yield a fresh allocator state in the copy";
    for (size_t i = 0; i < a.size(); ++i) EXPECT_EQ(a[i], b[i]);
}

TEST(Vector, CopyAssign_UsesCopySwap_ContentMatches_Source) {
    using Alloc = A_NonProp<int>; // Copy-swap swaps allocator subobjects regardless of POCSW
    auto a = make_vec<Alloc>({1, 1, 2, 3}, /*tag*/ 101);
    auto b = make_vec<Alloc>({4, 4}, /*tag*/ 202);
    auto a_state = a.allocator().st;

    b = a; // by-value parameter uses SOCCC (keeps state here), then swap

    ASSERT_EQ(b.size(), a.size());
    for (size_t i = 0; i < a.size(); ++i) EXPECT_EQ(b[i], a[i]);
    EXPECT_EQ(b.allocator().st, a_state) << "After copy-swap, 'b' should own the same allocator state as 'a' (SOCCC kept)";
}

TEST(Vector, MoveAssign_EndsWithStolenBuffer_When_Propagate) {
    using Alloc = A_MoveProp<int>;
    auto a = make_vec<Alloc>({10, 20, 30}, /*tag*/ 303);
    auto b = make_vec<Alloc>({7}, /*tag*/ 404);

    int *old_a_ptr = a.data;
    auto a_state = a.allocator().st;

    b = std::move(a); // parameter constructed via move-ctor (steals), then swap

    ASSERT_EQ(b.size(), 3u);
    EXPECT_EQ(b.data, old_a_ptr) << "Result of move-assign should get the stolen buffer";
    EXPECT_EQ(a.data, nullptr);
    EXPECT_EQ(b.allocator().st, a_state) << "Allocator state should follow the data when POCMA=true";
    EXPECT_EQ((std::array{b[0], b[1], b[2]}), (std::array{10, 20, 30}));
}

TEST(Vector, MoveAssign_NoSteal_When_NotPropagating) {
    using Alloc = A_NonProp<int>;
    auto a = make_vec<Alloc>({3, 2, 1}, /*tag*/ 505);
    auto b = make_vec<Alloc>({9, 9}, /*tag*/ 606);

    int *old_a_ptr = a.data;

    b = std::move(a);

    ASSERT_EQ(b.size(), 3u);
    EXPECT_NE(b.data, old_a_ptr) << "No propagate => moved elements, fresh buffer";
    EXPECT_EQ((std::array{b[0], b[1], b[2]}), (std::array{3, 2, 1}));
}

TEST(Vector, Equality_And_Ordering_WithStatefulAllocator) {
    using Alloc = A_MoveProp<int>;
    gtr::vector<int, Alloc> a{1, 2, 3};
    gtr::vector<int, Alloc> b{1, 2, 3};
    a.allocator() = Alloc{1};
    b.allocator() = Alloc{2};

    EXPECT_TRUE(a == b);
    auto cmp = (a <=> b);
    EXPECT_EQ(cmp, std::strong_ordering::equal);
}
