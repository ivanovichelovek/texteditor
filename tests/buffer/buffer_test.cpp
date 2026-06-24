#include "../../src/buffer/buffer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
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

TEST(gap_buffer_smoke_test, Assignment) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap_buffer<AllocatorWithCount<int>> gap2;
  gap2 = gap1;
  std::ignore = gap2;
}

TEST(gap_buffer_smoke_test, Clear) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer<AllocatorWithCount<int>> gap1(alloc);
  gap1.clear();
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

// Work tests

TEST(gap_buffer_other, Equality) {
  gap_buffer gap1("abcd");
  gap_buffer gap2("abcd");
  EXPECT_EQ(gap1, gap2);
  gap1.step(-1);
  EXPECT_NE(gap1, gap2);
  gap2.step(-1);
  EXPECT_EQ(gap1, gap2);

  gap_buffer gapcp;
  gapcp = gap1;
  EXPECT_EQ(gap1, gapcp);

  gap_buffer<AllocatorWithCount<char>> gap3(6, 'b');
  gap_buffer<AllocatorWithCount<char>> gap3_cp(gap3);
  EXPECT_EQ(gap3, gap3_cp);
}

TEST(gap_buffer_other, SizeAndCapacity) {
  SetupTest();
  AllocatorWithCount<int> alloc;
  gap_buffer gap;
  EXPECT_TRUE(gap.empty());
  gap_buffer<AllocatorWithCount<int>> gap1(25, alloc);
  gap_buffer gap2(5, alloc);
  EXPECT_EQ(gap1.size(), 25);
  EXPECT_EQ(gap1.capacity(), 25);
  EXPECT_EQ(gap2.size(), 5);
  EXPECT_EQ(gap2.capacity(), 16);
  gap1.insert('a');
  EXPECT_EQ(gap1.size(), 26);
  EXPECT_EQ(gap1.capacity(), 50);
  gap1.erase_in_front_of_cursor();
  EXPECT_EQ(gap1.size(), 25);
  EXPECT_EQ(gap1.capacity(), 50);
  for (int cnt = 0; cnt < 15; ++cnt) {
    gap1.erase_in_front_of_cursor();
  }
  EXPECT_EQ(gap1.size(), 10);
  EXPECT_EQ(gap1.capacity(), 25);
}

TEST(gap_buffer_other, Swap) {
  gap_buffer gap1(5, 'a');
  gap_buffer gap1_cp = gap1;
  gap1_cp.insert('b');
  EXPECT_EQ(gap1.to_string() + 'b', gap1_cp.to_string());
  gap1_cp.erase_in_front_of_cursor();
  EXPECT_EQ(gap1, gap1_cp);
  gap_buffer gap3(5, 'b');
  gap_buffer gap3_cp = gap3;
  gap1.swap(gap3);
  EXPECT_EQ(gap3, gap1_cp);
  EXPECT_EQ(gap1, gap3_cp);
}

TEST(gap_buffer_other, ToString) {
  gap_buffer gap1(5, 'a');
  gap_buffer<AllocatorWithCount<char>> gap2(6, 'b');
  EXPECT_EQ(gap1.to_string(), std::string(5, 'a'));
  EXPECT_EQ(gap2.to_string(), std::string(6, 'b'));
  gap1.insert('b');
  gap2.erase_in_front_of_cursor();
  EXPECT_EQ(gap1.to_string(), std::string(5, 'a') + 'b');
  EXPECT_EQ(gap2.to_string(), std::string(5, 'b'));

  gap_buffer gap("asddsa");
  gap.step(-3);
  EXPECT_EQ(gap.to_string(), "asddsa");
}

TEST(gap_buffer_other, Clear) {
  gap_buffer gap(14, 'a');
  EXPECT_EQ(gap.size(), 14);
  EXPECT_EQ(gap.to_string(), std::string(14, 'a'));
  gap.clear();
  EXPECT_EQ(gap.size(), 0);
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap.to_string(), "");

  gap_buffer gap1(18, 'a');
  EXPECT_EQ(gap1.size(), 18);
  EXPECT_EQ(gap1.to_string(), std::string(18, 'a'));
  gap1.clear();
  EXPECT_EQ(gap1.size(), 0);
  EXPECT_EQ(gap1.capacity(), 16);
  EXPECT_EQ(gap1.to_string(), "");
}

TEST(gap_buffer_other, ResizeAndReserve) {
  gap_buffer gap;
  gap.reserve(15);
  EXPECT_EQ(gap.capacity(), 16);
  gap.reserve(32);
  EXPECT_EQ(gap.capacity(), 32);
  EXPECT_EQ(gap.size(), 0);
  gap.resize(15, 'a');
  EXPECT_EQ(gap.size(), 15);
  EXPECT_EQ(gap.capacity(), 32);
  gap.shrink_to_fit();
  EXPECT_EQ(gap.size(), 15);
  EXPECT_EQ(gap.capacity(), 16);
  gap.resize(18, 'a');
  gap.reserve(40);
  EXPECT_EQ(gap.size(), 18);
  EXPECT_EQ(gap.capacity(), 40);
  gap.shrink_to_fit();
  EXPECT_EQ(gap.capacity(), 18);
  gap.resize(10, 'a');
  EXPECT_EQ(gap.size(), 18);
  EXPECT_EQ(gap.capacity(), 18);
  gap.resize(35, 'a');
  EXPECT_EQ(gap.size(), 35);
  EXPECT_EQ(gap.capacity(), 35);
  EXPECT_EQ(gap.to_string(), std::string(35, 'a'));
}

TEST(gap_buffer_other, Step) {
  gap_buffer gap1(5, 'a');
  gap1.insert('b');
  gap1.step(-1);
  gap1.erase_in_front_of_cursor();
  EXPECT_EQ(gap1.to_string(), std::string(4, 'a') + 'b');
  gap1.insert('c');
  EXPECT_EQ(gap1.to_string(), std::string(4, 'a') + "cb");
  gap1.step(1);
  gap1.insert('c');
  EXPECT_EQ(gap1.to_string(), std::string(4, 'a') + "cbc");
  gap1.step(100);
  EXPECT_EQ(gap1.GetCursorPositionIndex(), gap1.size());
  gap1.step(-100);
  EXPECT_EQ(gap1.GetCursorPositionIndex(), 0);
}

TEST(gap_buffer_other, InsertAndErase) {
  gap_buffer gap;
  gap.insert('a');
  gap.insert('b');
  gap.insert('c');
  gap.step(-3);
  gap.erase_in_front_of_cursor();
  EXPECT_EQ(gap.to_string(), "abc");
  gap.step(3);
  EXPECT_EQ(gap.to_string(), "abc");
  gap.step(-1);
  gap.insert('d');
  EXPECT_EQ(gap.to_string(), "abdc");
  gap.step(1);
  gap.erase_in_back_of_cursor();
  EXPECT_EQ(gap.to_string(), "abdc");
  gap.step(-1);
  gap.erase_in_back_of_cursor();
  EXPECT_EQ(gap.to_string(), "abd");
  gap.erase_in_back_of_cursor();
  EXPECT_EQ(gap.to_string(), "abd");
  gap.erase_in_front_of_cursor();
  EXPECT_EQ(gap.to_string(), "ab");
}

TEST(gap_buffer_other, OperatorSqBrackets) {
  gap_buffer gap("abcd");
  gap.step(-2);
  EXPECT_EQ(gap[0], 'a');
  EXPECT_EQ(gap[1], 'b');
  EXPECT_EQ(gap[2], 'c');
  EXPECT_EQ(gap[3], 'd');
  EXPECT_EQ(gap.at(0), 'a');
  EXPECT_EQ(gap.at(1), 'b');
  EXPECT_EQ(gap.at(2), 'c');
  EXPECT_EQ(gap.at(3), 'd');
  EXPECT_THROW(gap.at(4), std::out_of_range);
  const gap_buffer gap1(gap);
  EXPECT_EQ(gap1[0], 'a');
  EXPECT_EQ(gap1[1], 'b');
  EXPECT_EQ(gap1[2], 'c');
  EXPECT_EQ(gap1[3], 'd');
  EXPECT_EQ(gap1.at(0), 'a');
  EXPECT_EQ(gap1.at(1), 'b');
  EXPECT_EQ(gap1.at(2), 'c');
  EXPECT_EQ(gap1.at(3), 'd');
  EXPECT_THROW(gap1.at(4), std::out_of_range);
}

TEST(gap_buffer_other, BeginEnd) {
  gap_buffer gap("abcd");
  const gap_buffer cgap("abcd");
  EXPECT_EQ(gap.end() - gap.begin(), 4);
  EXPECT_EQ(gap.cend() - gap.cbegin(), 4);
  EXPECT_EQ(cgap.end() - cgap.begin(), 4);

  EXPECT_EQ(gap.rend() - gap.rbegin(), 4);
  EXPECT_EQ(gap.crend() - gap.crbegin(), 4);
  EXPECT_EQ(cgap.rend() - cgap.rbegin(), 4);

  gap.step(-2);

  for (std::size_t index = 0; index < gap.size(); ++index) {
    EXPECT_EQ(*(gap.begin() + index), "abcd"[index]);
  }

  for (std::size_t index = 0; index < gap.size(); ++index) {
    EXPECT_EQ(*(gap.rbegin() + index), "dcba"[index]);
  }
}

// Tests for iterator to gap_buffer
