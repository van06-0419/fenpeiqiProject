#include <iostream>
#include <map>
#include <functional>
#include "../include/pool_allocator.hpp"
#include "../include/simple_seq.hpp"

using namespace std;

long long factorial(int n) {
    long long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

int main() {
    cout << "=== std::map with default allocator ===\n";
    std::map<int,int> m1;
    for (int i = 0; i < 10; ++i) m1[i] = static_cast<int>(factorial(i));
    for (auto &p : m1) cout << p.first << " " << p.second << "\n";

    cout << "\n=== std::map with PoolAllocator (initial reserve 10) ===\n";
    // Note: std::map stores pair<const K, V> as value_type
    using PairType = std::pair<const int,int>;
    PoolAllocator<PairType> pool10(10); // reserve 10 elements
    std::map<int,int, std::less<int>, PoolAllocator<PairType>> m2(std::less<int>(), pool10);

    for (int i = 0; i < 10; ++i) m2[i] = static_cast<int>(factorial(i));
    for (auto &p : m2) cout << p.first << " " << p.second << "\n";

    cout << "\n=== SimpleSeq<int> with default allocator ===\n";
    SimpleSeq<int> s1;
    for (int i = 0; i < 10; ++i) s1.push_back(i);
    for (int x : s1) cout << x << " ";
    cout << "\n";

    cout << "\n=== SimpleSeq<int> with PoolAllocator<int> (reserve 10) ===\n";
    PoolAllocator<int> pool10i(10);
    SimpleSeq<int, PoolAllocator<int>> s2(0, pool10i);
    // we can reserve explicitly
    s2.reserve(10);
    for (int i = 0; i < 10; ++i) s2.push_back(i);
    for (int x : s2) cout << x << " ";
    cout << "\n";

    return 0;
}
