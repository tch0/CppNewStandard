<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [约束与概念](#%E7%BA%A6%E6%9D%9F%E4%B8%8E%E6%A6%82%E5%BF%B5)
  - [大体语法](#%E5%A4%A7%E4%BD%93%E8%AF%AD%E6%B3%95)
  - [原子约束](#%E5%8E%9F%E5%AD%90%E7%BA%A6%E6%9D%9F)
  - [requires表达式](#requires%E8%A1%A8%E8%BE%BE%E5%BC%8F)
  - [requires子句](#requires%E5%AD%90%E5%8F%A5)
  - [约束的偏序](#%E7%BA%A6%E6%9D%9F%E7%9A%84%E5%81%8F%E5%BA%8F)
  - [标准概念库`<concepts>`](#%E6%A0%87%E5%87%86%E6%A6%82%E5%BF%B5%E5%BA%93concepts)
  - [注意事项](#%E6%B3%A8%E6%84%8F%E4%BA%8B%E9%A1%B9)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# 约束与概念

## 大体语法

首先：
- 语法细节：[约束与概念](https://zh.cppreference.com/w/cpp/language/constraints)
- [约束与概念笔记](https://github.com/tch0/CppTemplateProgramming/tree/master/AppendixE)
- 上面的笔记只是一个大致轮廓，很多细节都不准确，理解也不到位。这里探讨这些缺失的细节：比如析取与合取的细节、约束的偏序等。

然后需要明确：
- 概念是一个编译期谓词。
- 概念和`using`模板别名很类似，前者是对布尔表达式的别名，后者是对模板类型的别名。两者都不能被特化。
- 所以在实现比如分类之类的概念时，必须借助类型特征实现（因为概念不能特化）。

## 原子约束

约束的析取与合取：
- 约束的合取和析取，具有短路求值特性。
- 析取和合取表达式的基本运算单位是**原子约束**。
- 约束的析取`||`中，每一个单独的原子约束可以是非法的，如果其他约束合法，那么最终结果都会为真。合取同理。
```C++
struct Foo { using type = float; };
struct Bar { using type = int; };
struct Buz {};
template<typename T, typename U>
concept C1 = std::is_integral_v<typename T::type> || std::is_integral_v<typename U::type>;
static_assert(C1<Bar, Buz>);
```
- `C1`表达的含义是：两个类型中至少有一个具有整型的嵌套类型`type`。
- 其中的两个布尔表达式是**两个原子约束**，通过**析取**运算符`||`进行运算。
- 如果将整个表达式括起来（单纯括起来还是析取），并进行一个`bool`转换：
```C++
template<typename T, typename U>
concept C2 = bool(std::is_integral_v<typename T::type> || std::is_integral_v<typename U::type>);
static_assert(!C2<Bar, Buz>);
static_assert(C2<Foo, Bar>);
```
- 那么`||`将**不再表达析取的含义**，而是**布尔逻辑或**，而整个表达式成为一个原子约束。
- `C2`表达的含义是：两个类型都需要有嵌套类型`type`，且其中至少有一个是整型。

逻辑非运算符：
```C++
#include <iostream>
#include <concepts>
#include <type_traits>

template<typename T>
concept C1 = std::is_integral_v<typename T::type>; // has nested ::type and is integral

template<typename T>
concept C2 = !std::is_integral_v<typename T::type>; // has nested ::type but is not integral

template<typename T>
concept C3 = !C1<T>; // C3 is negation of C1, do not has ::type or has ::type but is not integral

struct Foo { using type = float; };

int main(int argc, char const *argv[])
{
    static_assert(C1<int> == false);
    static_assert(C2<int> == false);
    static_assert(C3<int> == true);
    static_assert(C1<Foo> == false);
    static_assert(C2<Foo> == true);
    static_assert(C3<Foo> == true);
    return 0;
}
```
- 这里的C2并不是C1的否定，虽然它看起来是。
- 也就是说这里的`!`表达的同样是逻辑运算否，整个表达式成为一个原子约束，首先要求整个表达式合法，然后才是表达式为真。
- C1表达的约束是：具有嵌套类型`type`，且这个类型是整型。
- C2表达的约束是：具有嵌套类型`type`，且这个类型不是整型。
- C3才是C1的否定，他们的是互斥的：没有嵌套类型`type`，或者具有嵌套类型`type`但不是整型。
- 需要非常注意其中细微的差别。
- 总体上来说，只有析取和合取才是比较特殊的，他们是由多个原子约束组成的，具有短路求值特性。

可变参数模板：
```C++
template<typename... Ts>
concept C1 = (std::is_integral_v<typename Ts::type> || ...);

static_assert(C1<Foo, Bar>);
static_assert(!C1<Bar, Buz>);
```
- 可变模板参数展开时每个整个表达式成为一个原子约束。
- 如果要表达其中一个类型具有整型嵌套类型`type`的含义，需要一层间接层次：
```C++
template<typename T>
concept NestedIntegralType = std::is_integral_v<typename T::type>;

template<typename... Ts>
concept C2 = (NestedIntegralType<Ts> || ...);

static_assert(C2<Foo, Bar>);
static_assert(C2<Bar, Buz>);
```

总结：
- **原子约束才是判断表达式是否合法的基本单位**。

## requires表达式

四种要求：
- 简单要求：要求表达式合法。
- 类型要求：要求类型合法。
- 复合要求：要求表达式合法且对类型和异常声明有要求。
- 嵌套要求：要求谓词为真。

注意事项：
- requires表达式可以用在任何需要布尔表达式的地方。
- 比如定义布尔类型编译期模板或者非模板常量、用在`if constexpr`中等。
- 除了支持模板参数，还支持普通类型作为`requires`表达式的形参类型。
- 还有很多场景可以出现`requires`表达式：定义`constexpr`谓词函数、`static_assert`中、实现`type_traits`时、`requires`子句中等。
- 如果`requires`表达式中的简单要求是一个布尔表达式，那么仅检查合法性。将布尔表达式用在嵌套要求中时，才检查谓词为真。
- 所以如果一个布尔表达式不一定合法，那么可以先检查合法性，再判断其值：
```C++
requires {
    bool-expression; // check validity
    requires bool-expression; // check it's value
}
```
- 如果说要在嵌套要求中使用参数。那么需要使用两个`requires`关键字。第一个说明这是嵌套要求，第二个`requires`表达布尔表达式。
```C++
requires(T v) {
    requires requires(typename T::value_type x) { ++x; };
}
```
- 也就是说其实**嵌套要求就是`requires`子句**。
- 如果去掉第一个`requires`，那么就变成了简单要求，变成了检查这个`requires`表达式是否合法。这和我们的本意完全不一样。（好像某些编译器这种时候会报错或者报警告）。
- 最后需要注意，如果`requires`表达式的参数列表中的表达式非法，那么程序非良构，而不是整个`requires`表达式返回false。
- 但是`requires`表达式作为`concept`定义的约束表达式时，如果参数列表中表达式非法，那么不是编译错误，而是返回假，这是concept的性质，而不是`requires`表达式的性质。可以看做特例。

## requires子句

`requires`子句：
- `requires`子句后同样跟一个布尔表达式，可以是概念、`requires`表达式、`constexpr`谓词常量或者函数、类型特征。
- 类型必须就是布尔，而不能是其他类型，即是能隐式转换为布尔。
- 需要注意在`requires`子句中使用否定时会有和前面约束表达式一样的问题：
```C++
template<typename T> requires (!std::is_integral_v<typename T::type>)
void h(T)
{
    std::cout << "h(T): constrained version" << std::endl;
}

template<typename T>
void h(T)
{
    std::cout << "h(T): normal version" << std::endl;
}

struct Foo { using type = float; };
struct Bar { using type = int; };
struct Buz {};

h(Foo()); // constrained version
h(Bar()); // normal version
h(Buz()); // normal version
```
- 对约束进行否定必须添加圆括号。非概念或者类型特征的其他表达式也可能需要使用括号，比如函数调用。
- 首先第一步都是判断表达式是否合法，这里的受约束版本表达的含义：存在嵌套类型`type`且不为整型。
- 如果要修改为不含整型嵌套类型`type`的含义，可以参考前面增加一层间接层次。
- `requires`子句中也可以使用析取和合取，和概念定义的约束表达式一个道理。如果将整个`||`表达式转换为`bool`再括起来，那么将不再是析取表达式而是一个原子约束，那么含义可能也随之发生改变。
- 例子见：[RequiresClause.cpp](RequiresClause.cpp#L168)

概念的简写：
- 在比较简单的情况下，可以用概念替代`typename`关键字，此时将直接用对应模板参数填充概念的第一个模板实参。类型特征是做不到这一点的。
- 针对多个模板形参的多个简写等价于`requires`子句中的一个合取表达式。
```C++
template<std::integral T, std::integral U> // equal to: requires (integral<T> && integral<U>)
void f(T, U);
```
- 合取的时候以简写，析取则不能，只能写在`requires`子句中。
- 在C++20中，如果不关心模板参数类型（其实也可以用`decltype`获取），函数模板的参数可以用`auto`简化，并且同时支持添加约束，上面的例子等价于：
```C++
void f(std::integral auto t, std::integral auto u);
```
- 但他们并不严格意义等价，如果声明到一起，那么构成重载，而不是视为相同的声明。因为约束相同重载决议时就会有歧义。（与`requires`版本也是这样，功能等价，但声明并不等价）。
- 泛型lambda也能够使用概念进行约束：在约束的概念后添加`auto`（或者`decltype(auto)`）以声明lambda形参。
```C++
auto h = [](std::integral auto lhs, std::integral auto rhs) {
    std::cout << "h(T, U)" << std::endl;
};
```

类模板的约束：
- 模板类和它的特化版本（一般是偏特化）也能够通过`requires`子句增加约束，根据约束决策出约束最强的版本。
```C++
template<typename T>
struct A
{
    void foo()
    {
        std::cout << "primary template" << std::endl;
    }
};

template<std::integral T>
struct A<T>
{
    void foo()
    {
        std::cout << "integral version" << std::endl;
    }
};
```
- `requires`子句也可以用于类模板的普通成员：
```C++
template<typename T>
struct A
{
    void operator()() requires std::integral<T>
    {
        std::cout << "A::operator(): integral version" << std::endl;
    }
    void operator()() requires (!std::integral<T>)
    {
        std::cout << "A::operator(): non-integral version" << std::endl;
    }
};
```
- 在实例化时将会考虑约束，显式实例化时不满足约束的成员将不会生成代码。

## 约束的偏序

约束的偏序：
- 当同一套模板参数满足多个函数模板或者类模板偏特化的类型约束时，可以通过约束的偏序关系进行重载决议或者类模板特化决策。
- 当然前提是函数模板重载或者类模板偏特化是同一形式，如果函数模板重载或者类模板偏特化形式不同，他们本身就具有偏序关系，那么结果已经被重载或者偏特化的偏序关系决定了，还轮不到约束的偏序关系上场。
- 例子：
```C++
template<typename T> requires std::integral<T>
void f(T*) // 1
{
    std::cout << "f(T*)" << std::endl;
}

template<typename T>
void f(const T*) // 2
{
    std::cout << "f(const T*)" << std::endl;
}
```
- 重载版本1和版本2本身具有偏序关系，版本2更特化：
```C++
const int a = 0;
f(&a); // f(const T*)
```
- 虽然版本1的约束更强，但是根据重载的偏序就已经能决议了。
- 如果两个版本声明相同，但是约束不同，那么就到约束的偏序出场了：
```C++
template<typename T> requires std::integral<T>
void g(T*)
{
    std::cout << "g(T*): constrianed version" << std::endl;
}

template<typename T>
void g(T*)
{
    std::cout << "g(T*): normal version" << std::endl;
}

const int a = 0;
g(&a); // g(T*): constrained version
```
- 如果两个重载本身不具有偏序关系，且同等程度匹配，都满足约束但其中一个约束更弱。那么直接就产生歧义报错，而不会看他们约束强弱关系。
- 所以约束的偏序参与重载决议一定是函数或者偏特化具有同一声明形式的情况下。这是大前提。另外如果是类模板偏特化，那么必须是偏特化的约束比主模板更强，而不是主模板比偏特化约束更强。

约束表达式归一化：
- 对于受约束的函数模板、类模板来说，施加在他们上面的约束表达式称之为关联约束（associated constraint）。
- 为了进一步判断约束的偏序，需要将关联约束分解成原子约束的合取与析取形式，这个过程称之为**归一化**（normalization）。
- 前面提到概念只是约束表达式的别名，在归一化时会对概念进行展开，展开后若还包含概念，则会进一步递归展开，直到所有约束都无法展开为止，这些约束就是**原子约束**（atomic constraint）。那么最终形式就是原子约束的合取与析取表达式，每个原子约束既不是合取也不是析取。

简单约束的包含关系：
- 约束的偏序关系也称之为包含关系（subsumption），若P包含Q而Q不包含P，则P比Q更优，也就是P比Q约束更强。
- 约束表达式P包含Q，当且仅当P满足要求时Q也满足，但反之则不然。
- 显而易见，对于单纯的合取和析取表达式：
    - 约束合取表达式比其中的原子约束更强。
    - 原子约束比他们的析取更强。
- 这里的包含这个词感觉和集合包含关系完全是反过来的，我不太习惯这样称呼，有点反直觉。所以后面不会再使用这个词。

原子约束的相等关系：
- 当面临复杂的约束表达式，首先会进行归一化。
- 那么判断归一化后的原子约束的之间是否存在相同（identical）关系就是重点：
- 原子约束A和B的相等关系定义为：**是否词法上相等且来自于同一个concept**。
- 例子：
```C++
template<typename T> requires std::is_integral_v<T> && std::is_signed_v<T>
void f(T)
{
    std::cout << "f(T): signed integral version" << std::endl;
}
template<typename T> requires std::is_integral_v<T>
void f(T)
{
    std::cout << "f(T): integral version" << std::endl;
}
```
- 这两个重载的约束就不具有包含关系，因为原子约束`std::is_integral`并不来自于同一个概念。传入`int`调用时就会产生歧义。
- 改成使用标准概念库中的`std::integral`后则具有偏序关系：
```C++
template<typename T> requires std::integral<T> && std::is_signed_v<T>
void g(T) // 1
{
    std::cout << "g(T): signed integral version" << std::endl;
}
template<typename T> requires std::integral<T>
void g(T) // 2
{
    std::cout << "g(T): integral version" << std::endl;
}
```
- 此时传入`int`将调用版本2。
- 在使用概念时，编译器才会尝试去计算原子约束之间的关系，这是概念所具有的独特性质。
- 所以说实践中如果有基于约束的有包含关系的重载最好是将其中的原子约束定义为一个个概念，使用合取析取组合他们，以便包含关系的判断。而不是直接复用类型特征等编译期常量作为原子约束。
- 来自同一个概念已经理解了，但是词法上相等需要特别注意：比如对于`std::some_concept<T, U>`和`std::some_concept<U, T>`是等价的，但是词法上不相等，也会被判断为不相等。后面会介绍如何规避这个问题。

一般约束的包含关系：
- 归一化之后可以将约束表达式标准化为析取范式和合取范式。
    - 其中析取范式的析取子句为约束合取表达式。
    - 合取范式的合取子句为约束合取表达式。
- 比如`A && (B || C)`已经是合取范式，标准化为析取范式则是`(A && B) || (A && C)`（布尔代数的分配律）。
- 一般约束的包含关系判断方式：`P`包含`Q`，当且仅当`P`的析取范式中的每个析取子句`Pi`包含`Q`的合取范式的每个合取子句`Qj`。
    - 而其中的析取子句`Pi`包含合取子句`Qj`当且仅当`Pi`存在一个原子约束`Pia`且`Pia`与`Qj`的一个原子约束相同。
- 例子：判断已经标准化好的约束`P`为`(A && B) || (C && D)`是否包含约束`Q`为`(A || C) && (B || D)`。
    - `P`要包含`Q`也即是`P`中每个析取子句都需要包含`Q`中的每个合取子句，也即是必须满足：
    - `A && B`包含`A || C`。
    - `A && B`包含`B || D`。
    - `C && D`包含`A || C`。
    - `C && D`包含`B || D`。
    - 显然成立，所以`P`包含`Q`。
- 综上，当原子约束很多时，编译器检查约束包含关系时可能需要花费指数时间。
- 一般来说合取表达式用得会比较多，而析取相对来说用的少。减少析取的使用有助于降低概念的编译时间。

## 标准概念库`<concepts>`

标准概念库：
- C++20标准概念库`<concepts>`提供了一些基本概念，用于编译期进行参数校验和基于概念的函数重载。
- 同样概念只能检查语法约束，而语义上的约束需要程序员自行检查。

常用标准概念：
- `std::same_as`：与某类型相同。
    ```C++
    template<typename T>
    concept same_as = is_same_v<T>;
    ```
    - `same_as`很明显应该是对称的。但是因为上面说过的词法上不同导致不相等的问题。所以正确   实现应该是：
    ```C++
    template<typename T, typename U>
    concept _same_as = is_same_v<T, U>;
    template<typename T, typename U>
    concept same_as = _same_as<T, U> && _same_as<U, T>;
    ```
- **具有对称关系的概念都应该这样实现**。
- `std::derived_from`：派生自某类。
    ```C++
    template< class Derived, class Base >
    concept derived_from =
      std::is_base_of_v<Base, Derived> &&
      std::is_convertible_v<const volatile Derived*, const volatile Base*>;
    ```
    - 经常用在概念简写或者复合要求中写作`std::derived_from<Base>`。
- `std::convertible_to`：可隐式或者显式转化为某类。
    ```C++
    template <class From, class To>
    concept convertible_to =
      std::is_convertible_v<From, To> && // implicit conversion
      requires {
        static_cast<To>(std::declval<From>()); // explicit conversion
      };
    ```
    - 注意`std::is_convertible_v`只允许隐式转换，而`std::convertible_to`还包含了显式转换。
- 算术概念：`std::integral` `std::signed_integral` `std::unsigned_integral` `std::floating_point`，都是通过标准库类型特征组合实现的。
- 值概念：
    - 正规（regular）：`std::regular`，正规的概念是指类型看上去和基础数据类型一样，能够进行默认构造、移动构造与赋值、拷贝构造与赋值，并且能够判等的类型。
    - 半正规（semiregular）：`std::semiregular`，在正规基础上不要求能够判等。
- 可调用：
    - `std::invocable std::regular_invocable`表示可用指定参数调用函数的概念。他们的区别是纯语义的。
    - 另外还有`std::prediate`表示可调用对象返回布尔。
    - `std::relation`是仅接受两个参数的可调用对象。
    - 更进一步可以定义等价关系`std::equivalence_relation`和严格弱序`std::strict_weak_order`，他们和`std::relation`定义完全相同，但是语义不同。

## 注意事项

- 在概念之前，已经有了`static_assert`、`constexpr`函数与值、`if constexpr`还是标准库提供的类型特征等机制。这些机制能够实现概念提供的功能，但是这些机制都太多复杂，牵扯了太多模板的技巧与细节，就类似于模板元编程中的汇编代码。虽然够用，但是仅提供这些能力就会使程序变得复杂晦涩。
- 概念相对来说提供了更为高级的表达，在一个更高的层次上抽象。
    - 概念并不是专门针对泛型编程专家才能使用的高级特性。
    - 概念不仅仅是类型特征、`enable_if`、标签分发等方法的语法糖。
    - 概念是最基础的语言机制，最好在最初使用模板时就使用它。
- 在C++20以后，所有能够被概念取代的`enable_if void_t`、标签分发、类型特征都应该使用概念替代。
- 最后需要注意概念仅仅检查模板参数声明部分，而不会检查函数体。函数体中依然可以使用未被约束的操作。
- 换句话说，很多时候不太可能将所有用到的操作都使用概念表达出来。
- 概念设计的太宽泛则起不到约束的作用，设计的太细则难以应对变化，失去灵活性。
- 不同的概念尽量要设计得不同，但是很多时候区别是语义性的，无法通过语法表示出来。比如上面的`std::relation std::equivalence_relation std::strict_weak_order`，那也没有办法，这种时候概念语法没有区别，编译器会认为他们是相同的，设计成不同概念的好处仅仅是告诉程序员他们是有区别需要注意。
