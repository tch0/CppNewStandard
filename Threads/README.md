<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [并发支持库](#%E5%B9%B6%E5%8F%91%E6%94%AF%E6%8C%81%E5%BA%93)
  - [内存模型](#%E5%86%85%E5%AD%98%E6%A8%A1%E5%9E%8B)
  - [无锁编程](#%E6%97%A0%E9%94%81%E7%BC%96%E7%A8%8B)
  - [线程](#%E7%BA%BF%E7%A8%8B)
  - [避免数据竞争](#%E9%81%BF%E5%85%8D%E6%95%B0%E6%8D%AE%E7%AB%9E%E4%BA%89)
  - [互斥锁](#%E4%BA%92%E6%96%A5%E9%94%81)
  - [条件变量](#%E6%9D%A1%E4%BB%B6%E5%8F%98%E9%87%8F)
  - [任务](#%E4%BB%BB%E5%8A%A1)

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

线程类，定义于`<thread>`：
```C++
class thread;
```
- 类型：
    - `id`：轻量可平凡赋值线程id类型，可用作关联容器关键字。
    - `native_handle_type`：实现定义的操作系统底层线程句柄类型。
- 构造：
    - 默认构造一个没有任务的线程。
    - 不可拷贝，只能移动构造。
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


## 避免数据竞争

当多个线程需要对同一份数据进行读写时，就不可避免地会出现数据竞争：
- 首先，避免数据竞争最好的方法就是不要共享数据。具体方式是：
    - 只使用局部变量、`thread_local`数据以及仅在线程内部使用的动态数据。
    - 并且不要将这些数据的指针引用传递到其他线程。
    - 如果其他线程一定需要访问当前线程的数据，则将数据按指针传递到其他线程，并且在该线程处理完成前当前线程不要使用、不要释放这些数据。
- 上面的做法都是防止同时访问数据的手段，所以不需要任何锁来同步可以获得最高的性能。
- 但不是所有时候上面的做法都能够达成目的，那么在一定需要共享数据的时候就需要某种形式的锁用于数据同步：
    - 互斥锁（mutexes）：互斥锁是用来表示对某种资源的排他性访问的对象。如果资源通过互斥锁来同步，那么访问前需要先获取锁，然后访问，访问弯成后释放锁。
    - 条件变量（condition variables）：条件变量是线程用来等待其他线程的一个事件发生或者一个定时器到期的变量。严格来说，条件变量不能避免数据竞争，但是使用它某些时候可以让我们避免使用可能引起数据竞争的共享数据。

## 互斥锁

互斥锁（mutex）：
- 互斥锁用于避免数据竞争以及同步多个线程中对共享数据的访问。
- `std::mutex`：基本互斥锁，如果尝试获取已经被获取的`mutex`会阻塞当前线程。
- `std::timed_mutex`：相比普通互斥锁还提供尝试在某个一段时间内获取锁的操作，如果这段时间内没有获取到，则直接返回。
- `std::recursive_mutex`：可以被一个线程重复获取（递归获取）的互斥锁。
- `std::recursive_timed_mutex`：递归地、提供定时获取操作的互斥锁。
- `std::lock_guard<>`：互斥锁的RAII包装类。
- `std::unique_lock<>`：互斥锁的可移动RAII包装类。
- `std::scoped_lock<>`：同时获取多个锁、避免死锁的RAII包装类，和`std::lock`做同样的事情。具体来说就是同时尝试获取多个锁，如果其中有一个未获取到就释放所有已经获取到的锁，以避免死锁。
- 其中`std::mutex`是最简单、最小、最快的互斥锁，递归的和提供计时功能的互斥锁都会有一些额外开销。
- 任何时刻只有一个线程可以拥有（获取(acquire)后即为拥有，own）同一个互斥锁。
    - 如果一个锁没有被获取（acquire），那么获取它将获得他的所有权。如果一个锁已经被其他线程获取，那么获取它将会阻塞当前线程。
    - 释放（release）一个锁则代表放弃对其的所有权，释放锁后其他尝试获取该锁被阻塞的线程会获得该锁，从而解除阻塞，开始运行。
- 互斥锁本身并不做任何事情，我们使用互斥锁来表示操作某种资源（比如一个对象、一些数据、或者IO设备）的权利。
- 为了最小化线程因为获取不到锁而被阻塞的可能，应该最小化地使用加锁解锁，仅在即将要使用时获取锁，并在不使用后的第一时间释放。
- 使用锁保护的一段代码区域称为临界区（critical section）。
- 标准库提供的互斥锁是排他性拥有语义的，也就是只有一个线程能够排他地拥有该锁。实践中除了这种锁，还有许多其他种类的锁，比如允许多个读线程一个写线程的互斥锁，标准库没有提供这种锁。如果要用，可以使用操作系统提供的或者自己写。

`std::mutex`和`std::recursive_mutex`：
- 类型：`native_handle_type`，实现定义的底层句柄类型。`native_handle`接口用于获取该句柄。
- 加锁解锁：
    - `lock`：获取锁，未获取到会阻塞当前线程，获取到则继续执行。
    - `try_lock`：尝试获取锁，是否获取到都会直接返回，未获取到则返回`false`，获取成功则返回`true`。
    - `unlock`：解锁。
- 互斥锁不能被拷贝或者移动，将互斥锁看做一种资源，而不是一种资源的句柄。事实上，`mutex`通常被实现为操作系统资源的句柄，但因为这种操作系统资源不能被分享、拷贝、释放、移动，所以上层的互斥锁也不能这样做。
- `try_lock`一般用于一个线程获取不到资源也有其他事情可做的情况。比如它有一个任务队列，其中一个任务没有获取到锁可以暂停执行然后执行下一个任务。
- 当使用锁时，就需要考虑死锁问题（deadlock）：一个线程在等待一个永远也不会被释放的锁。最简单的死锁场景就是递归获取`std::mutex`，获取已经被当前线程获取到的锁会永远阻塞当前线程，这种情况改用`std::recursive_mutex`就能避免，`std::recursive_mutex`允许一个线程多次获取和释放一个锁，获取多少次就需要释放多少次，第一次获取就会获取到该锁，直到最后一个释放才会真正释放该锁。

`std::timed_mutex`和`std::recrusive_timed_mutex`：
- 除了提供`std::mutex`和`std::recursive_mutex`的所有操作外，还提供两个接口：
    - `try_lock_for(d)`：尝试获取一个锁一段时间，期间获取到返回`true`，到时候后获取不到返回`false`。
    - `try_lock_until(tp)`：尝试获取一个所到一个时间点。
    - 如果上述的`d`小于等于0，或者`tp`在当前时间点之前，那么就相当于朴素的`try_lock`。
- `std::recursive_timed_mutex`允许递归锁定。

互斥锁的RAII类：
- 因为锁可以看做一个资源，锁定之后必须要被释放，所以可以使用RAII来管理，标准库提供了RAII类。
- `std::lock_guard`：最简单、最小、最快的RAII类，构造时获取锁，析构时释放锁，没有任何其他功能。
    - 除了朴素的构造和析构还有一个构造：`lock_guard( mutex_type& m, std::adopt_lock_t t );`，传入第二个参数`adopt_lock`构造，此时会假定当前线程已经获取了锁，则构造时不会获取锁，只会在析构时释放，如果当前线程没有获取到该锁，那么是UB。
- `std::unique_lock`：可以移动的RAII类，构造时获取锁，析构时释放锁，可以通过移动构造和移动赋值移动锁的锁资源的管理权。
    - 可以默认构造，不持有任何锁。
    - `explicit unique_lock( mutex_type& m );`：构造时获取锁。
    - `unique_lock( mutex_type& m, std::defer_lock_t t ) noexcept;`：持有但是不获取锁。
    - `unique_lock( mutex_type& m, std::try_to_lock_t t );`：构造时调用`try_lock`尝试获取锁，获取到则持有，未获取到则不持有。
    - `unique_lock( mutex_type& m, std::adopt_lock_t t );`：持有互斥锁，但是不在构造时获取，假定当前线程已经获取了锁。
    - `unique_lock lck(m, tp)`：传入一个时间点，对互斥锁调用`try_lock_until(tp)`，获取到才持有。
    - `unique_lock lck(m, d)`：传入时间段，对互斥锁调用`try_lock_for(d)`，获取到才持有。
    - 析构：如果持有锁，则释放它。
    - 移动构造、移动赋值、交换操作略。
    - 提供显式的`lock try_lock try_lock_for try_lock_until unlock`操作。
    - `release`在不释放锁的情况下释放其拥有权。
    - `mutex`：获取其中保存的互斥锁的指针。
    - `owns_lock`：测试是否持有一个锁。
    - `operator bool`：测试是否持有一个锁。
    - 只有持有`timed_mutex recursive_timed_mutex`时才允许调用时间相关操作。
- `std::scoped_lock`：C++17引入，只有简单的构造与析构操作，目的是同时持有多个锁，有一个未获取到都释放所有获取到的锁，要么就全获取到，要么就全都不获取。
    - 有一个接受`std::adopt_lock_t`的构造：`scoped_lock( std::adopt_lock_t, MutexTypes&... m );`。
- 应该最小化锁的获取间隔（即临界区），所以引入RAII类之后，可能需要显式加入一个新的块来控制RAII对象的生命周期，而不是让其一直自然持续到大的块结束。
- 任何可锁定对象（定义了`lock unlock`）都可以使用`std::lock_guard`管理，不仅仅是标准库中的几个互斥锁。

相关类型与常量：
- 定义了几个类型在RAII类构造时用来标识持有锁的策略：
    - `std::defer_lock_t`：持有但不获取锁。
    - `std::try_to_lock_t`：调用`try_lock`尝试获取锁，获取成功才持有。
    - `std::adopt_lock_t`：假定锁已经被获取，构造时持有但不获取锁。
- 每个类型都定义了一个常量，直接用这几个常量作为参数传递就行：
    - `std::defer_lock std::try_to_lock std::adopt_lock`。

多个锁：
- 很多时候需要同时获取多个锁，但是可能会由于执行顺序的不确定导致死锁，所以这种情况就需要一种`commit or rollback`操作，要么获取所有锁，要么一个都不获取。避免两个线程互相持有一个对方想获取的锁导致死锁。
- `std::try_lock(locks...)`：尝试获取多个锁，返回-1表示成功，否则返回一个基于0的获取失败的锁的索引。
- `std::lock(locks...)`：获取多个锁，获取不到则会释放已经获取到的，并且阻塞当前线程。`std::scoped_lock`是这个函数的RAII类。
- 注意直接对互斥锁而不是`std::unique_lock`使用`std::lock`会需要程序员显式释放每个锁。
- 可以使用`std::defer_lock`构造`std::unique_lock`，然后使用`std::lock`获取多个锁，然后依赖于`std::unique_lock`析构释放这些锁。

`call_once`：
- `std::once_flag std::call_once`提供了一种避免数据竞争的变量初始化机制，可以用于静态变量的初始化。编译器可能就是使用类似机制初始化局部静态变量的。
- `std::once_flag fl{};`：默认构造，此时`fl`还没有被使用过。
- `std::call_once(fl, f, args)`：如果`fl`还没有被使用过则调用`f(args)`，调用之后`fl`则变为使用过。
- 例子：可以用这个机制实现其他静态变量：比如全局变量、类的静态变量的第一次使用前初始化，就像局部静态变量一样。从而得到确定的初始化控制、并且避免数据竞争（什么时候这种变量的初始化会出现数据竞争？）。
```C++
class X
{
private:
    static std::once_flag static_flag;
    static Y static_data_for_class_X;
    static void init()
    {
        // do initialization of static_data_for_class_X
    }
public:
    X()
    {
        call_once(static_flag, init);
    }
};
```
- 这种初始化其实是先默认初始化一次之后，再在第一次构造`X`时调用初始化函数进行仅一次的初始化。

`<shared_mutex>`：
- C++14和C++17还引入了头文件`<shared_mutex>`，其中包括：
- C++17引入了`std::shared_mutex`：提供两个层次的访问控制：
    - 同普通互斥锁一样的只能单个线程拥有该锁：`lock try_lock unlock`。
    - 在此基础之上提供多个线程可以共同拥有该锁的机制：`lock_shard try_lock_shared unlock_shared`。
- C++14引入的`std::shared_timed_mutex`在`std::shared_mutex`基础上提供：`try_lock_for try_lock_until try_lock_shared_for try_lock_shard_until`。
- C++14引入的`std::shared_lock`作为上面的共享互斥锁的RAII类，提供和`std::unique_lock`完全相同的接口与功能，只可移动不可拷贝。

## 条件变量

条件变量：
- 条件变量被用来管理线程之间的通信。
- 一个线程可以（阻塞起来）在一个条件变量上等待某一事件的到来，比如到达某一特定时间或者某个其他线程完成等，使用条件变量可以减少数据竞争。
- 条件变量的工作模式：
    - 等待通知方：先获取到用来保护共享变量的锁，调用等待接口等待事件到来（在这之前最好先检查事件是否已经到来如果到来就可以不用等待了），等待期间会自动（由等待接口来）阻塞并释放锁拥有权，知道事件完成后（或者等待超时，或者假醒）自动重新获取锁，检查条件是否得到满足。
    - 通知方：获取锁，拥有锁的期间修改共享变量，调用`notify_one notify_all`通知所有在等待这个条件变量的线程。

首先状态类型`std::cv_status`：
```C++
enum class cv_status {
    no_timeout,
    timeout  
};
```
- 作为条件变量按时间等待的返回值，`no_timeout`表示按时间等待等到了通知，`timeout`表示超时了没有等到通知。

`std::condition_variable`：
- 首先：`std::condition_variable`仅与`std::unique_lock`配合使用。
- 下列的`lock`只能是`std::unique_lock<std::mutex>`。
- 默认构造、析构比较简单，略，不可拷贝。
- 同样有`native_handle_type`类型和`native_handle`接口。
- 通知接口：
    - `nofity_one`：通知其中一个等待的线程（如果有）。
    - `notify_all`：通知所有正在等待的线程。
- 等待接口：
    - `wait(lock)`：自动释放`lock`，阻塞当前线程，添加当前线程到等待`*this`的列表中。当`notify_one notify_all`执行时当前线程会被唤醒，也可能被虚假地唤醒（也就是没有接到通知但是被唤醒了）。无论任何原因，唤醒之后都会重新获取锁`lock`。
    - `wait(lock, pred)`：等价于：
        ```C++
        while (!pred())
        {
            wait(lock);
        }
        ```
        - 也即是会确保谓词满足，不然就一直等待，可以过滤掉假醒（spurious awaken）的情况。
        - 注意这个锁在进入之前必须是锁住的，然后进入`wait`之后才会被锁住，也就是说这个锁可以在`pred`中用于保护相关共享变量的访问。
    - 如果传入的`lock`不拥有互斥锁或者没有当前线程获取，那么会违反前置条件，会调用`std::terminate`终止程序。
    - `x = cv.wait_until(lock, tp)`同理，不过只等待到某一时间点，如果超时返回`std::cv_status::timeout`，没有超时等到了事件则返回`std::no_timeout`。
    - `b = cv.wait_until(lock, tp, pred)`返回布尔值，等价于：
    ```C++
    while (!pred())
    {
        if (wait_until(lock, tp) == std::cv_status::timeout)
        {
            return pred();
        }
    }
    return true;
    ```
    - `x = cv.wait_for(lock, d)`：等价于`x = cv.wait_until(lock, std::steady_lock::now() + d)`
    - `x = cv.wait_for(lock, d, pred)`：等价于`x = cv.wait_until(lock, std::steady_lock::now() + d, std::move(pred))`
- 一个条件变量可能依赖也有可能不依赖操作系统资源，如果依赖且已经没有这种资源，那么构造会抛出异常。
- 就像`mutex`一样，`condition_variable`不能被复制或者拷贝，所以最好将其视为一种资源，而不是一种资源句柄。
- `condition_variable`析构时需要确保没有线程等待在该条件变量上，否则他们将永远等待下去。
- `wait`是一个底层操作，因为实现原因，标准允许该接口出现假醒以简化实现复杂度，也就是在没有得到通知的情况下返回。要使用朴素的`wait`的话应该在循环中使用，比如：
```C++
while (queue.empty())
{
    wait(queue_lck);
}
```
- 带谓词版本就是做这个事情，所以最好优先选择带谓词版本。

实践细节：
- `notify_one`还是`notify_all`的选择，取决于应用程序。比如在一个生产者消费者模型中，如果已有未消费数据太多，则可以使用`notify_all`加大吞吐量，如果数据队列中数据保有量比较小，则可以选择`notify_one`避免频繁唤醒多个线程去消费一个接近空的数据队列。具体选什么需要根据场景来，如果要确定一个参数作为`notify_one notify_all`的划分依据，最好进行相关性能测试找一个最优值，而不是凭直觉。
- 实践中（比如生产者消费者模型）通常会有在未等到条件变量的情况下也可以做一些其他事情，而不是单纯阻塞线程的情况。这时候可以使用等待一定时间的接口（带谓词版本），并在具体接口中返回`bool`表示是否等到了条件变量/是否取得数据等，根据此结果决定下一步是处理数据还是做其他事情。要这样做就要能够重新等待条件变量，通常要在一个循环中才有意义。

`std::condition_variable_any`：
- 相对于只能在`std::unique_lock<std::mutex>`上工作的`std::condition_variable`（对其有优化，所以能用`std::condition_variable`就用`std::condition_variable`）。`std::condition_variable_any`可以在任何基本可锁定对象上工作（也就是具有`lock unlock`接口的类型）。
- 接口基本完全一致，一个用法。

## 任务
