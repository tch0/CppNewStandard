<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [并发支持库](#%E5%B9%B6%E5%8F%91%E6%94%AF%E6%8C%81%E5%BA%93)
  - [内存模型](#%E5%86%85%E5%AD%98%E6%A8%A1%E5%9E%8B)
  - [无锁编程](#%E6%97%A0%E9%94%81%E7%BC%96%E7%A8%8B)
  - [线程](#%E7%BA%BF%E7%A8%8B)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# 并发支持库

- 支持多线程的内存模型。
- 三种粒度和层级的并发编程支持：
    - 无锁编程`memory_order atomic atomic_thread_fence`等。
    - 基于线程的并发支持：`thread condition_variable mutex`。
    - 基于任务的并发支持：`future promise packaged_task async`。


## 内存模型

C++引入了内存模型使我们不必考虑诸如下列问题：
- 多线程中多个全局变量位于同一个内存位置（同一个最小CPU存取单元中，比如一个字）。
- 多线程中多级缓存数据不一致的影响。
- 现代CPU中的指令重排对多线程的影响。
- 但多线程中的数据竞争仍需要显式的同步或锁处理。

内存顺序：

定义于`<atomic>`：
```C++
// C++11
typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;
// C++20
enum class memory_order : /*unspecified*/ {
    relaxed, consume, acquire, release, acq_rel, seq_cst
};
inline constexpr memory_order memory_order_relaxed = memory_order::relaxed;
inline constexpr memory_order memory_order_consume = memory_order::consume;
inline constexpr memory_order memory_order_acquire = memory_order::acquire;
inline constexpr memory_order memory_order_release = memory_order::release;
inline constexpr memory_order memory_order_acq_rel = memory_order::acq_rel;
inline constexpr memory_order memory_order_seq_cst = memory_order::seq_cst;
```
- 主要用于原子数据类型的操作和内存栅栏（内存屏障）等操作中。
- 更多细节和含义我也还不懂。

## 无锁编程

首先：
- 无锁编程是真正的专家领域，非专家不要轻易尝试使用原子类型作为单纯的跨线程计数器之外的无锁编程。
- 无锁编程通常是和硬件高度相关、不可跨平台移植的，除非是深入理解硬件的专家，否则不要轻易尝试。
- 就算是专家都需要大量的时间、大量测试才能够保证无锁程序的可用性，无锁程序通常都会是bug制造机、不可预测的噩梦源泉。
- 无锁编程即是指多个线程之间共享数据，但是依赖数据的原子操作来保证并发正确性与线程安全，不使用任何形式的锁、信号量等阻塞性的同步工具。任何时刻都不会有任何程序因为同步问题处于阻塞状态。
- 无锁编程通常用于**极端性能敏感**的情况，现实中这种情况少之又少，总之不要轻易尝试通用的无锁编程。
- 但是在多个线程间用一个原子计数器之类比较简单的场景还是比较容易使用和预测的。

无锁编程相关设施基本都定义于`<atomic>`：
```C++
// Defined in header <atomic>
template< class T >
struct atomic; // (1)	(since C++11)
template< class U >
struct atomic<U*> // (2)	(since C++11) // Defined in header <memory>
template< class U >
struct atomic<std::shared_ptr<U>> // (3)	(since C++20)
template< class U >
struct atomic<std::weak_ptr<U>>; // (4)	(since C++20 // Defined in header <stdatomic.h>
#define _Atomic(T) /* see below */ // (5)	(since C++23)
```
- `std::atomic`对基本整数类型和指针做了特化，并且通过`_Atomic`宏对应于C标准的相应名称，每个特化都直接对应的C标准的一个类型，C标准的东西不赘述，这里只讨论C++的东西。


`std::atomic`操作：
- 赋值。
- 是否无锁：`is_lock_free`。
- 存取`load store`。
- 交换`exchange`。
- 比较并交换：`compare_exchange_weak compare_exchange_strong`。
- 对于整数：原子`+ - & | ^ ++ -- += -= &= |= ^=`。
- 对于指针：原子`+ - ++ -- += -=`。
- 每个操作都对应一个C标准中的一个函数。

原子标志`std::atomic_flag`：
- 原子布尔类型，不同于所有`std::atomic`的特化，保证无锁（`std::atomic`是否有锁由实现与平台决定，不过一般来说内置类型都是无锁的）。
- 不同于`std::atomic<bool>`，不提供加载存储操作。
- 可移植的原子默认初始化：`std::atomic_flag fl {ATOMIC_FLAG_INIT};`
- 测试并设置（并且返回旧值）：`test_and_set`。
- 清除：`clear`。
- C++20提供操作：
    - 测试（原子地返回当前值）`test`。
    - 等待（阻塞线程直到值更改）`wait`。
    - 通知其他线程值已更改：`notify_one notify_all`。
- 可在用户空间实现自选互斥。

内存栅栏/内存屏障：
- `std::atomic_thread_fence(order)`。
- `std::atomic_signal_handler(order)`。

`volatile`：
- `volatile`（易挥发性存储）在内存模型中没有任何含义，也不用于任何形式的线程同步机制。
- `volatile`只是指出变量可能被线程之外的其他东西（基本上说的就是硬件）更改，必须执行每一次该变量的访存操作，而不能将其中的某些访存操作合并优化掉。
- 除了直接操作硬件的程序（比如驱动程序、嵌入式程序）中，不应该在任何其他地方使用`volatile`。
- 某些语言中比如java用`volatile`这个关键字来表示某种线程同步机制，但C++中并非如此（而很多人却以为如此拿来这么用），这个关键字被广泛误解也是因为这个原因。
- 线程间数据同步请使用`atomic condition_variable mutex`。

## 线程

线程类，定义与`<thread>`：
```C++
class thread;
```
- 类型：
    - `id`轻量可平凡赋值线程id类型，可用作关联容器关键字。
    - `native_handle_type`实现定义的句柄类型。
- 构造：
    - 默认构造一个没有任务的线程。
    - 不可拷贝，只能移动。
    - `thread t(f, args...)`，创建可调用对象调用特定参数作为任务的线程。
- 观察器：
    - `jionable`，检查对象是否标识一个活跃的执行线程，具体来说就是`get_id() != std::thread::id()`返回`true`。默认构造的线程不可结合。结束执行代码，但仍未结合的线程可被当做活跃的执行线程，从而可以结合。
    - `get_id`：获取id。
    - `native_handle`：返回实现定义的底层线程句柄。
    - `hardware_concurrency`：静态方法，返回实现支持的并发线程数，应该只当做提示。比如四核八线的intel x86处理器则返回8。
- 操作：
    - `join`：阻塞当前线程直至`*this`标识线程执行结束，即等待某个线程运行到结束。运行结束后`jionable()`返回`false`。不可对当前线程、不可结合的线程调用。
    - `detach`：从`thread`对象分离执行线程，允许执行独立地持续，一旦该线程退出，则释放任何分配资源。调用后`*this`不再占有该线程。不可结合则会抛出错误，调用后即不可结合。
    - `swap`：交换两个线程的底层句柄。`std::swap`特化。

说明：
- 身份`id`：
    - 每个线程的身份标识是`thread::id`，每个有任务的线程唯一，默认没有任务（默认构造）的时候、运行结束（`join`）之后、被移动后、分离（`detach`）之后就是`id{}`，通过`get_id`获取。也可以在线程内部通过`std::this_thread::get_id()`获取。
    - 每个`thread`都有一个`id`，但是系统线程可能没有`id`但是可以继续运行，比如在`detach`之后。
    - `std::thread::id`可以比较、输出、作为哈希表的键。
- 构造：
    - 构造的时候线程就立即开始运行，没有一个显式的线程运行操作。
    - 传递给`thread`的第一个参数后的函数参数会通过一个类似于`bind`的机制绑定到函数对象上，也就是会拷贝过去（事实上如果将函数对象参数声明为引用会直接编译失败）。要传递引用需要使用lambda的引用捕获或者`std::ref std::cref`。
    - 将一个线程移动到另一个上不会影响其执行。
- 析构：
    - 当`std::thread`对象析构的时候，如果对象还是可结合的（也就是还没有执行完），那么会直接调用`std::terminate`。
    - `std::thread`不会在析构的时候自动`join`。
    - C++20引入了`std::jthread`则会在结束时自动结合，并且还能够取消/停止。
- 结合`join`：
    - `std::thread::join`会告诉当前线程等待特定线程直到其运行结束。
    - 如果析构时没有`join`则会调用`std::terminate`。
    - 考虑使用RAII类来管理将`std::thread`作为资源管理，或者使用析构时会自动join的`std::jthread`。
    - 可以这样定义这个RAII类：
    ```C++
    struct guarded_thread : std::thread
    {
        using std::thread::thread;
        ~guarded_thread()
        {
            if (this->joinable())
            {
                this->join();
            }
        }
    };
    ```
    - 当然如果C++20可用，那么最好使用`std::jthread`。
    - 那为什么不让线程在析构时自动结合呢？因为长期存在让线程自己决定要长期运行还是什么时候结束的传统。比如定时器程序或者系统中的守护进程（daemons）。
- 分离`detach`：
    - 在不分离线程的情况下尝试使线程执行到超出其析构函数被视为严重的错误。
    - 如果真的需要一个底层系统线程比它对应的`std::thread`存活更长时间，那么需要使用`detach`，分离后`get_id`得到`id{}`，不再可结合。
    - `detach`会让程序变得难以调试和容易出错，但某些时候确实是有用的。
    - `std::thread`可以移动，所以可以通过返回值传递出局部作用域，这在某种程度上可以作为`detach`的替代。
    - 如果要`detach`一个线程，确保这个线程不要引用局部作用域中的变量（典型例子是lambda不要引用捕获局部作用域的东西）。这种情况很可能会导致往栈上写无效数据，导致程序宕掉。这种数据共享必须非常谨慎小心地处理，稍有不慎就可能出问题。
- 命名空间`this_thread`：
    - `get_id()`：获取当前运行的线程的id。
    - `yield()`：通知/建议调度器让出当前线程的时间片，让其他线程能够得到运行。
    - `sleep_until(tp)`：当前线程睡眠到`time_point tp`。
    - `sleeep_for(d)`：当前线程睡眠一段时间(`duration d`)。
    - `yield`在等到原子变量改变状态或者多线程之间合作非常有用。
    - 但通常来说使用`sleep_for`睡眠一段时间可能比仅仅使用`yield`更有用，`yield`可以看做是非常罕见和特定的场景下的优化措施。
    - 大多数实现中线程实现为抢占式的，由实现确保每个线程得到合理数量的时间片。
    - 通常来说，程序不应该依赖于系统时钟，如果系统时钟被修改，可能会影响`timed_mutex`的`wait_until`接口，但是`wait_for`不会受到影响。在编写多线程程序时，通常来说需要考虑系统时钟被修改的影响。
- 杀死线程：
    - 某些时候，杀死一个线程是很有用的，这表示对这个线程的任务已经不感兴趣，直接让该线程释放用到的资源并结束就行。
    - 在不同的系统中，这个操作也叫做杀死(kill)、取消(cancel)、中断(interrupt)一个线程。
    - 由于各种各样的历史原因和技术原因，`std::thread`没有提供这个操作。
    - 不过可以在用户层去自己实现这个东西，比如说许多线程实现时都涉及到一个请求循环，接受不同的请求，然后依次进行处理，那么可以将杀死线程作为一个请求发送给该线程，然后该线程释放资源，结束运行。
    - 如果线程中没有一个请求循环这种东西，那么可以通过定期检验一个是否需要杀死线程的原子变量来实现，由主线程或者其他线程进行设置。
    - 通用的跨平台的取消操作可能难以实现，但是在特定的场景下一个特定的取消操作还是很简单的。
- `thread_local`数据：
    - `thread_local`用于静态生命周期对象时，就变成了线程存储期对象，每个线程将会有一份。更具体来说用于静态对象（局部、类）、全局对象、命名空间作用域对象。
    - `thread_local`在每个线程中存在一份，在每个线程中会在第一次使用前初始化，相当于每个线程中调用初始化语句一次。如果这个初始化语句有状态，那么不同线程中这个变量的初值可能不同，比如`std::atomic<int> var = 10; thread_local val = var++;`
    - 不要误解为线程创建时才从当前变量创建一个分支，使用当前进程的值初始化，这明显是不合理的。
    - 对于每个线程需要一份，但是比较大不适合存储在栈里面的数据，最好方式就是使用`thread_local`。
    - 就像`static`变量一样，`thread_local`变量默认初始化为0。
    - 对于静态局部的`thread_local`变量，第一次运行时才会初始化。多个`thread_local`变量的构造顺序是不确定的，最好不要有初始化依赖关系，有依赖的话同样可以使用静态局部变量实现的单例模式。
    - `thread_local`变量不会在多个线程间共享，也就不会有数据竞争。但是`thread_local`不是解决数据竞争的灵丹妙药，如果一个静生命周期对象在语义上就是用与多线程间共享数据的，那么使用`thread_local`实际上改变了语义，大概率是一个错误选项。简单的将共享的对象改成`thread_local`可能是有问题的，需要仔细斟酌。
    - 最后注意静态成员变量、局部静态变量都属于静态生命周期，同全局变量一样都会在多个线程中共享。
