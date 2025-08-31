#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "string.hpp"

using gtr::char_string;
using gtr::large_string;
using gtr::string;

char const * const long_text = "This is a long text that exceeds the local buffer size of the char_string class to test heap allocation and dynamic resizing functionality.";
char const * const also_a_long_text = "Another long text to ensure that multiple allocations and deallocations work correctly in the char_string class.";

TEST(CharString, DefaultConstructIsEmptyLocal) {
    string s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.capacity(), 63u); // N-1 for string=char_string<64>
    EXPECT_EQ(s.c_str()[0], '\0');
}

TEST(CharString, ConstructWithLongTextGrowsToHeap) {
    string s(long_text);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.size(), strlen(long_text));
    EXPECT_GE(s.capacity(), s.size());
    EXPECT_STREQ(s.c_str(), long_text);
}

TEST(CharString, MoveWithHeapAllocations) {
    string s(long_text);
    string t(std::move(s));
    EXPECT_TRUE(s.empty());
    EXPECT_FALSE(t.empty());
    EXPECT_EQ(t.size(), strlen(long_text));
    EXPECT_STREQ(t.c_str(), long_text);
}

TEST(CharString, SwapWithHeapAllocations) {
    string a(long_text);
    string b(also_a_long_text);
    std::swap(a, b);
    EXPECT_EQ(a.size(), strlen(also_a_long_text));
    EXPECT_EQ(b.size(), strlen(long_text));
    EXPECT_STREQ(a.c_str(), also_a_long_text);
    EXPECT_STREQ(b.c_str(), long_text);
}

TEST(CharString, ConstructFromCStringFitsLocal) {
    string s("abc");
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.size(), 3u);
    EXPECT_STREQ(s.c_str(), "abc");
}

TEST(CharString, AppendGrowsAndNullTerminated) {
    string s("a");
    for (int i = 0; i < 10; ++i) s.append('b');
    EXPECT_EQ(s.size(), 11u);
    EXPECT_EQ(s.c_str()[11], '\0');
    EXPECT_STREQ(s.c_str(), "abbbbbbbbbb");
}

TEST(CharString, ReserveTriggersHeapTransition) {
    // force growth beyond local capacity (63)
    string s("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // 62 x
    EXPECT_LE(s.size(), s.capacity());
    s.append('Y'); // size=63
    s.append('Z'); // size=64 => must grow to heap
    EXPECT_GE(s.capacity(), 64u);
    EXPECT_EQ(s.last(), 'Z');
    EXPECT_STREQ(s.c_str(), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxYZ");
}

TEST(CharString, CopyConstructAndAssign) {
    string a("hello");
    string b(a);
    EXPECT_EQ(a, b);
    string c = a;
    EXPECT_EQ(c, a);
}

TEST(CharString, MoveConstructPropagatesWhenAllowed) {
    string a("payload");
    string b(std::move(a));
    EXPECT_STREQ(b.c_str(), "payload");
}

TEST(CharString, MoveAssignDeepCopiesWhenNoPropagate) {
    string a("payload");
    string b("x");
    b = std::move(a);
    EXPECT_STREQ(b.c_str(), "payload");
}

TEST(CharString, InsertEraseBasics) {
    string s("abef");
    s.insert(2, "cd"); // abcd ef
    EXPECT_STREQ(s.c_str(), "abcdef");
    s.insert(6, '!'); // "abcdef!"
    EXPECT_STREQ(s.c_str(), "abcdef!");
    s.erase(3, 2); // remove "de" -> "abcf!"
    EXPECT_STREQ(s.c_str(), "abcf!");
    s.erase(10, 5); // out of range no-op
    EXPECT_STREQ(s.c_str(), "abcf!");
}

TEST(CharString, Substr) {
    string s("hello world");
    auto a = s.substr(0, 5);
    auto b = s.substr(6);
    EXPECT_STREQ(a.c_str(), "hello");
    EXPECT_STREQ(b.c_str(), "world");
}

TEST(CharString, FindAndFindChar) {
    string s("the quick brown fox");
    EXPECT_EQ(s.find("quick"), 4u);
    EXPECT_EQ(s.find('q'), 4u);
    EXPECT_EQ(s.find("zzz"), string::npos);
}

TEST(CharString, ComparisonsAndSpaceship) {
    const string a("abc");
    const string b("abc");
    const string c("abd");
    const string d("ab");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE((a <=> c) < 0);
    EXPECT_TRUE((a <=> d) > 0);
}

TEST(CharString, CaseConversion) {
    string s("AbC1!");
    auto up = s.upper();
    auto lo = s.lower();
    EXPECT_STREQ(up.c_str(), "ABC1!");
    EXPECT_STREQ(lo.c_str(), "abc1!");
}

TEST(CharString, NumberParsing) {
    string a("123");
    string b("-45.5");
    EXPECT_TRUE(a.is_number());
    EXPECT_FALSE(b.is_number());
    EXPECT_EQ(a.to_int(), 123);
    EXPECT_FLOAT_EQ(b.to_float(), -45.5f);
}

TEST(CharString, AppendOtherStringTemplates) {
    char_string<32> small("aa");
    char_string<256> big("bb");
    small.append(big);
    EXPECT_STREQ(small.c_str(), "aabb");
}

TEST(CharString, FormatHelper) {
    auto s = gtr::format<64>("x=%d y=%s", 42, "ok");
    EXPECT_STREQ(s.c_str(), "x=42 y=ok");
}

TEST(CharString, StrlenNoOverreadLocalBuffer) {
    char_string<64> s;
    for (int i = 0; i < 62; ++i) s.append('x');
    EXPECT_EQ(s.size(), 62u);
}

TEST(CharString, ResizeShrinkWritesExactBytes) {
    string s("abcdef");
    s.resize(2);
    EXPECT_STREQ(s.c_str(), "ab");
}
