#include "BigInt.h"

#include <algorithm>
#include <stdexcept>
#include <cctype>

// 此处宏定义用于 operator/ 与 operator% 的重载
// 提供不同 C++ 标准的实现
#if __cplusplus >= 201703L
    #define USE_STRUCTURED_BINDING 1    
#else
    #define USE_STRUCTURED_BINDING 0
#endif 

/*========== 数据状态维护 ==========*/

bool BigInt::isZero() const {
    return limbs.size() == 1 && limbs[0] == 0;
}

void BigInt::normalize() {
    // 1. 去掉高位多余的0
    while (limbs.size() > 1 && limbs.back() == 0) {
        limbs.pop_back();
    }

    // 2. 保证limbs不为空
    if (limbs.empty()) {
        limbs.push_back(0);
    }

    // 3. 0永远非负，避免出现 -0
    if (isZero()) {
        is_neg = false;
    }
}

/*========== 绝对值层操作 ==========*/

int BigInt::compareAbs(const BigInt& a, const BigInt& b) {
    // 先比较limb的数量
    if (a.limbs.size() != b.limbs.size()) {
        return (a.limbs.size() < b.limbs.size()) ? -1 : 1;
    }

    // assert(a.limbs.size() == b.limbs.size());
    // limb数量相同，则从高位往低位比较
    for (int i = static_cast<int>(a.limbs.size()) - 1; i >= 0; --i) {
        if (a.limbs[i] != b.limbs[i]) {
            return (a.limbs[i] < b.limbs[i]) ? -1 : 1;
        }
    }
    // 此处实现方式中迭代变量 i 为int类型
    // 不过在数字几百亿位的极端情况下
    // 强制类型转换有窄化范围的风险
    
    // 所以无符号数 size_t 的反向遍历有一种经典写法
    // for (size_t i = size(); i-- > 0; )

    // 而下面这种写法会死循环，因为无符号数永远不可能为负
    // for (size_t i = size() - 1; i >= 0; --i)
    
    // 此处遍历也可使用反向迭代器，此改进招募已经发布在 Issues  
    
    return 0;
}

BigInt BigInt::addAbs(const BigInt& a, const BigInt& b) {
    BigInt res;
    res.is_neg = false;
    res.limbs.clear();

    size_t n = std::max(a.limbs.size(), b.limbs.size());
    uint64_t carry = 0; // carry表示进位
    
    // 逐limb从低位向高位相加，并且处理进位
    for (size_t i = 0; i < n || carry; ++i) {
        uint64_t cur = carry;

        if (i < a.limbs.size()) cur += a.limbs[i];
        if (i < b.limbs.size()) cur += b.limbs[i];

        res.limbs.push_back(static_cast<uint32_t>(cur % BASE));
        carry = cur / BASE;
    }

    res.normalize();
    return res;
}

BigInt BigInt::subAbs(const BigInt& a, const BigInt& b) {
    // assert(compareAbs(a, b) >= 0);
    BigInt res;
    res.is_neg = false;
    res.limbs.clear();

    // 注意borrow是64位有符号整数
    int64_t borrow = 0; // borrow表示借位

    //逐limb从低位向高位做减法，并处理借位
    for (size_t i = 0; i < a.limbs.size(); ++i) {
        int64_t cur = static_cast<int64_t>(a.limbs[i]) - borrow;
        if (i < b.limbs.size()) {
            cur -= b.limbs[i];
        }

        if (cur < 0) {
            cur += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }

        res.limbs.push_back(static_cast<uint32_t>(cur));
    }

    res.normalize();
    return res;
}

BigInt BigInt::mulAbs(const BigInt& a, const BigInt& b) {
    BigInt res;
    res.is_neg = false;

    // 结果最多有 a.size + b.size 个limb
    res.limbs.assign(a.limbs.size() + b.limbs.size(), 0);

    // 竖式乘法，时间复杂度是 O(a.size * b.size)
    // 此处有很多优化算法，本人实现一种未优化的基础版
    // 此处优化算法招募已经发布 Issues 中了
    // 欢迎各位朋友提出 Pull Request 呦
    for (size_t i = 0; i < a.limbs.size(); ++i) {
        uint64_t carry = 0; // carry表示进位
        
        for (size_t j = 0; j < b.limbs.size() || carry; ++j) {
            uint64_t cur = res.limbs[i + j] + carry;

            if (j < b.limbs.size()) {
                cur += static_cast<uint64_t>(a.limbs[i]) * b.limbs[j];
                // 注意：上一行必须先强制类型转换，然后再相乘
            }

            res.limbs[i + j] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }
    }

    res.normalize();
    return res;
}

BigInt BigInt::mulAbsUint32(const BigInt& a, uint32_t m) {
    if (m == 0) return BigInt(0);
    if (m == 1) return a.abs();

    BigInt res;
    res.is_neg = false;
    res.limbs.clear();

    uint64_t carry = 0; // carry表示进位

    for (size_t i = 0; i < a.limbs.size() || carry; ++i) {
        uint64_t cur = carry;
        if (i < a.limbs.size()) {
            cur += static_cast<uint64_t>(a.limbs[i]) * m;
        }

        res.limbs.push_back(static_cast<uint32_t>(cur % BASE));
        carry = cur / BASE;
    }

    res.normalize();
    return res;
}

// 长除法辅助函数
// current = current * BASE + digit
void BigInt::shiftBaseAndAdd(uint32_t digit) {
    // 由于低位在前存储
    // 乘 BASE 相当于整体向“高位方向”移动一块
    // 即在开头插入一个新的最低块
    if (isZero() && digit == 0) {
        return;
    }

    limbs.insert(limbs.begin(), digit);
    normalize();
}

// 长除法 + 二分查找试商
std::pair<BigInt, BigInt> BigInt::divmodAbs(const BigInt& a, const BigInt& b) {
    if (b.isZero()) {
        throw std::runtime_error("division by zero");
    }

    // 若|a| < |b|，则商为0，余数为a
    if (compareAbs(a, b) < 0) {
        return {BigInt(0), a};
    }

    BigInt divisor = b.abs();   // 除数的绝对值
    BigInt current(0);          // 长除法的“当前余数”
    BigInt quotient;            // 商
    quotient.is_neg = false;
    quotient.limbs.assign(a.limbs.size(), 0);

    // 从高位到低位做长除法
    for (int i = static_cast<int>(a.limbs.size()) - 1; i >= 0; --i) {
        // 1. current = current * BASE + a.limbs[i]
        current.shiftBaseAndAdd(a.limbs[i]);

        // 2. 在[0, BASE)中二分查找第一个不满足 divisor * x <= current 的位置
        // 最终当前商位就是 left - 1
        uint32_t left = 0;
        uint32_t right = BASE;

        while (left < right) {
            uint32_t mid = left + (right - left) / 2;
            BigInt prod = mulAbsUint32(divisor, mid); // 除数 * 试商值

            if (compareAbs(prod, current) <= 0) {
                left = mid + 1; // mid合法，继续向右找更大的合法值
            } else {
                right = mid;    // mid不合法，答案一定在左侧
            }
        }

        // 3. 把当前位商存入结果（低位在前）
        // left是第一个不合法的位置，所以当前位商为 left - 1
        quotient.limbs[i] = left - 1;

        // 4.更新余数 
        // current = current - divisor * quotient.limbs[i]
        if (quotient.limbs[i] != 0) {
            current = subAbs(current, mulAbsUint32(divisor, quotient.limbs[i]));
        }
    }

    quotient.normalize();
    current.normalize();
    return {quotient, current};
}


/*========== 字符串读入 ==========*/

void BigInt::readFromString(const std::string& s) {
    is_neg = false;
    limbs.clear();

    size_t pos = 0;

    // 跳过前导空白
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
        ++pos;
    }

    if (pos == s.size()) {
        throw std::invalid_argument("empty string");
    }

    // 读取符号
    bool neg = false;
    if (s[pos] == '+' || s[pos] == '-') {
        neg = (s[pos] == '-');
        ++pos;
    }

    // 跳过前导0
    while (pos < s.size() && s[pos] == '0') {
        ++pos;
    }

    // 收集纯数字部分
    std::string digits;
    while (pos < s.size() && !std::isspace(static_cast<unsigned char>(s[pos]))) {
        if (!std::isdigit(static_cast<unsigned char>(s[pos]))) {
            throw std::invalid_argument("invalid character in integer string");
        }
        digits.push_back(s[pos]);
        ++pos;
    }

    // 若后面还有非空白字符，则视为非法
    while (pos < s.size()) {
        if (!std::isspace(static_cast<unsigned char>(s[pos]))) {
            throw std::invalid_argument("invalid trailing characters");
        }
        ++pos;
    }

    // 若没有有效数字，说明输入的是 0 / +0 / -0 / 0000
    if (digits.empty()) {
        limbs = {0};
        is_neg = false;
        return;
    }

    // 从后往前每 BASE_DIGITS 位切一块
    for (int i = static_cast<int>(digits.size()); i > 0; i -= BASE_DIGITS) {
        int start = std::max(0, i - BASE_DIGITS);
        int len = i - start;
        uint32_t block = static_cast<uint32_t>(std::stoul(digits.substr(start, len)));
        limbs.push_back(block);
    }

    is_neg = neg;
    normalize();
}


/*========== Constructor ==========*/
/*==========   构造函数   =========*/

BigInt::BigInt(): is_neg(false), limbs{0} {}

BigInt::BigInt(long long val): is_neg(false) {
    long long x = val;
    is_neg = (x < 0);
    limbs.clear();

    // 注意 -LLONG_MIN 会溢出，所以要先转换成无符号方式处理
    unsigned long long ux;
    if (x < 0) {
        ux = static_cast<unsigned long long>(-(x + 1)) + 1ULL;
    } else {
        ux = static_cast<unsigned long long>(x);
    }

    if (ux == 0) {
        limbs.push_back(0);
        is_neg = false;
        return;
    }

    while (ux > 0) {
        limbs.push_back(static_cast<uint32_t>(ux % BASE));
        ux /= BASE;
    }

    normalize();
}

BigInt::BigInt(const std::string& s) {
    readFromString(s);
}

/*========== Basic Interface ==========*/
/*==========    基础接口      =========*/

BigInt BigInt::abs() const {
    BigInt t = *this;
    t.is_neg = false;
    return t;
}

BigInt BigInt::abs(const BigInt& x) {
    return x.abs();
}

std::string BigInt::toString() const {
    if (isZero()) return "0";

    std::string s;
    if (is_neg) s += "-";

    // 最高位块直接输出
    s += std::to_string(limbs.back());

    // 后续每一块都补齐到9位
    for (int i = static_cast<int>(limbs.size()) - 2; i >= 0; --i) {
        std::string part = std::to_string(limbs[i]);
        s += std::string(BASE_DIGITS - static_cast<int>(part.length()), '0') + part;
    }

    return s;
}

/*========== 输入输出流 ==========*/

std::ostream& operator<<(std::ostream& os, const BigInt& x) {
    os << x.toString();
    return os;
}

std::istream& operator>>(std::istream& is, BigInt& x) {
    std::string s;
    is >> s;
    x.readFromString(s);
    return is;
}

/*========== 比较运算 ==========*/

bool operator==(const BigInt& a, const BigInt& b) {
    return a.is_neg == b.is_neg && a.limbs == b.limbs;
}

bool operator!=(const BigInt& a, const BigInt& b) {
    return !(a == b);
}

bool operator<(const BigInt& a, const BigInt& b) {
    // 先比较符号，这也正是符号与绝对值解耦的价值之一
    if (a.is_neg != b.is_neg) return a.is_neg;

    // assert(a.is_neg == b.is_neg);
    // 符号相同，再比较绝对值
    int cmp = BigInt::compareAbs(a, b);

    // 均为非负：绝对值小的更小
    if (!a.is_neg) {
        return cmp < 0;
    }

    // 均为负数：绝对值大的更小
    return cmp > 0;
}

bool operator<=(const BigInt& a, const BigInt& b) {
    return !(b < a);
}

bool operator>(const BigInt& a, const BigInt& b) {
    return b < a;
}

bool operator>=(const BigInt& a, const BigInt& b) {
    return !(a < b);
}

/*========== 单目运算 ==========*/

BigInt BigInt::operator-() const {
    BigInt res = *this;
    // 禁止出现 -0
    if (!res.isZero()) {
        res.is_neg = !res.is_neg;
    }
    return res;
}

BigInt BigInt::operator+() const {
    return *this;
}

/*========== 四则运算 ==========*/

BigInt operator+(const BigInt& a, const BigInt& b) {
    // 同号：直接做绝对值加法，结果符号与原来相同
    if (a.is_neg == b.is_neg) {
        BigInt res = BigInt::addAbs(a, b);
        res.is_neg = a.is_neg;
        res.normalize();
        return res;
    }

    // 异号：转化为绝对值减法
    int cmp = BigInt::compareAbs(a, b);

    if (cmp == 0) {
        return BigInt(0);
    }

    if (cmp > 0) {
        BigInt res = BigInt::subAbs(a, b);
        res.is_neg = a.is_neg;
        res.normalize();
        return res;
    } else { // assert(cmp < 0);
        BigInt res = BigInt::subAbs(b, a);
        res.is_neg = b.is_neg;
        res.normalize();
        return res;
    }
}

BigInt operator-(const BigInt& a, const BigInt& b) {
    // a - b 等价于 a + (-b)
    return a + (-b);
}

BigInt operator*(const BigInt& a, const BigInt& b) {
    if (a.isZero() || b.isZero()) {
        return BigInt(0);
    }

    BigInt res = BigInt::mulAbs(a, b);
    res.is_neg = (a.is_neg != b.is_neg);
    res.normalize();
    return res;
} 

BigInt operator/(const BigInt& a, const BigInt& b) {
    if (b.isZero()) {
        throw std::runtime_error("division by zero");
    }

    if (a.isZero()) {
        return BigInt(0);
    }

    // 此处提供不同 C++ 标准的实现
    // 如果编译器支持 C++17 则使用结构化绑定  
    #if USE_STRUCTURED_BINDING
        /*========== C++17 ==========*/
        auto [q, r] = BigInt::divmodAbs(a.abs(), b.abs());
        (void) r;   // 忽略余数，消除编译器警告
    #else
        /*========== C++11/14 ==========*/
        auto result = BigInt::divmodAbs(a.abs(), b.abs());
        BigInt q = result.first;    // 商
    #endif

    q.is_neg = (a.is_neg != b.is_neg) && !(q == BigInt(0));
    q.normalize();
    return q;
}

BigInt operator%(const BigInt& a, const BigInt& b) {
    if (b.isZero()) {
        throw std::runtime_error("modulo by zero");
    }

    if (a.isZero()) {
        return BigInt(0);
    }

    // 此处提供不同 C++ 标准的实现
    // 如果编译器支持 C++17 则使用结构化绑定 
    #if USE_STRUCTURED_BINDING
        /*========== C++17 ==========*/
        auto [q, r] = BigInt::divmodAbs(a.abs(), b.abs());
        (void) q;   // 忽略商，消除编译器警告
    #else
        /*========== C++11/14 ==========*/
        auto result = BigInt::divmodAbs(a.abs(), b.abs());
        BigInt r = result.second;    // 余数
    #endif

    // 与 C++ 内建函数一致，余数符号与被除数相同
    r.is_neg = a.is_neg && !(r == BigInt(0));
    r.normalize();
    return r;
}

/*========== 复合赋值 ===========*/

BigInt& BigInt::operator+=(const BigInt& other) {
    *this = *this + other;
    return *this;
}

BigInt& BigInt::operator-=(const BigInt& other) {
    *this = *this - other;
    return *this;
}

BigInt& BigInt::operator*=(const BigInt& other) {
    *this = *this * other;
    return *this;
}

BigInt& BigInt::operator/=(const BigInt& other) {
    *this = *this / other;
    return *this;
}

BigInt& BigInt::operator%=(const BigInt& other) {
    *this = *this % other;
    return *this;
}

/*========== Increment and Decrement ==========*/

BigInt& BigInt::operator++() {
    *this += BigInt(1);
    return *this;
}

BigInt BigInt::operator++(int) {
    BigInt tmp = *this;
    ++(*this);
    return tmp;
}

BigInt& BigInt::operator--() {
    *this -= BigInt(1);
    return *this;
}

BigInt BigInt::operator--(int) {
    BigInt tmp = *this;
    --(*this);
    return tmp;
}
