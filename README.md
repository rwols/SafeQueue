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
    auto x = q.Pop();
    return 0;
}
```
