#pragma once

#include "array_ptr.h"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>

using namespace std::string_literals;

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t value)
        : value_(value)
    {}

    size_t GetValue() const {
        return value_;
    }

private:
    size_t value_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : capacity_(size), size_(size), items_(size) {}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : capacity_(size), size_(size), items_(size) {
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        Assign(init.begin(), init.end());
    }

    SimpleVector(const SimpleVector& other) {
        *this = other;
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            Assign(rhs.begin(), rhs.end());
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other) {
        *this = std::move(other);
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            capacity_ = std::exchange(rhs.capacity_, 0);
            size_ = std::exchange(rhs.size_, 0);
            items_.Delete();
            items_.swap(rhs.items_);
        }
        return *this;
    }

    SimpleVector(const ReserveProxyObj& Rsrv) {
        Reserve(Rsrv.GetValue());
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range!");
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range!");
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ResizeCapacity(new_capacity);
        }
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }
        if (new_size <= capacity_) {
            for (auto it = &items_[size_];
                      it != &items_[new_size]; ++it) {
                *it = std::move(Type{});                  
            }

            size_ = new_size;
            return;
        }
        ResizeCapacity(std::max(new_size, 2 * capacity_));
        size_ = new_size;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& value) {
        if (capacity_ == size_) {
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        items_[size_++] = value;
    }

    void PushBack(Type&& value) {
        if (capacity_ == size_) {
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        items_[size_++] = std::move(value);
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        size_t number_pos = VectorMove(pos);
        items_[number_pos] = std::move(value);
        return &items_[number_pos];
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t number_pos = VectorMove(pos);
        items_[number_pos] = value;
        return &items_[number_pos];
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        Iterator it = const_cast<Iterator>(pos);
        size_t index = std::distance(begin(), it);
        if (size_ > index + 1) {
            for (auto i = index; i < size_ - 1; ++i) {
                items_[i] = std::move(items_[i + 1]);
            }
        }
        --size_;
        return it;
    }

    Iterator begin() noexcept {
        return items_.Get();
    }

    Iterator end() noexcept {
        return &items_[size_];
    }

    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    ConstIterator end() const noexcept {
        return &items_[size_];
    }

    ConstIterator cbegin() const noexcept {
        return const_cast<Type*>(items_.Get());
    }

    ConstIterator cend() const noexcept {
        return &items_[size_];
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

    ~SimpleVector()
    {}

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    ArrayPtr<Type> items_;

    template <typename InputIterator>
    void Assign(InputIterator from, InputIterator to) {
        size_t size = std::distance(from, to);
        SimpleVector<Type> tmp(size);
        if (size > 0) {
            std::move(std::make_move_iterator(from), std::make_move_iterator(to), tmp.begin());
        }
        swap(tmp);
    }

    void ResizeCapacity(size_t new_capacity) {
        ArrayPtr<Type> tmp_data(new_capacity);
        std::move(std::make_move_iterator(begin()), std::make_move_iterator(end()), &tmp_data[0]);
        items_ = std::move(tmp_data);
        capacity_ = new_capacity;
    }

    size_t VectorMove(ConstIterator pos) {
        size_t number_pos = std::distance<ConstIterator>(cbegin(), pos);
        if (size_ < capacity_) {
            if (pos == end()) {
                assert(pos == end());
            }
            std::move_backward(&items_[number_pos], end(), &items_[size_ + 1]);
        } else {
            if (capacity_ == 0) {
                Reserve(1);
            } else {
                Reserve(2 * capacity_);
                std::move_backward(&items_[number_pos], end(), &items_[size_ + 1]);
            }
        }
        ++size_;
        return number_pos;
    }
};

template <typename Type>
void swap(SimpleVector<Type>& lhs, SimpleVector<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (&lhs == &rhs) || (lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs);
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}