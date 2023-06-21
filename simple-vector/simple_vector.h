#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>
#include "array_ptr.h"


struct ReserveProxyObj
{
    ReserveProxyObj(size_t capacity) :capacity_to_reserve_(capacity)
    {
    }
    size_t capacity_to_reserve_;
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
    explicit SimpleVector(size_t size) 
        : SimpleVector(size, Type{}) 
    {      
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) 
        : vector_(size)
        , size_(size)
        , capacity_(size) 
    {
        std::fill(vector_.Get(), vector_.Get() + size, value);     
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
        : vector_(init.size())
        , size_(init.size())
        , capacity_(init.size()) 
    {
        std::copy(init.begin(), init.end(), vector_.Get()); 
    }

    SimpleVector(const SimpleVector& other) 
        : vector_(other.GetSize())
        , size_(other.GetSize())
        , capacity_(other.GetSize()) 
    {
        std::copy(other.begin(), other.end(), vector_.Get());
    }


    SimpleVector(SimpleVector&& other) 
        : vector_(std::move(other.vector_))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0)) 
    {
    }

    SimpleVector(const ReserveProxyObj& cap_cap)
        : vector_(cap_cap.capacity_to_reserve_)
        , size_(0)
        , capacity_(cap_cap.capacity_to_reserve_)
    {
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (*this != rhs) {
            SimpleVector tmp(rhs);
            swap(tmp);
            return *this;
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (*this != rhs) {
            rhs.swap(*this);
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ == capacity_) {          
            Reserve(std::max(1u, capacity_ * 2));
            vector_[size_] = item;
            ++size_;
        }
        else {
            vector_[size_] = item;
            ++size_;
        }
    }

    void PushBack(Type&& item) {
        if (size_ == capacity_) {           
            Reserve(std::max(1u, capacity_ * 2));
            vector_[size_] = std::move(item);
            ++size_;
        }
        else {
            vector_[size_] = std::move(item);
            ++size_;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t index;
        if (pos == end()) {
            index = pos - begin();
        }
        InsertHelper(index);
        vector_[index] = std::move(value);
        return Iterator(vector_.Get() + index);
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        size_t index;
        if (pos == end()) {
            index = size_;
        }
        else {
            index = pos - begin();
        }
        InsertHelper(index);
        vector_[index] = std::move(value);
        return Iterator(vector_.Get() + index);
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty());
        size_t index = pos - begin();
        std::move(vector_.Get() + index + 1, end(), vector_.Get() + index);
        --size_;
        return Iterator(vector_.Get() + index);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        vector_.swap(other.vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return (size_ == 0 ? true : false);
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("out_of_range");
        }
        else {
            return vector_[index];
        }
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("out_of_range");
        }
        else {
            return vector_[index];
        }
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            Reserve(new_size);
            std::generate(vector_.Get() + size_, vector_.Get() + new_size, []() { return Type{}; });
            size_ = new_size;         
        }
        if (new_size < size_) {
            size_ = new_size;
        }
        else {
            std::generate(vector_.Get() + size_, vector_.Get() + new_size, []() { return std::move(Type{}); });
            size_ = new_size;
        }
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> tmp(new_capacity);
            std::move(vector_.Get(), vector_.Get() + size_, tmp.Get());
            vector_.swap(tmp);
            capacity_ = new_capacity;
        } else {
            return;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(vector_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return Iterator(vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return Iterator(vector_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return Iterator(vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return Iterator(vector_.Get() + size_);
    }

private:
    ArrayPtr<Type> vector_;
    size_t size_ = 0;
    size_t capacity_ = 0;

    void InsertHelper(size_t index) {
        if (size_ == capacity_) {
            if (capacity_ == 0) {
                ArrayPtr<Type> tmp(1);
                vector_.swap(tmp);
                size_ = 1;
                capacity_ = 1;
            } 
            else {
                ArrayPtr<Type>tmp(capacity_ * 2);
                std::move(begin(), vector_.Get() + index, tmp.Get());
                std::move(vector_.Get() + index, end(), tmp.Get() + index + 1);
                vector_.swap(tmp);
                ++size_;
                capacity_ = capacity_ * 2;
            }           
        }
        else {
            std::move(vector_.Get() + index, vector_.Get() + size_, vector_.Get() + index + 1);
            ++size_;
        }
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}