#include <iostream>
#include <string>
#include <utility>

class Course {
public:
    // Best practice
    // Constructor: uses "pass-by-value + move" idiom
    // - When lvalue is passed: one copy (into parameter) + one move (into member)
    // - When rvalue is passed: parameter is move-constructed + member is move-constructed => zero copies 
    Course(std::string name, int credit, double diff)
        : _name(std::move(name)), _credit(credit), _diff(diff) {}
    
    explicit operator std::string() const { return _name; }
    explicit operator int() const { return _credit; }
    explicit operator double() const { return _diff; }

    friend std::ostream& operator<<(std::ostream& os, const Course& c) {
        os << "Course: " << c._name << ", Credit: " << c._credit << ", Diff: " << c._diff;
        return os;
    }

private:
    std::string _name;
    int _credit;
    double _diff;
};

int main() {
    Course oop("Liu", 2, 99.6);
    std::cout << oop << std::endl;
    std::cout << static_cast<std::string>(oop) << std::endl;
    std::cout << static_cast<int>(oop) << std::endl;
    std::cout << static_cast<double>(oop) << std::endl;
    return 0;
}

