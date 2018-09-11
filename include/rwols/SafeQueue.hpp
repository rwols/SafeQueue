///\file    SafeQueue.hpp
///\brief   Thread-safe queue
///\author  Raoul Wols
///\date    September, 2018

#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace rwols {

class TimeoutError final : public std::exception {
  const char *what() const noexcept override { return "timeout"; }
};

template <class T, class Container = std::deque<T>> class SafeQueue final {
public:
  using container_type = Container;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;

  struct TaskDoneGuard {
    TaskDoneGuard(const TaskDoneGuard &) = delete;
    TaskDoneGuard(TaskDoneGuard &&);
    TaskDoneGuard &operator=(const TaskDoneGuard &) = delete;
    TaskDoneGuard &operator=(TaskDoneGuard &&);
    ~TaskDoneGuard() noexcept(false);

  private:
    SafeQueue *mQ = nullptr;
    TaskDoneGuard(SafeQueue *);
    friend class SafeQueue;
  };

  ~SafeQueue();

  void Push(const_reference item);
  void Push(value_type &&item);
  void PushAndJoin(const_reference item);
  void PushAndJoin(value_type &&item);

  template <class... Args> void Emplace(Args &&... args);
  template <class... Args> void EmplaceAndJoin(Args &&... args);

  value_type Pop();
  std::pair<value_type, TaskDoneGuard> PopWithGuard();
  template <class Rep, class Period>
  value_type Pop(const std::chrono::duration<Rep, Period> &timeout);
  template <class Rep, class Period>
  std::pair<value_type, TaskDoneGuard>
  PopWithGuard(const std::chrono::duration<Rep, Period> &timeout);

  void TaskDone();

  void Join();

private:
  std::mutex mMutex;
  std::condition_variable mNotEmpty, mAllTasksDone;
  std::queue<value_type, container_type> mQ;
  std::size_t mUnfinishedTasks = 0;

  using UniqueLock = std::unique_lock<std::mutex>;
  using LockGuard = std::lock_guard<std::mutex>;
};

// Implementation follows.

template <class T, class C>
SafeQueue<T, C>::TaskDoneGuard::TaskDoneGuard(TaskDoneGuard &&other)
    : mQ(other.mQ) {
  other.mQ = nullptr;
}

template <class T, class C>
typename SafeQueue<T, C>::TaskDoneGuard &SafeQueue<T, C>::TaskDoneGuard::
operator=(TaskDoneGuard &&other) {
  mQ = other.mQ;
  other.mQ = nullptr;
  return *this;
}

template <class T, class C>
SafeQueue<T, C>::TaskDoneGuard::TaskDoneGuard(SafeQueue *q) : mQ(q) {}

template <class T, class C>
SafeQueue<T, C>::TaskDoneGuard::~TaskDoneGuard() noexcept(false) {
  if (mQ)
    mQ->TaskDone();
}

template <class T, class C> SafeQueue<T, C>::~SafeQueue() {
  Join();
  assert(mUnfinishedTasks == 0 && "Expected all tasks to be finished");
}

template <class T, class C> void SafeQueue<T, C>::Push(const_reference item) {
  {
    LockGuard lock(mMutex);
    mQ.push(item);
    ++mUnfinishedTasks;
  }
  mNotEmpty.notify_one();
}

template <class T, class C> void SafeQueue<T, C>::Push(value_type &&item) {
  {
    LockGuard lock(mMutex);
    mQ.push(std::move(item));
    ++mUnfinishedTasks;
  }
  mNotEmpty.notify_one();
}

template <class T, class C>
void SafeQueue<T, C>::PushAndJoin(const_reference item) {
  UniqueLock lock(mMutex);
  mQ.push(item);
  ++mUnfinishedTasks;
  mNotEmpty.notify_one();
  mAllTasksDone.wait(lock, [this]() { return mUnfinishedTasks == 0; });
}

template <class T, class C>
void SafeQueue<T, C>::PushAndJoin(value_type &&item) {
  UniqueLock lock(mMutex);
  mQ.push(std::move(item));
  ++mUnfinishedTasks;
  mNotEmpty.notify_one();
  mAllTasksDone.wait(lock, [this]() { return mUnfinishedTasks == 0; });
}

template <class T, class C>
template <class... Args>
void SafeQueue<T, C>::Emplace(Args &&... args) {
  {
    LockGuard lock(mMutex);
    mQ.emplace(std::forward<Args>(args)...);
    ++mUnfinishedTasks;
  }
  mNotEmpty.notify_one();
}

template <class T, class C>
template <class... Args>
void SafeQueue<T, C>::EmplaceAndJoin(Args &&... args) {
  UniqueLock lock(mMutex);
  mQ.emplace(std::forward<Args>(args)...);
  ++mUnfinishedTasks;
  mNotEmpty.notify_one();
  mAllTasksDone.wait(lock, [this]() { return mUnfinishedTasks == 0; });
}

template <class T, class C>
typename SafeQueue<T, C>::value_type SafeQueue<T, C>::Pop() {
  UniqueLock lock(mMutex);
  mNotEmpty.wait(lock, [this]() { return !mQ.empty(); });
  auto item = std::move(mQ.front());
  mQ.pop();
  lock.unlock();
  return item;
}

template <class T, class C>
std::pair<typename SafeQueue<T, C>::value_type,
          typename SafeQueue<T, C>::TaskDoneGuard>
SafeQueue<T, C>::PopWithGuard() {
  return std::make_pair(Pop(), TaskDoneGuard(this));
}

template <class T, class C>
template <class Rep, class Period>
typename SafeQueue<T, C>::value_type
SafeQueue<T, C>::Pop(const std::chrono::duration<Rep, Period> &timeout) {
  UniqueLock lock(mMutex);
  if (mNotEmpty.wait_for(lock, timeout, [this]() { return !mQ.empty(); })) {
    auto item = std::move(mQ.front());
    mQ.pop();
    lock.unlock();
    return item;
  }
  throw TimeoutError();
}

template <class T, class C>
template <class Rep, class Period>
std::pair<typename SafeQueue<T, C>::value_type,
          typename SafeQueue<T, C>::TaskDoneGuard>
SafeQueue<T, C>::PopWithGuard(
    const std::chrono::duration<Rep, Period> &timeout) {
  return std::make_pair(Pop(timeout), TaskDoneGuard(this));
}

template <class T, class C> void SafeQueue<T, C>::TaskDone() {
  LockGuard lock(mMutex);
  assert(mUnfinishedTasks > 0 && "TaskDone() called too many times");
  --mUnfinishedTasks;
  if (mUnfinishedTasks == 0)
    mAllTasksDone.notify_all();
}

template <class T, class C> void SafeQueue<T, C>::Join() {
  UniqueLock lock(mMutex);
  mAllTasksDone.wait(lock, [this]() { return mUnfinishedTasks == 0; });
}

} // namespace rwols
