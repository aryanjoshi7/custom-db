#ifndef PRINT
#define PRINT
#include <iostream>
#include <vector>

template <typename T>
void out(T head) {
    std::cout << head << " ";
}
template <typename T>
void out(std::vector<T> v) {
    std::cout << "[";
    for (int i = 0; i < v.size(); ++i) {
        if (i)
            std::cout << ", ";
        std::cout << v[i];
    }
    std::cout << "]";
}
void print() {
    out("\n");
}

template <typename T, typename... Tail>
void print(T head, Tail... end) {
    out(head);
    print(end...);
}
#endif