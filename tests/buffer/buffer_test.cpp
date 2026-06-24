#include "../../src/buffer/buffer.hpp"

#include <gtest/gtest.h>

#include <tuple>

#include "../memory_utils.hpp"
#include "../utils.hpp"

size_t MemoryManager::type_new_allocated = 0;
size_t MemoryManager::type_new_deleted = 0;
size_t MemoryManager::allocator_allocated = 0;
size_t MemoryManager::allocator_deallocated = 0;
size_t MemoryManager::allocator_constructed = 0;
size_t MemoryManager::allocator_destroyed = 0;

template <typename T, bool PropagateOnConstruct, bool PropagateOnAssign>
size_t WhimsicalAllocator<T, PropagateOnConstruct, PropagateOnAssign>::counter =
    0;

size_t Accountant::ctor_calls = 0;
size_t Accountant::dtor_calls = 0;

bool ThrowingAccountant::need_throw = false;

void SetupTest() {
  MemoryManager::type_new_allocated = 0;
  MemoryManager::type_new_deleted = 0;
  MemoryManager::allocator_allocated = 0;
  MemoryManager::allocator_deallocated = 0;
  MemoryManager::allocator_constructed = 0;
  MemoryManager::allocator_destroyed = 0;
}

// Tests for gap_buffer

// Smoke tests

TEST(gap_buffer_smoke_test, DefaultConstructor) {
  gap_buffer gap;
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, DefaultConstructorWithAllocator) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap(alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumber) {
  gap_buffer gap(25);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumberWithAllocator) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap(25, alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumberAndLetter) {
  gap_buffer gap(25, 'a');
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumberAndLetterWithAllocator) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap(25, 'a', alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromString) {
  std::string str{"abacaba"};
  gap_buffer gap1(str);
  gap_buffer gap2(static_cast<std::string>("abacaba"));
  std::ignore = gap1;
  std::ignore = gap2;
}

TEST(gap_buffer_smoke_test, FromStringWithAllocator) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  std::string str{"abacaba"};
  gap_buffer<AllocatorWithCount<int>> gap1(str, alloc);
  gap_buffer<AllocatorWithCount<int>> gap2(static_cast<std::string>("abacaba"),
                                           alloc);
  std::ignore = gap1;
  std::ignore = gap2;
}

TEST(gap_buffer_smoke_test, FromCString) {
  gap_buffer gap("abacaba");
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromCStringWithAllocator) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap("abacaba", alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromGapBuffer) {
  gap_buffer gap1;
  gap_buffer gap2(gap1);
  gap_buffer gap3(std::move(gap1));
  std::ignore = gap2;
  std::ignore = gap3;
}

TEST(gap_buffer_smoke_test, FromGapBufferWithAllocator) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap_buffer<AllocatorWithCount<int>> gap2(gap1, alloc);
  gap_buffer<AllocatorWithCount<int>> gap3(std::move(gap1), alloc);
  std::ignore = gap2;
  std::ignore = gap3;
}

TEST(gap_buffer_smoke_test, Swap) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap_buffer<AllocatorWithCount<int>> gap2(alloc);
  gap1.swap(gap2);
}

TEST(gap_buffer_smoke_test, Asigment) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap_buffer<AllocatorWithCount<int>> gap2 = gap1;
  std::ignore = gap2;
}

TEST(gap_buffer_smoke_test, Clear) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.clear();
}

TEST(gap_buffer_smoke_test, ResizeNumber) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.resize(125);
}

TEST(gap_buffer_smoke_test, ResizeNumberAndLetter) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.resize(125, 'a');
}

TEST(gap_buffer_smoke_test, Reserve) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.reserve(125);
}

TEST(gap_buffer_smoke_test, ShrinkToFit) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.shrink_to_fit();
}

TEST(gap_buffer_smoke_test, Size) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.size();
}

TEST(gap_buffer_smoke_test, Capacity) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.capacity();
}

TEST(gap_buffer_smoke_test, Empty) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.empty();
}

TEST(gap_buffer_smoke_test, Begin) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.begin();
}

TEST(gap_buffer_smoke_test, ConstBegin) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.begin();
}

TEST(gap_buffer_smoke_test, CBegin) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.cbegin();
}

TEST(gap_buffer_smoke_test, End) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.end();
}

TEST(gap_buffer_smoke_test, ConstEnd) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.end();
}

TEST(gap_buffer_smoke_test, CEnd) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.cend();
}

TEST(gap_buffer_smoke_test, RBegin) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.rbegin();
}

TEST(gap_buffer_smoke_test, ConstRBegin) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.rbegin();
}

TEST(gap_buffer_smoke_test, CRBegin) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.crbegin();
}

TEST(gap_buffer_smoke_test, REnd) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.rend();
}

TEST(gap_buffer_smoke_test, ConstREnd) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.rend();
}

TEST(gap_buffer_smoke_test, CREnd) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.crend();
}

TEST(gap_buffer_smoke_test, ToString) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  const gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  std::ignore = gap1.to_string();
}

TEST(gap_buffer_smoke_test, Insert) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.insert('a');
}

TEST(gap_buffer_smoke_test, EraseInFrontOfCursor) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.erase_in_front_of_cursor();
}

TEST(gap_buffer_smoke_test, EraseInBackOfCursor) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.erase_in_back_of_cursor();
}

TEST(gap_buffer_smoke_test, Step) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(15, alloc);
  gap1.step(5);
  gap1.step(-5);
}

TEST(gap_buffer_smoke_test, OperatorSqBrackets) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(15, alloc);
  const gap_buffer<AllocatorWithCount<int>> gap2(15, alloc);
  std::ignore = gap1[0];
  std::ignore = gap2[14];
}

TEST(gap_buffer_smoke_test, At) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(15, alloc);
  const gap_buffer<AllocatorWithCount<int>> gap2(15, alloc);
  std::ignore = gap1.at(0);
  std::ignore = gap2.at(14);
}

// Tests for iterator to gap_buffer
