#include "DArray.h"
#include <cstdio>

int main(int argc, char** argv) {
    // 测试 double 类型
    DArray<double> a;
    a.InsertAt(0, 2.1);
    a.Print();  // [2.10]

    a.PushBack(3.0);
    a.PushBack(3.1);
    a.PushBack(3.2);
    a.Print();  // [2.10 3.00 3.10 3.20]

    a.DeleteAt(0);
    a.Print();  // [3.00 3.10 3.20]
    a.InsertAt(0, 4.1);
    a.Print();  // [4.10 3.00 3.10 3.20]

    // 测试拷贝构造
    DArray<double> acopy = a;
    acopy.Print();  // [4.10 3.00 3.10 3.20]

    DArray<double> acopy2(a);
    acopy2.Print();  // [4.10 3.00 3.10 3.20]

    // 测试赋值运算符
    DArray<double> acopy3, acopy4;
    acopy4 = acopy3 = a;
    acopy3.Print();  // [4.10 3.00 3.10 3.20]
    acopy4.Print();  // [4.10 3.00 3.10 3.20]

    // 测试 int 类型
    DArray<int> b;
    b.PushBack(21);
    b.Print();  // [21]
    b.DeleteAt(0);
    b.Print();  // [Empty]
    b.PushBack(22);
    b.SetSize(5);
    b.Print();  // [22 0 0 0 0]

    // 测试 char 类型
    DArray<char> c;
    c.PushBack('a');
    c.PushBack('b');
    c.PushBack('c');
    c.InsertAt(0, 'd');
    c.Print();  // [100 97 98 99]

    return 0;
}