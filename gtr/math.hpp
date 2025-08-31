#ifndef MATH_H
#define MATH_H
namespace gtr {
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
template <typename T> T abs(T value) { return value < 0 ? -value : value; }
}
#endif // MATH_H
