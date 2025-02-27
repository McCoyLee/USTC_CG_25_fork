#pragma once
#include <cstdio>
#include <algorithm>
#include <stdexcept>

template <typename T>
class DArray {
public:
    DArray();                        // 默认构造函数
    DArray(int nSize, const T& dValue = T()); // 带参构造函数
    DArray(const DArray& arr);       // 拷贝构造函数
    ~DArray();                       // 析构函数

    void Print() const;              // 打印数组元素
    int GetSize() const;             // 获取元素数量
    void SetSize(int nSize);         // 设置数组大小

    const T& GetAt(int nIndex) const;// 获取元素（const版本）
    void SetAt(int nIndex, const T& dValue); // 设置元素值

    T& operator[](int nIndex);       // 重载[]操作符（非const）
    const T& operator[](int nIndex) const; // 重载[]操作符（const）

    void PushBack(const T& dValue);  // 尾部插入元素
    void DeleteAt(int nIndex);       // 删除指定位置元素
    void InsertAt(int nIndex, const T& dValue); // 插入元素

    DArray& operator=(const DArray& arr); // 赋值操作符

private:
    T* m_pData;     // 数据存储指针
    int m_nSize;    // 实际元素数量
    int m_nMax;     // 预分配内存容量

private:
    void Init();     // 初始化成员变量
    void Free();     // 释放内存
    void Reserve(int nSize); // 预分配内存
};

// ================== 成员函数实现 ==================
template <typename T>
DArray<T>::DArray() {
    Init();
}

template <typename T>
DArray<T>::DArray(int nSize, const T& dValue) {
    Init();
    if (nSize > 0) {
        Reserve(nSize);
        m_nSize = nSize;
        std::fill(m_pData, m_pData + m_nSize, dValue);
    }
}

template <typename T>
DArray<T>::DArray(const DArray& arr) {
    Init();
    if (arr.m_nSize > 0) {
        Reserve(arr.m_nSize);
        m_nSize = arr.m_nSize;
        std::copy(arr.m_pData, arr.m_pData + m_nSize, m_pData);
    }
}

template <typename T>
DArray<T>::~DArray() {
    Free();
}

template <typename T>
void DArray<T>::Print() const {
    if (m_nSize <= 0 || !m_pData) {
        printf("The array is empty!\n");
        return;
    }

    for (int i = 0; i < m_nSize; ++i) {
        if constexpr (std::is_same_v<T, int>) {
            printf("%d ", m_pData[i]);
        } else if constexpr (std::is_same_v<T, double>) {
            printf("%.2f ", m_pData[i]);
        } else if constexpr (std::is_same_v<T, char>) {
            printf("%d ", static_cast<int>(m_pData[i]));
        } else {
            printf("[Object at %p] ", &m_pData[i]);
        }
    }
    printf("\n");
}

template <typename T>
void DArray<T>::Init() {
    m_pData = nullptr;
    m_nSize = 0;
    m_nMax = 0;
}

template <typename T>
void DArray<T>::Free() {
    delete[] m_pData;
    m_pData = nullptr;
    m_nSize = 0;
    m_nMax = 0;
}

template <typename T>
int DArray<T>::GetSize() const {
    return m_nSize;
}

template <typename T>
void DArray<T>::Reserve(int nSize) {
    if (nSize <= m_nMax) return;

    int newMax = std::max(nSize, m_nMax * 2);
    T* newData = new (std::nothrow) T[newMax];
    if (!newData) {
        throw std::bad_alloc();
    }

    if (m_pData) {
        std::copy(m_pData, m_pData + m_nSize, newData);
        delete[] m_pData;
    }

    m_pData = newData;
    m_nMax = newMax;
}

template <typename T>
void DArray<T>::SetSize(int nSize) {
    if (nSize == m_nSize) return;
    if (nSize <= 0) {
        Free();
        return;
    }

    int oldSize = m_nSize;
    Reserve(nSize);
    m_nSize = nSize;

    if (nSize > oldSize) {
        std::fill(m_pData + oldSize, m_pData + m_nSize, T());
    }
}

template <typename T>
const T& DArray<T>::GetAt(int nIndex) const {
    if (nIndex < 0 || nIndex >= m_nSize) {
        throw std::out_of_range("Index out of range");
    }
    return m_pData[nIndex];
}

template <typename T>
void DArray<T>::SetAt(int nIndex, const T& dValue) {
    if (nIndex >= 0 && nIndex < m_nSize) {
        m_pData[nIndex] = dValue;
    } else {
        throw std::out_of_range("Index out of range");
    }
}

template <typename T>
T& DArray<T>::operator[](int nIndex) {
    if (nIndex < 0 || nIndex >= m_nSize) {
        throw std::out_of_range("Index out of range");
    }
    return m_pData[nIndex];
}

template <typename T>
const T& DArray<T>::operator[](int nIndex) const {
    return GetAt(nIndex);
}

template <typename T>
void DArray<T>::PushBack(const T& dValue) {
    if (m_nSize >= m_nMax) {
        Reserve(m_nSize + 1);
    }
    m_pData[m_nSize++] = dValue;
}

template <typename T>
void DArray<T>::DeleteAt(int nIndex) {
    if (nIndex < 0 || nIndex >= m_nSize) {
        throw std::out_of_range("Index out of range");
    }

    if (m_nSize == 1) {
        Free();
        return;
    }

    for (int i = nIndex; i < m_nSize - 1; ++i) {
        m_pData[i] = m_pData[i + 1];
    }
    --m_nSize;
}

template <typename T>
void DArray<T>::InsertAt(int nIndex, const T& dValue) {
    if (nIndex < 0 || nIndex > m_nSize) {
        throw std::out_of_range("Invalid index");
    }

    if (m_nSize >= m_nMax) {
        Reserve(m_nSize + 1);
    }

    for (int i = m_nSize; i > nIndex; --i) {
        m_pData[i] = m_pData[i - 1];
    }

    m_pData[nIndex] = dValue;
    ++m_nSize;
}

template <typename T>
DArray<T>& DArray<T>::operator=(const DArray& arr) {
    if (this == &arr) return *this;

    DArray tmp(arr);
    std::swap(m_pData, tmp.m_pData);
    std::swap(m_nSize, tmp.m_nSize);
    std::swap(m_nMax, tmp.m_nMax);

    return *this;
}