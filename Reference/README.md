# 引用绑定规则（Reference Binding Rule）

## 核心应用：指导开发者设计函数接口

| 形参类型 | 实参：**lvalue** | **const lvalue** | **rvalue** | **const rvalue** |
| :--- | :--- | :--- | :--- | :--- |
| **`T`** | ✅ 拷贝构造 | ✅ 拷贝构造 | ✅ 优先移动，否则拷贝 | ✅ 拷贝构造 |
| **`T&`** | ✅ 可修改 | ❌ 不能绑定 | ❌ 不能绑定 | ❌ 不能绑定 |
| **`const T&`** | ✅ 只读 | ✅ 只读 | ✅ 只读（延长生命周期） | ✅ 只读（延长生命周期） |
| **`T&&`** | ❌ 不能绑定 | ❌ 不能绑定 | ✅ 可修改 | ❌ 不能绑定 |
| **`const T&&`** | ❌ 不能绑定 | ❌ 不能绑定 | ✅ 只读 | ✅ 只读 |

### 关键说明

- **`T`（值类型）**：虽然`T`并非引用，但是出现在此表格中，是本人考虑到引用绑定规则的实际应用，Reference Binding Rule一个很重要的应用是：**指导开发者设计函数接口**。
- 但是函数接口的形参（parameter）并非只有引用（reference），还有指针（pointer），值（value）。
- 那么在这种实际使用的过程中，将不可避免地把`T`与引用并列在一起考虑，以实现形参（parameter）与实参（argument）的最佳适配

### 接口设计速查建议

| 设计意图 | 推荐形参类型 | 理由 |
| :--- | :--- | :--- |
| **只读观察，不拷贝** | `const T&` | 绑定范围最广，开销最小（仅引用） |
| **修改实参，或输出结果** | `T&` | 仅接受左值，语义明确 |
| **必须获取对象所有权（拷贝或移动）** | `T` | 值语义，清晰表达“函数内部需要独立副本” |
| **只想处理临时对象（右值）** | `T&&` | 专门捕获右值以实现移动优化 |

---

### 💡 工程实践中的避坑指南：警惕 const rvalue

虽然表格在语法层面是完备的，但在**实际工程应用**中，应极力避免出现 `const rvalue` 或使用 `const T&&` 形参：

1.  **移动语义的“杀手”**：
    右值的核心价值在于“移动（Move）”而非“拷贝（Copy）”。由于 `const` 属性具有强制性，常见的移动构造/赋值（通常签名为 `T(T&&)` / `T& operator=(T&&)`）无法接收 `const rvalue`，因此很多场景下会退化为拷贝构造/赋值；当然，在某些情况下也可能被拷贝消除（如 RVO/NRVO）。
    
2.  **函数返回值的陷阱**：
    在设计接口时，按值返回通常应写成 `T`，而不是为了所谓的“安全”返回带顶层 `const` 的对象（例如：`const T get_data()`）。这会导致调用者在接收该返回值时，无法触发移动构造函数来优化性能。

3.  **`const T&&` 的尴尬地位**：
    该类型在语法上是为了保证 C++ 类型的正交性而存在，但在 99% 的工程场景下都属于“反模式（Anti-pattern）”。如果你发现代码中需要用到它，通常意味着接口的逻辑设计可能存在问题。

```mermaid
graph TD
    Start[开始：设计函数接口] --> IsPrimitive{是否为基础类型?<br>int/ptr/double等}
    
    IsPrimitive -- 是 --> PassByValue[使用 T<br>按值传递]
    IsPrimitive -- 否 --> NeedToModify{是否需要<br>修改原对象?}
    
    NeedToModify -- 是 --> PassByRef[使用 T&<br>左值引用]
    NeedToModify -- 否 --> NeedLocalCopy{是否需要在函数<br>内部保存副本?}
    
    NeedLocalCopy -- 否 --> PassByConstRef[使用 const T&<br>常量左值引用]
    NeedLocalCopy -- 是 --> SinkPattern[使用 T 并 std::move<br>或提供 T&& 重载]

    style PassByValue fill:#f9f,stroke:#333,stroke-width:2px
    style PassByRef fill:#bbf,stroke:#333,stroke-width:2px
    style PassByConstRef fill:#dfd,stroke:#333,stroke-width:2px
    style SinkPattern fill:#ffd,stroke:#333,stroke-width:2px
