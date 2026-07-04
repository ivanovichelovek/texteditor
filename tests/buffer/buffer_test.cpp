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
  gap_buffer<AllocatorWithCount<int>> gap2(gap1);
  gap_buffer<AllocatorWithCount<int>> gap3(std::move(gap1));
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

// Work tests (gap_buffer)

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
  gap_buffer<AllocatorWithCount<char>> gap3_cp1(gap3);
  gap_buffer<AllocatorWithCount<char>> gap3_cp2(std::move(gap3));
  EXPECT_EQ(gap3_cp2, gap3_cp1);
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
  EXPECT_EQ(gap.to_string(), std::string(15, 'a'));
  gap.shrink_to_fit();
  EXPECT_EQ(gap.size(), 15);
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap.to_string(), std::string(15, 'a'));
  gap.resize(18, 'a');
  gap.reserve(40);
  EXPECT_EQ(gap.size(), 18);
  EXPECT_EQ(gap.capacity(), 40);
  gap.shrink_to_fit();
  EXPECT_EQ(gap.capacity(), 18);
  gap.resize(10, 'a');
  EXPECT_EQ(gap.size(), 18);
  EXPECT_EQ(gap.capacity(), 18);
  EXPECT_EQ(gap.to_string(), std::string(18, 'a'));
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
  EXPECT_EQ(gap1.get_cursor_position_index(), gap1.size());
  gap1.step(-100);
  EXPECT_EQ(gap1.get_cursor_position_index(), 0);
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
  EXPECT_THROW((void)gap.at(4), std::out_of_range);
  const gap_buffer gap1(gap);
  EXPECT_EQ(gap1[0], 'a');
  EXPECT_EQ(gap1[1], 'b');
  EXPECT_EQ(gap1[2], 'c');
  EXPECT_EQ(gap1[3], 'd');
  EXPECT_EQ(gap1.at(0), 'a');
  EXPECT_EQ(gap1.at(1), 'b');
  EXPECT_EQ(gap1.at(2), 'c');
  EXPECT_EQ(gap1.at(3), 'd');
  EXPECT_THROW((void)gap1.at(4), std::out_of_range);
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

// Smoke tests for iterator

TEST(gap_buffer_iterator_smoke, DefaultConstructor) {
  gap_buffer<>::iterator iter;
  std::ignore = iter;
}

TEST(gap_buffer_iterator_smoke, FromPointerAndSize) {
  gap_buffer<>::iterator iter(nullptr, 0, 0);
  std::ignore = iter;
}

TEST(gap_buffer_iterator_smoke, FromIterator) {
  gap_buffer<>::iterator iter1;
  gap_buffer<>::iterator iter(iter1);
  gap_buffer<>::iterator iter2(std::move(iter1));
  std::ignore = iter;
  std::ignore = iter2;
}

TEST(gap_buffer_iterator_smoke, AssignmentOperator) {
  gap_buffer<>::iterator iter1;
  gap_buffer<>::iterator iter2;
  iter2 = iter1;
  std::ignore = iter2;
}

TEST(gap_buffer_iterator_smoke, Swap) {
  gap_buffer<>::iterator iter1;
  gap_buffer<>::iterator iter2;
  std::swap(iter1, iter2);
}

TEST(gap_buffer_iterator_smoke, ConversionToConst) {
  gap_buffer<>::iterator iter;
  gap_buffer<>::const_iterator citer = iter;
  std::ignore = citer;
}

TEST(gap_buffer_iterator_smoke, ArithmeticOperations) {
  gap_buffer<> gap("abcd");
  auto b = gap.begin();
  auto e = gap.end();
  std::ignore = b++;
  ++b;
  std::ignore = b--;
  --b;
  b += 1;
  std::ignore = b + 1;
  std::ignore = b - 1;
  b -= 1;
  std::ignore = e - b;
}

TEST(gap_buffer_iterator_smoke, OperatorSqBrackets) {
  gap_buffer<> gap("abcd");
  auto b = gap.begin();
  std::ignore = b[0];
  std::ignore = b[3];
}

TEST(gap_buffer_iterator_smoke, Comparison) {
  gap_buffer<> gap("abcd");
  auto b = gap.begin();
  auto e = gap.end();
  std::ignore = b < e;
  std::ignore = b > e;
  std::ignore = b <= e;
  std::ignore = b >= e;
  std::ignore = b == e;
  std::ignore = b != e;
}

TEST(gap_buffer_iterator_smoke, Other) {
  gap_buffer<> gap("abcd");
  auto b = gap.begin();
  std::ignore = *b;
  std::ignore = b.operator->();
}

// Work tests (iterator)

TEST(gap_buffer_iterator_other, Dereference) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  EXPECT_EQ(*b, 'a');
  const gap_buffer<> cgap(gap);
  EXPECT_EQ(*cgap.begin(), 'a');
  EXPECT_EQ(*cgap.cbegin(), 'a');
}

TEST(gap_buffer_iterator_other, PreIncrement) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  EXPECT_EQ(*it, 'a');
  EXPECT_EQ(*++it, 'b');
  EXPECT_EQ(*++it, 'c');  // crosses the gap
  EXPECT_EQ(*++it, 'd');
}

TEST(gap_buffer_iterator_other, PostIncrement) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  EXPECT_EQ(*it++, 'a');
  EXPECT_EQ(*it++, 'b');
  EXPECT_EQ(*it++, 'c');  // crosses the gap
  EXPECT_EQ(*it, 'd');
}

TEST(gap_buffer_iterator_other, PreDecrement) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.end();
  EXPECT_EQ(*--it, 'd');
  EXPECT_EQ(*--it, 'c');
  EXPECT_EQ(*--it, 'b');  // crosses the gap
  EXPECT_EQ(*--it, 'a');
}

TEST(gap_buffer_iterator_other, PostDecrement) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.end();
  --it;
  EXPECT_EQ(*it--, 'd');
  EXPECT_EQ(*it--, 'c');
  EXPECT_EQ(*it--, 'b');  // crosses the gap
  EXPECT_EQ(*it, 'a');
}

TEST(gap_buffer_iterator_other, PlusEquals) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  it += 2;
  EXPECT_EQ(*it, 'c');  // jumped across the gap
  it += 1;
  EXPECT_EQ(*it, 'd');
}

TEST(gap_buffer_iterator_other, MinusEquals) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.end();
  it -= 2;
  EXPECT_EQ(*it, 'c');  // jumped across the gap
  it -= 1;
  EXPECT_EQ(*it, 'b');
}

TEST(gap_buffer_iterator_other, PlusN) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  EXPECT_EQ(*(b + 0), 'a');
  EXPECT_EQ(*(b + 1), 'b');
  EXPECT_EQ(*(b + 2), 'c');  // crosses the gap
  EXPECT_EQ(*(b + 3), 'd');
}

TEST(gap_buffer_iterator_other, MinusN) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto e = gap.end();
  EXPECT_EQ(*(e - 1), 'd');
  EXPECT_EQ(*(e - 2), 'c');
  EXPECT_EQ(*(e - 3), 'b');  // crosses the gap
  EXPECT_EQ(*(e - 4), 'a');
}

TEST(gap_buffer_iterator_other, Difference) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  auto e = gap.end();
  EXPECT_EQ(e - b, 4);
  EXPECT_EQ((b + 3) - (b + 1), 2);  // straddling the gap
  EXPECT_EQ(b - e, -4);
}

TEST(gap_buffer_iterator_other, SubscriptOperator) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  EXPECT_EQ(b[0], 'a');
  EXPECT_EQ(b[1], 'b');
  EXPECT_EQ(b[2], 'c');  // crosses the gap
  EXPECT_EQ(b[3], 'd');
}

TEST(gap_buffer_iterator_other, ForwardTraversal) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  std::string out;
  for (char c : gap) {
    out += c;
  }
  EXPECT_EQ(out, "abcd");
}

TEST(gap_buffer_iterator_other, ReverseTraversal) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  std::string out;
  for (auto it = gap.rbegin(); it != gap.rend(); ++it) {
    out += *it;
  }
  EXPECT_EQ(out, "dcba");
}

TEST(gap_buffer_iterator_other, Comparison) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  auto e = gap.end();
  EXPECT_TRUE(b == b);
  EXPECT_TRUE(b != e);
  EXPECT_TRUE(b < e);
  EXPECT_TRUE(e > b);
  EXPECT_TRUE(b <= b);
  EXPECT_TRUE(b >= b);
  EXPECT_FALSE(b == e);
  EXPECT_EQ(b + 4, e);
}

TEST(gap_buffer_iterator_other, MutationThroughIterator) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  *(gap.begin() + 2) = 'Y';  // crosses the gap
  EXPECT_EQ(gap.to_string(), "abYd");
}

TEST(gap_buffer_iterator_other, ConstIteratorConversion) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  ++it;
  gap_buffer<>::const_iterator cit = it;
  EXPECT_EQ(*cit, 'b');
  ++cit;
  EXPECT_EQ(*cit, 'c');  // crosses the gap
}

TEST(gap_buffer_iterator_other, ConstTraversal) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  const gap_buffer<> cgap(gap);
  std::string out;
  for (char c : cgap) {
    out += c;
  }
  EXPECT_EQ(out, "abcd");
}

// Regression tests

// Regression: iterator copy-assignment must transfer gap_size_ (was a self-swap
// in iterator_impl::swap, so gap_size_ never propagated through operator=).
TEST(gap_buffer_iterator_regression, AssignmentTransfersGapSize) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto src = gap.begin();  // gap_size_ == capacity - size == 12
  ASSERT_NE(src.get_gap_size(), 0);
  gap_buffer<>::iterator dst;  // gap_size_ == 0
  dst = src;
  EXPECT_EQ(dst.get_gap_size(), src.get_gap_size());
  EXPECT_EQ(dst.get_gap_index(), src.get_gap_index());
  EXPECT_EQ(*dst, 'a');
}

// Regression: with the cursor at logical position 0 the gap sits at the front,
// so begin() must point past the gap. Traversal used to read the raw buffer.
TEST(gap_buffer_iterator_regression, TraversalWithCursorAtFront) {
  gap_buffer<> gap("abcd");
  gap.step(-4);  // cursor to the very front
  ASSERT_EQ(gap.get_cursor_position_index(), 0);

  std::string forward;
  for (char c : gap) {
    forward += c;
  }
  EXPECT_EQ(forward, "abcd");
  EXPECT_EQ(forward, gap.to_string());

  std::string backward;
  for (auto it = gap.rbegin(); it != gap.rend(); ++it) {
    backward += *it;
  }
  EXPECT_EQ(backward, "dcba");

  EXPECT_EQ(gap.end() - gap.begin(), 4);
  for (std::size_t index = 0; index < gap.size(); ++index) {
    EXPECT_EQ(gap[index], "abcd"[index]);
  }
}

// Regression: an empty buffer must have begin() == end() so range-for is a no-op.
TEST(gap_buffer_iterator_regression, EmptyBufferTraversal) {
  gap_buffer<> gap;
  EXPECT_EQ(gap.begin(), gap.end());
  std::string out;
  for (char c : gap) {
    out += c;
  }
  EXPECT_EQ(out, "");
}

// Regression: same front-cursor case but heap-backed (capacity > SSO), so the
// gap-crossing arithmetic runs with a large gap_size_ and ASan sees heap access.
TEST(gap_buffer_iterator_regression, TraversalHeapCursorAtFront) {
  std::string text = "abcdefghijklmnopqrstuvwxyz";  // 26 chars -> heap
  gap_buffer<> gap(text);
  gap.step(-static_cast<int>(text.size()));
  ASSERT_GT(gap.capacity(), 16u);
  ASSERT_EQ(gap.get_cursor_position_index(), 0);

  std::string out;
  for (char c : gap) {
    out += c;
  }
  EXPECT_EQ(out, text);
}

// By design, resize() adds the new elements at the cursor, not at the end.
TEST(gap_buffer_other, ResizeInsertsAtCursor) {
  gap_buffer<> gap("abcd");
  gap.step(-2);            // cursor between "ab" and "cd"
  gap.resize(6, 'X');
  EXPECT_EQ(gap.to_string(), "abXXcd");
  EXPECT_EQ(gap.size(), 6);
}

// Regression: iterator + (negative delta) must cross the gap like - (positive),
// and iterator[] must match at the gap boundary. Both used to read gap bytes;
// the defects were masked on the stack by stale bytes left after a step().
TEST(gap_buffer_iterator_regression, NegativeDeltaCrossesGap) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.begin() + 2;  // 'c', just past the gap
  ASSERT_EQ(*it, 'c');
  EXPECT_EQ(*(it + (-1)), 'b');  // crosses the gap backwards
  EXPECT_EQ(*(it + (-1)), *(it - 1));
  EXPECT_EQ(*(it + (-2)), 'a');
}

TEST(gap_buffer_iterator_regression, SubscriptAtGapBoundaryHeap) {
  std::string text = "abcdefghijklmnopqrst";  // 20 chars -> heap
  gap_buffer<> gap(text);
  gap.step(-8);  // gap boundary sits at logical index 12
  auto b = gap.begin();
  for (std::size_t index = 0; index < text.size(); ++index) {
    EXPECT_EQ(b[index], text[index]);
  }
}

// Correctness tests for the added (previously untested) multi-char inserts.

TEST(gap_buffer_insert, InsertConstStringRef) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  const std::string text = "XY";
  auto it = gap.insert(text);
  EXPECT_EQ(gap.to_string(), "abXYcd");
  EXPECT_EQ(gap.size(), 6);
  EXPECT_EQ(*it, 'Y');
}

TEST(gap_buffer_insert, InsertStringRValue) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.insert(std::string("XY"));
  EXPECT_EQ(gap.to_string(), "abXYcd");
  EXPECT_EQ(gap.size(), 6);
  EXPECT_EQ(*it, 'Y');
}

TEST(gap_buffer_insert, InsertCString) {
  gap_buffer<> gap("abcd");
  gap.step(-2);
  auto it = gap.insert("XY");
  EXPECT_EQ(gap.to_string(), "abXYcd");
  EXPECT_EQ(gap.size(), 6);
  EXPECT_EQ(*it, 'Y');
}

TEST(gap_buffer_insert, InsertStringGrowsCapacity) {
  gap_buffer<> gap;
  gap.insert(std::string(40, 'z'));  // forces a reserve onto the heap
  EXPECT_EQ(gap.size(), 40);
  EXPECT_GT(gap.capacity(), 16u);
  EXPECT_EQ(gap.to_string(), std::string(40, 'z'));
}

TEST(gap_buffer_other, GetData) {
  gap_buffer<> gap("abcd");  // gap at the end: raw layout starts with the text
  const char* data = gap.get_data();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data[0], 'a');
  EXPECT_EQ(data[3], 'd');
}

// Heap-path (capacity > SSO) tests: exercise the branches that only run once the
// buffer has spilled onto the heap.

TEST(gap_buffer_heap, LargeConstructors) {
  gap_buffer<> from_count(25);
  EXPECT_EQ(from_count.size(), 25);
  EXPECT_EQ(from_count.to_string(), std::string(25, '\0'));

  gap_buffer<> from_fill(25, 'q');
  EXPECT_EQ(from_fill.to_string(), std::string(25, 'q'));

  gap_buffer<> from_str(std::string(20, 'w'));
  EXPECT_EQ(from_str.to_string(), std::string(20, 'w'));

  gap_buffer<> from_cstr("abcdefghijklmnopqrst");  // 20 chars
  EXPECT_EQ(from_cstr.to_string(), "abcdefghijklmnopqrst");
  EXPECT_GT(from_cstr.capacity(), 16u);
}

TEST(gap_buffer_heap, MoveConstructorHeap) {
  gap_buffer<> src(std::string(20, 'm'));
  ASSERT_GT(src.capacity(), 16u);
  gap_buffer<> dst(std::move(src));
  EXPECT_EQ(dst.to_string(), std::string(20, 'm'));
  EXPECT_EQ(src.size(), 0);
  EXPECT_EQ(src.capacity(), 16);
}

TEST(gap_buffer_heap, SwapHeapWithHeap) {
  gap_buffer<> a(std::string(20, 'a'));
  gap_buffer<> b(std::string(30, 'b'));
  a.swap(b);
  EXPECT_EQ(a.to_string(), std::string(30, 'b'));
  EXPECT_EQ(b.to_string(), std::string(20, 'a'));
}

TEST(gap_buffer_heap, SwapStackWithHeap) {
  gap_buffer<> small("abc");
  gap_buffer<> large(std::string(20, 'x'));
  small.swap(large);
  EXPECT_EQ(small.to_string(), std::string(20, 'x'));
  EXPECT_EQ(large.to_string(), "abc");
}

TEST(gap_buffer_heap, ClearHeap) {
  gap_buffer<> gap(std::string(20, 'y'));
  ASSERT_GT(gap.capacity(), 16u);
  gap.clear();
  EXPECT_EQ(gap.size(), 0);
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap.to_string(), "");
}

TEST(gap_buffer_heap, ReserveWithGapInMiddle) {
  gap_buffer<> gap("abcd");
  gap.step(-2);  // gap sits between "ab" and "cd"
  gap.reserve(64);
  EXPECT_EQ(gap.capacity(), 64);
  EXPECT_EQ(gap.to_string(), "abcd");
  gap.insert('X');
  EXPECT_EQ(gap.to_string(), "abXcd");
}

TEST(gap_buffer_heap, ShrinkToFitHeapWithGapInMiddle) {
  gap_buffer<> gap(std::string(20, 'a'));
  gap.reserve(64);
  gap.step(-5);  // gap in the middle, capacity has slack
  gap.shrink_to_fit();
  EXPECT_EQ(gap.capacity(), 20);
  EXPECT_EQ(gap.to_string(), std::string(20, 'a'));
}

// Exception-safety tests: a throwing allocator drives the catch(...) rollback
// paths. ASan (build-asan) additionally verifies these paths do not leak.

TEST(gap_buffer_exception, ConstructorRollback) {
  ThrowingAllocator<char>::reset();
  ThrowingAllocator<char>::throw_after = 5;  // throw on the 6th construct
  ThrowingAllocator<char> alloc;
  EXPECT_THROW((gap_buffer<ThrowingAllocator<char>>(25, 'a', alloc)),
               std::runtime_error);
  EXPECT_EQ(ThrowingAllocator<char>::live_allocations, 0u);
  ThrowingAllocator<char>::reset();
}

TEST(gap_buffer_exception, ReserveRollbackKeepsBufferIntact) {
  ThrowingAllocator<char>::reset();
  ThrowingAllocator<char> alloc;
  gap_buffer<ThrowingAllocator<char>> gap(5, 'a', alloc);  // stays on the stack
  ASSERT_EQ(gap.capacity(), 16);

  ThrowingAllocator<char>::construct_calls = 0;
  ThrowingAllocator<char>::live_allocations = 0;
  ThrowingAllocator<char>::throw_after = 3;  // throw partway through the copy
  EXPECT_THROW(gap.reserve(40), std::runtime_error);

  // Strong guarantee: the original buffer is untouched.
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap.to_string(), std::string(5, 'a'));
  EXPECT_EQ(ThrowingAllocator<char>::live_allocations, 0u);
  ThrowingAllocator<char>::reset();
}

TEST(gap_buffer_exception, ResizeRollbackKeepsSize) {
  ThrowingAllocator<char>::reset();
  ThrowingAllocator<char> alloc;
  gap_buffer<ThrowingAllocator<char>> gap(20, 'a', alloc);  // on the heap
  gap.reserve(60);  // give capacity slack so resize does not reallocate

  ThrowingAllocator<char>::throw_after = ThrowingAllocator<char>::construct_calls + 5;
  EXPECT_THROW(gap.resize(60, 'z'), std::runtime_error);

  EXPECT_EQ(gap.size(), 20);
  EXPECT_EQ(gap.to_string(), std::string(20, 'a'));
  ThrowingAllocator<char>::reset();
}

TEST(gap_buffer_heap, ForwardAndReverseTraversalHeap) {
  std::string text = "abcdefghijklmnopqrst";  // 20 chars -> heap
  gap_buffer<> gap(text);
  gap.step(-8);  // gap in the middle of a heap buffer

  std::string forward;
  for (char c : gap) {
    forward += c;
  }
  EXPECT_EQ(forward, text);

  std::string reverse(text.rbegin(), text.rend());
  std::string got;
  for (auto it = gap.rbegin(); it != gap.rend(); ++it) {
    got += *it;
  }
  EXPECT_EQ(got, reverse);

  for (std::size_t index = 0; index < gap.size(); ++index) {
    EXPECT_EQ(gap[index], text[index]);
  }
}
