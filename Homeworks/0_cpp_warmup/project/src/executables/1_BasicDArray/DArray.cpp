// implementation of class DArray
#include "DArray.h"
#include <cstdio>
#include <algorithm>

// default constructor
DArray::DArray() {
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
	if(nSize <= 0){
		Init();
		return;
	}
	
	m_pData = new double[nSize];
	if (!m_pData) {
        Init();
        printf("Memory allocation failed!\n");
        return;
    }

	m_nSize = nSize;
	std::fill_n(m_pData, nSize, dValue);
}

DArray::DArray(const DArray& arr) {
	if (arr.m_nSize <= 0) {
		Init();
		return;
	}
	
	m_pData = new double[arr.m_nSize];
	if (!m_pData) {
        Init();
        printf("Memory allocation failed!\n");
        return;
    }

	m_nSize = arr.m_nSize;
	std::copy_n(arr.m_pData, m_nSize, m_pData);;
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
	if(m_nSize <= 0 || !m_pData){
		printf("The array is empty!\n");
		return;
	}

	for(int i = 0; i < m_nSize; ++i){
		printf("%.2f ", m_pData[i]);
	}
	printf("\n");
}

// initilize the array
void DArray::Init() {
	m_pData = nullptr;
	m_nSize = 0;
}

// free the array
void DArray::Free() {
	delete[] m_pData;
	m_pData = nullptr;
	m_nSize = 0;
}

// get the size of the array
int DArray::GetSize() const {
	return m_nSize;
}

// set the size of the array
void DArray::SetSize(int nSize) {
	if(nSize == m_nSize) return;
	if (nSize <= 0){
		Free();
		return;
	}
	
	double* NewData = new double[nSize]();

	if (!NewData) {
        Free();
        printf("Memory allocation failed!\n");
        return;
    }

	const int copySize = std::min(m_nSize, nSize);
	if(m_pData){
		std::copy(m_pData, m_pData + copySize, NewData);
	}

	delete[] m_pData;
	m_pData = NewData;
	m_nSize = nSize;
}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
	if(nIndex < 0 || nIndex >= m_nSize){
		static const double ERROR = 0.0; 
		printf("Index out of range!\n");
		return ERROR; 
	}
	return m_pData[nIndex];
}

// set the value of an element 
void DArray::SetAt(int nIndex, double dValue) {
	if (nIndex >= 0 && nIndex < m_nSize){
		m_pData[nIndex] = dValue;
	}else{
		printf("Index out of range!\n");
	}
	
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
    if(nIndex < 0 || nIndex >= m_nSize){
	static double ERROR = 0.0; 
	printf("Index out of range!\n");
	return ERROR; // you should return a correct value
    }
	return m_pData[nIndex];
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
	double* newData = new (std::nothrow) double[m_nSize + 1];	
	if(!newData){
		printf("Memory allocation failed!\n");
		return;
	}

	std::copy(m_pData, m_pData + m_nSize, newData);
	newData[m_nSize] = dValue;

	delete[] m_pData;
	m_pData = newData;
	++m_nSize;
}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
	if(nIndex < 0 || nIndex >= m_nSize){
		printf("Index out of range!\n");
		return;
	}

	if (m_nSize == 1){
		Free();
		return;
	}

	double* newData = new (std::nothrow) double[m_nSize - 1];
	if (!newData){
		printf("Memory allocation failed!\n");
		return;
	}

	std::copy(m_pData, m_pData + nIndex, newData);
	std::copy(m_pData + nIndex + 1, m_pData + m_nSize, newData + nIndex);
	
	delete[] m_pData;
	m_pData = newData;
	--m_nSize;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	if(nIndex < 0 || nIndex > m_nSize){
		printf("Invalid index!\n");
		return;
	}

	double* newData = new (std::nothrow) double[m_nSize + 1];
	if (!newData){
		printf("Memory allocation failed!\n");
		return;
	}

	std::copy(m_pData, m_pData + nIndex, newData);
	newData[nIndex] = dValue;
	std::copy(m_pData + nIndex, m_pData + m_nSize, newData + nIndex + 1);

	delete[] m_pData;
	m_pData = newData;
	++m_nSize;
}

// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
	if(this == &arr) return *this;

	DArray tmp(arr);
	std::swap(m_pData, tmp.m_pData);
	std::swap(m_nSize, tmp.m_nSize);

	return *this;
}
