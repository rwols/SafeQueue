#include <rwols/SafeQueue.hpp>

#include <gmock/gmock.h>

#include <iostream>
#include <memory>
#include <thread>

#define sqPRINT std::cerr << "[++++++++++] "

using namespace rwols;

TEST(SafeQueue, ConstructAndDestruct) { SafeQueue<int> q; }

TEST(SafeQueue, Push) {
  SafeQueue<int> q;
  q.Push(42);
  q.TaskDone();
}

TEST(SafeQueue, Pop) {
  SafeQueue<int> q;
  q.Push(42);
  auto x = q.Pop();
  EXPECT_EQ(x, 42);
  q.TaskDone();
}

TEST(SafeQueue, TwoThreads) {
  SafeQueue<int> q;
  auto dowork = [&]() {
    sqPRINT << "Waiting for an item...\n";
    auto item = q.Pop();
    sqPRINT << "Got item.\n";
    EXPECT_EQ(item, 42);
    // simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sqPRINT << "Invoking TaskDone()\n";
    q.TaskDone();
  };
  std::thread worker(dowork);
  sqPRINT << "Inserting item...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  q.Push(42);
  worker.join();
}

TEST(SafeQueue, MultiPush) {
  SafeQueue<int> q;
  auto dowork = [&]() {
    int count = 0;
    while (true) {
      auto item = q.Pop();
      EXPECT_EQ(item, count);
      ++count;
      q.TaskDone();
      if (count == 5)
        break;
    }
  };
  std::thread worker(dowork);
  for (int i = 0; i < 5; ++i)
    q.Push(i);
  worker.join();
}

TEST(SafeQueue, TaskDoneGuard) {
  SafeQueue<int> q;
  auto dowork = [&]() {
    int count = 0;
    while (true) {
      auto pair = q.PopWithGuard();
      EXPECT_EQ(pair.first, count);
      ++count;
      if (count == 5)
        break;
    }
  };
  std::thread worker(dowork);
  for (int i = 0; i < 5; ++i)
    q.Push(i);
  worker.join();
}

TEST(SafeQueue, MoveOnlyType) {
  struct Foo {
    Foo() = default;
    Foo(const Foo &) = delete;
    Foo &operator=(const Foo &) = delete;
    Foo(Foo &&other) : data(other.data) { other.data = nullptr; }
    Foo &operator=(Foo &&other) {
      data = other.data;
      other.data = nullptr;
      return *this;
    }
    void *data;
  };
  Foo foo;
  foo.data = &foo;
  SafeQueue<Foo> q;
  q.Push(std::move(foo));
  auto pair = q.PopWithGuard();
  EXPECT_EQ(pair.first.data, &foo);
  EXPECT_FALSE(foo.data);
}

TEST(SafeQueue, UniquePtr) {
  SafeQueue<std::unique_ptr<int>> q;
  q.Push(std::make_unique<int>(42));
  auto x = q.PopWithGuard();
  ASSERT_TRUE(x.first.get());
  EXPECT_EQ(*x.first, 42);
}

TEST(SafeQueue, SharedPtr) {
  SafeQueue<std::shared_ptr<int>> q;
  q.Push(std::make_shared<int>(42));
  auto x = q.PopWithGuard();
  ASSERT_TRUE(x.first.get());
  EXPECT_EQ(*x.first, 42);
}

TEST(SafeQueue, PushAndJoin) {
  SafeQueue<int> q;
  auto dowork = [&]() {
    sqPRINT << "Waiting for an item...\n";
    auto item = q.Pop();
    sqPRINT << "Got item.\n";
    EXPECT_EQ(item, 42);
    // simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sqPRINT << "Invoking TaskDone()\n";
    q.TaskDone();
  };
  std::thread worker(dowork);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  q.PushAndJoin(42);
  worker.join();
}

TEST(SafeQueue, Stress) {
  std::mutex mut;
  std::condition_variable trafficLight;
  struct BigThing {
    char data[2048];
    BigThing() { std::fill(std::begin(data), std::end(data), 0); };
  };
  SafeQueue<BigThing> q;
  bool green = false;
  constexpr int numItems = 4000;
  constexpr int numProducers = 1;
  constexpr int numConsumers = 4;
  auto waitForGreenLight = [&]() {
    std::unique_lock<std::mutex> theSignal(mut);
    trafficLight.wait(theSignal, [&]() { return green; });
  };
  auto popFromQueue = [&]() {
    waitForGreenLight();
    while (true) {
      try {
        auto pair = q.PopWithGuard(std::chrono::milliseconds(100));
        // simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      } catch (const TimeoutError &error) {
        std::lock_guard<std::mutex> printerLock(mut);
        sqPRINT << std::this_thread::get_id() << " timeout!\n";
        break;
      }
    }
  };
  auto pushToQueue = [&]() {
    waitForGreenLight();
    for (int i = 0; i < numItems; ++i) {
      q.Push(BigThing{});
    }
  };
  std::thread producers[numProducers];
  std::thread consumers[numConsumers];
  for (auto &consumer : consumers)
    consumer = std::thread(popFromQueue);
  for (auto &producer : producers)
    producer = std::thread(pushToQueue);
  auto startTime = std::chrono::high_resolution_clock::now();
  green = true;
  trafficLight.notify_all();
  for (auto &consumer : consumers)
    consumer.join();
  auto duration = std::chrono::high_resolution_clock::now() - startTime;
  sqPRINT
      << "This took "
      << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
      << " milliseconds\n";
  for (auto &producer : producers)
    producer.join();
}
