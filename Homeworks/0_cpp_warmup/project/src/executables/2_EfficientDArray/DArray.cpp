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
	Init();
	if(nSize > 0){
		Reserve(nSize);
		m_nSize = nSize;
		std::fill(m_pData, m_pData + m_nSize, dValue);
	}
}

DArray::DArray(const DArray& arr) {
	Init();
	if(arr.m_nSize > 0){
		Reserve(arr.m_nSize);
		m_nSize = arr.m_nSize;
		std::copy(arr.m_pData, arr.m_pData + arr.m_nSize, m_pData);
	}
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
	m_nMax = 0;
}

// free the array
void DArray::Free() {
	delete[] m_pData;
	m_pData = nullptr;
	m_nSize = 0;
	m_nMax = 0;
}

// get the size of the array
int DArray::GetSize() const {
	return m_nSize;
}

void DArray::Reserve(int nSize) {
	if(nSize <= m_nMax) return;

	int newMax = std::max(nSize, m_nMax * 2);
	double* newData = new (std::nothrow) double[newMax];
	if(!newData){
		printf("Memory allocation failed!\n");
		return;
	}

	if (m_pData){
		std::copy(m_pData, m_pData + m_nSize, newData);
		delete[] m_pData;
	}

	m_pData = newData;
	m_nMax = newMax;
}

// set the size of the array
void DArray::SetSize(int nSize) {
	if(nSize == m_nSize) return;
	if (nSize <= 0){
		Free();
		return;
	}
	
	int oldSize = m_nSize;
	Reserve(nSize);
	m_nSize = nSize;

	if(nSize > oldSize){
		std::fill(m_pData + oldSize, m_pData + m_nSize, 0.0);
	}
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
	if(m_nSize >= m_nMax){
		Reserve(m_nSize + 1);
	}
	m_pData[m_nSize++] = dValue;
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

	for(int i = nIndex; i < m_nSize - 1; ++i){
		m_pData[i] = m_pData[i + 1];
	}

	--m_nSize;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	if(nIndex < 0 || nIndex > m_nSize){
		printf("Invalid index!\n");
		return;
	}

	if(m_nSize >= m_nMax){
		Reserve(m_nSize + 1);
	}

	for(int i = m_nSize; i > nIndex; --i){
		m_pData[i] = m_pData[i - 1];
	}

	m_pData[nIndex] = dValue;
	++m_nSize;
}

// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
	if(this == &arr) return *this;

	DArray tmp(arr);
	std::swap(m_pData, tmp.m_pData);
	std::swap(m_nSize, tmp.m_nSize);
	std::swap(m_nMax, tmp.m_nMax);

	return *this;
}
