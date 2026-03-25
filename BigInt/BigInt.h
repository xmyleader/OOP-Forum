#ifndef BIGINT_H
#define BIGINT_H

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <utility>

class BigInt {
private:
    /*========== 内部数据成员 ==========*/

    // 采用 1e9 作为一个limb的进制
    // 拆成若干块，每块范围都是[0, 1e9)
    static constexpr uint32_t BASE = 1000000000;
    static constexpr int BASE_DIGITS = 9;
    
    // 符号位：true表示负数，false表示非负数
    bool is_neg;

    // 数值部分：只存绝对值，低位在前
    // Ex: 1234567890123 存成 [567890123, 1234]
    // Ex: 12345567890123456789 存成 [123456789, 234567890, 1]
    std::vector<uint32_t> limbs;

    /*========== 内部辅助函数 ==========*/

    // 判断当前对象是否为0
    bool isZero() const;

    // 统一归一化函数，规范内部表示
    // 1. 去掉高位多余的0
    // 2. 保证limbs至少保留一个元素
    // 3. 若值为0，则强制设为非负
    void normalize();

    // 比较两个绝对值对象大小
    // 返回值：
    // -1: |a| < |b|
    //  0: |a| == |b|
    //  1: |a| > |b|
    static int compareAbs(const BigInt& a, const BigInt& b);

    // 绝对值加法：返回 |a| + |b|
    static BigInt addAbs(const BigInt& a, const BigInt& b);

    // 绝对值减法：返回 |a| - |b|
    // assert(|a| >= |b|);
    static BigInt subAbs(const BigInt& a, const BigInt& b);

    // 绝对值乘法：返回 |a| * |b|
    static BigInt mulAbs(const BigInt& a, const BigInt& b);

    // 绝对值乘以一个32位无符号整数
    static BigInt mulAbsUint32(const BigInt& a, uint32_t m);

    // 温馨提示：大整数除法实现难度较高 

    // 长除法辅助函数：在开头插入一个新的最低块   
    void shiftBaseAndAdd(uint32_t digit);

    // 绝对值除法与取模
    // 返回 {商， 余数}，均为非负
    // assert(b != 0);
    static std::pair<BigInt, BigInt> divmodAbs(const BigInt& a, const BigInt& b);

    // 从字符串读取整数
    void readFromString(const std::string& s);

public:
    /*========== Constructor Interface ==========*/
    /*==========      构造函数接口      ==========*/
    BigInt();
    BigInt(long long val);
    explicit BigInt(const std::string& s);

    // 返回绝对值
    BigInt abs() const;
    static BigInt abs(const BigInt& x);

    // 转成十进制字符串
    std::string toString() const;

    /*========== 输入输出流 ==========*/
    friend std::ostream& operator<<(std::ostream& os, const BigInt& x);
    friend std::istream& operator>>(std::istream& is, BigInt& x);

    /*========== 比较运算 ==========*/
    friend bool operator==(const BigInt& a, const BigInt& b);
    friend bool operator!=(const BigInt& a, const BigInt& b);
    friend bool operator<(const BigInt& a, const BigInt& b);
    friend bool operator<=(const BigInt& a, const BigInt& b);
    friend bool operator>(const BigInt& a, const BigInt& b);
    friend bool operator>=(const BigInt& a, const BigInt& b);

    /*========== 单目运算 ==========*/
    BigInt operator-() const;
    BigInt operator+() const;

    /*========== 四则运算 (五则运算bushi) ==========*/
    friend BigInt operator+(const BigInt& a, const BigInt& b);
    friend BigInt operator-(const BigInt& a, const BigInt& b);
    friend BigInt operator*(const BigInt& a, const BigInt& b);
    friend BigInt operator/(const BigInt& a, const BigInt& b);
    friend BigInt operator%(const BigInt& a, const BigInt& b);

    /*========== 复合赋值 ==========*/
    BigInt& operator+=(const BigInt& other);
    BigInt& operator-=(const BigInt& other);
    BigInt& operator*=(const BigInt& other);
    BigInt& operator/=(const BigInt& other);
    BigInt& operator%=(const BigInt& other);

    /*========== Increment and Decrement ==========*/
    /*==========         自增自减         ==========*/
    BigInt& operator++();    // prefix  ++x
    BigInt  operator++(int); // postfix x++
    BigInt& operator--();    // prefix  --x
    BigInt operator--(int);  // postfix x--
};

#endif // BIGINT_H