# lecture 6的课后练习

**本人以此课后练习为例，给出了一种构造函数高效实现的惯例，即`pass-by-value + move`，详见代码文件，以下为摘要部分**

```cpp
// Best practice
// Constructor: uses "pass-by-value + move" idiom
// - When lvalue is passed: one copy (into parameter) + one move (into member)
// - When rvalue is passed: parameter is move-constructed + member is move-constructed => zero copies 
Course(std::string name, int credit, double diff)
    : _name(std::move(name)), _credit(credit), _diff(diff) {}
```
