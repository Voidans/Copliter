#include <iostream>
#include <cmath>
#include <cstdlib>
using namespace std;

// 计算函数：返回 a + (a + b) 的整数部分
double w(double a, double b)
{
    int c = static_cast<int>(a + b); // 将 a+b 转换为整数
    return a + c; // 返回 a + (a+b 的整数部分)
}

int main()
{
    double output = 0.0; // 声明输出变量
    
    while(true) // 无限循环，直到满足条件跳出
    {
        double inputA; // s声明输入变量
        
        cout << "请输入一个数字: ";
        cin >> inputA; // 获取输入
        
        // 计算并显示函数结果
        double resultA = w(inputA, 2);
        cout << "w(" << inputA << ", 2) = " << resultA << endl;
        output = resultA; // 保存当前计算结果
        
        // 如果计算结果大于3，跳出循环
        if(output > 3) {
            cout << "结果大于3，循环结束" << endl;
            break;
        }
        
        cout << "结果不大于3，继续循环..." << endl;
    }
    
    cout << "最终结果: " << output << endl;
    return 0; // 程序正常退出
}
