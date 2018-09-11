# SafeQueue

A naive implementation of a thread-safe queue for transporting data across
threads.

# Cloning

Clone recursively if you want to run the unit tests.

# Installation

Just run cmake and run `cmake --build . --target install`. It will install
this header-only library.

# Usage

After installation your CMakeLists.txt should contain something like this:
```
# your CMakeLists.txt:

find_package(SafeQueue REQUIRED CONFIG)
add_executable(Foo main.cpp)
target_link_libraries(Foo rwols::SafeQueue)
```

Your code should contain something like this:
```
#include <rwols/SafeQueue.hpp>

int main() {
    rwols::SafeQueue<int> q;
    q.Push(42);
    // Will block until there's an item available. You typically use this from
    // another thread. Items are *moved*, so move-only types like
    // std::unique_ptr can be used as item types.
    auto x = q.PopWithGuard();
    return 0;
}
```

The `SafeQueue` will refuse to destroy itself until all "tasks" are done. This
`SafeQueue` keeps track of the number of active "tasks".
Every `.Push(...)` will increment the task count by one. You will have to
manually invoke `.TaskDone()` to decrement it if you use `.Pop()` in a consumer
thread. Alternatively, you can use `.PopWithGuard()` in a consumer thread. This
will return an `std::pair<T, TaskDoneGuard>`. If the `TaskDoneGuard` goes out
of scope, it will call `.TaskDone()` on the queue automatically. However, in
some cases you might want to call `.TaskDone()` from the producer thread
instead. The task idea was inspired by Python's Queue implementation.
