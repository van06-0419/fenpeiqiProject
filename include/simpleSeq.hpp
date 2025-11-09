#pragma once
#include <memory>
#include <cstddef>
#include <iterator>

// 一个非常精简的顺序容器，支持分配器模板参数。
// - push_back
// - begin/end (指针迭代器)
// - size, empty
// 设计用于测试自定义分配器与 std::allocator 的可用性。

template <typename T, typename Alloc = std::allocator<T>>
class SimpleSeq {
public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;

    explicit SimpleSeq(size_type reserve_capacity = 0, const Alloc& alloc = Alloc())
        : alloc_(alloc) {
        if (reserve_capacity > 0) {
            reserve(reserve_capacity);
        }
    }

    ~SimpleSeq() {
        clear();
        if (data_) {
            alloc_traits::deallocate(alloc_, data_, capacity_);
        }
    }

    void push_back(const T& v) {
        if (size_ >= capacity_) {
            grow();
        }
        alloc_traits::construct(alloc_, data_ + size_, v);
        ++size_;
    }

    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    iterator begin() noexcept { return data_; }
    iterator end() noexcept { return data_ + size_; }
    const_iterator begin() const noexcept { return data_; }
    const_iterator end() const noexcept { return data_ + size_; }

    void reserve(size_type new_cap) {
        if (new_cap <= capacity_) return;
        pointer new_data = alloc_traits::allocate(alloc_, new_cap);
        for (size_type i = 0; i < size_; ++i) {
            alloc_traits::construct(alloc_, new_data + i, std::move_if_noexcept(data_[i]));
            alloc_traits::destroy(alloc_, data_ + i);
        }
        if (data_) alloc_traits::deallocate(alloc_, data_, capacity_);
        data_ = new_data;
        capacity_ = new_cap;
    }

    void clear() {
        for (size_type i = 0; i < size_; ++i)
            alloc_traits::destroy(alloc_, data_ + i);
        size_ = 0;
    }

private:
    using alloc_traits = std::allocator_traits<Alloc>;
    Alloc alloc_;
    T* data_ = nullptr;
    size_type size_ = 0;
    size_type capacity_ = 0;

    void grow() {
        size_type newcap = capacity_ == 0 ? 2 : capacity_ * 2;
        reserve(newcap);
    }
};
