#include "../../src/buffer/buffer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
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

// The primary element type is char; the buffer is now templated on the element
// type, so the character buffer is spelled gap_buffer<char> and a counting
// allocator is gap_buffer<char, AllocatorWithCount<char>>. gap_buffer<std::string>
// is exercised in its own section (that is the "outer" buffer of the editor's
// line-based hybrid model), and gap_buffer<TypeWithCounts> covers a non-trivial,
// instrumented element type.

// ======================= gap_buffer<char> smoke tests =======================

TEST(gap_buffer_smoke_test, DefaultConstructor) {
  gap_buffer<char> gap;
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, DefaultConstructorWithAllocator) {
  SetupTest();
  AllocatorWithCount<char> alloc;
  gap_buffer<char, AllocatorWithCount<char>> gap(alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumber) {
  gap_buffer<char> gap(25);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumberWithAllocator) {
  SetupTest();
  AllocatorWithCount<char> alloc;
  gap_buffer<char, AllocatorWithCount<char>> gap(25, alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumberAndLetter) {
  gap_buffer<char> gap(25, 'a');
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromNumberAndLetterWithAllocator) {
  SetupTest();
  AllocatorWithCount<char> alloc;
  gap_buffer<char, AllocatorWithCount<char>> gap(25, 'a', alloc);
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromString) {
  std::string str{"abacaba"};
  gap_buffer<char> gap1(str);
  gap_buffer<char> gap2(static_cast<std::string>("abacaba"));
  std::ignore = gap1;
  std::ignore = gap2;
}

TEST(gap_buffer_smoke_test, FromCString) {
  gap_buffer<char> gap("abacaba");
  std::ignore = gap;
}

TEST(gap_buffer_smoke_test, FromGapBuffer) {
  gap_buffer<char> gap1;
  gap_buffer<char> gap2(gap1);
  gap_buffer<char> gap3(std::move(gap1));
  std::ignore = gap2;
  std::ignore = gap3;
}

TEST(gap_buffer_smoke_test, SwapAndAssignAndClear) {
  gap_buffer<char> gap1("ab");
  gap_buffer<char> gap2("cd");
  gap1.swap(gap2);
  gap2 = gap1;
  gap1.clear();
}

TEST(gap_buffer_smoke_test, CapacityQueries) {
  gap_buffer<char> gap(15);
  std::ignore = gap.size();
  std::ignore = gap.capacity();
  std::ignore = gap.empty();
  gap.reserve(125);
  gap.resize(30, 'a');
  gap.shrink_to_fit();
}

TEST(gap_buffer_smoke_test, Iterators) {
  gap_buffer<char> gap(15);
  const gap_buffer<char> cgap(15);
  std::ignore = gap.begin();
  std::ignore = gap.end();
  std::ignore = gap.rbegin();
  std::ignore = gap.rend();
  std::ignore = cgap.begin();
  std::ignore = cgap.cbegin();
  std::ignore = cgap.end();
  std::ignore = cgap.cend();
  std::ignore = cgap.rbegin();
  std::ignore = cgap.crbegin();
  std::ignore = cgap.rend();
  std::ignore = cgap.crend();
}

TEST(gap_buffer_smoke_test, ElementAccessAndEdits) {
  gap_buffer<char> gap(15, 'a');
  const gap_buffer<char> cgap(15, 'a');
  std::ignore = gap.to_string();
  gap.insert('a');
  gap.erase_in_front_of_cursor();
  gap.erase_in_back_of_cursor();
  gap.step(5);
  gap.step(-5);
  std::ignore = gap[0];
  std::ignore = gap.at(0);
  std::ignore = cgap[14];
  std::ignore = cgap.at(14);
}

// ===================== gap_buffer<char> correctness tests ====================

TEST(gap_buffer_other, Equality) {
  gap_buffer<char> gap1("abcd");
  gap_buffer<char> gap2("abcd");
  EXPECT_EQ(gap1, gap2);
  gap1.step(-1);
  EXPECT_NE(gap1, gap2);
  gap2.step(-1);
  EXPECT_EQ(gap1, gap2);

  gap_buffer<char> gapcp;
  gapcp = gap1;
  EXPECT_EQ(gap1, gapcp);

  gap_buffer<char, AllocatorWithCount<char>> gap3(6, 'b');
  gap_buffer<char, AllocatorWithCount<char>> gap3_cp1(gap3);
  gap_buffer<char, AllocatorWithCount<char>> gap3_cp2(std::move(gap3));
  EXPECT_EQ(gap3_cp2, gap3_cp1);
}

TEST(gap_buffer_other, SizeAndCapacity) {
  gap_buffer<char> gap;
  EXPECT_TRUE(gap.empty());
  gap_buffer<char> gap1(25);
  gap_buffer<char> gap2(5);
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
  gap_buffer<char> gap1(5, 'a');
  gap_buffer<char> gap1_cp = gap1;
  gap1_cp.insert('b');
  EXPECT_EQ(gap1.to_string() + 'b', gap1_cp.to_string());
  gap1_cp.erase_in_front_of_cursor();
  EXPECT_EQ(gap1, gap1_cp);
  gap_buffer<char> gap3(5, 'b');
  gap_buffer<char> gap3_cp = gap3;
  gap1.swap(gap3);
  EXPECT_EQ(gap3, gap1_cp);
  EXPECT_EQ(gap1, gap3_cp);
}

TEST(gap_buffer_other, ToString) {
  gap_buffer<char> gap1(5, 'a');
  gap_buffer<char, AllocatorWithCount<char>> gap2(6, 'b');
  EXPECT_EQ(gap1.to_string(), std::string(5, 'a'));
  EXPECT_EQ(gap2.to_string(), std::string(6, 'b'));
  gap1.insert('b');
  gap2.erase_in_front_of_cursor();
  EXPECT_EQ(gap1.to_string(), std::string(5, 'a') + 'b');
  EXPECT_EQ(gap2.to_string(), std::string(5, 'b'));

  gap_buffer<char> gap("asddsa");
  gap.step(-3);
  EXPECT_EQ(gap.to_string(), "asddsa");
}

TEST(gap_buffer_other, Clear) {
  gap_buffer<char> gap(14, 'a');
  EXPECT_EQ(gap.size(), 14);
  EXPECT_EQ(gap.to_string(), std::string(14, 'a'));
  gap.clear();
  EXPECT_EQ(gap.size(), 0);
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap.to_string(), "");

  gap_buffer<char> gap1(18, 'a');
  EXPECT_EQ(gap1.size(), 18);
  EXPECT_EQ(gap1.to_string(), std::string(18, 'a'));
  gap1.clear();
  EXPECT_EQ(gap1.size(), 0);
  EXPECT_EQ(gap1.capacity(), 16);
  EXPECT_EQ(gap1.to_string(), "");
}

TEST(gap_buffer_other, ResizeAndReserve) {
  gap_buffer<char> gap;
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
  gap_buffer<char> gap1(5, 'a');
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
  gap_buffer<char> gap;
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
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  EXPECT_EQ(gap[0], 'a');
  EXPECT_EQ(gap[1], 'b');
  EXPECT_EQ(gap[2], 'c');
  EXPECT_EQ(gap[3], 'd');
  EXPECT_EQ(gap.at(0), 'a');
  EXPECT_EQ(gap.at(3), 'd');
  EXPECT_THROW((void)gap.at(4), std::out_of_range);
  const gap_buffer<char> gap1(gap);
  EXPECT_EQ(gap1[0], 'a');
  EXPECT_EQ(gap1[3], 'd');
  EXPECT_EQ(gap1.at(0), 'a');
  EXPECT_EQ(gap1.at(3), 'd');
  EXPECT_THROW((void)gap1.at(4), std::out_of_range);
}

TEST(gap_buffer_other, BeginEnd) {
  gap_buffer<char> gap("abcd");
  const gap_buffer<char> cgap("abcd");
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

TEST(gap_buffer_other, SelfSwapIsNoOp) {
  gap_buffer<char> stack_gap("abc");
  stack_gap.swap(stack_gap);
  EXPECT_EQ(stack_gap.to_string(), "abc");
  gap_buffer<char> heap_gap(std::string(20, 'z'));
  heap_gap.swap(heap_gap);
  EXPECT_EQ(heap_gap.to_string(), std::string(20, 'z'));
}

TEST(gap_buffer_other, SelfAndMoveAssignment) {
  gap_buffer<char> gap("abcd");
  gap = gap;  // self copy-assignment must not corrupt
  EXPECT_EQ(gap.to_string(), "abcd");
  gap_buffer<char> src(std::string(20, 'q'));
  gap_buffer<char> dst;
  dst = std::move(src);  // move-assignment
  EXPECT_EQ(dst.to_string(), std::string(20, 'q'));
}

// By design, resize() adds the new elements at the cursor, not at the end.
TEST(gap_buffer_other, ResizeInsertsAtCursor) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);  // cursor between "ab" and "cd"
  gap.resize(6, 'X');
  EXPECT_EQ(gap.to_string(), "abXXcd");
  EXPECT_EQ(gap.size(), 6);
}

TEST(gap_buffer_other, GetData) {
  gap_buffer<char> gap("abcd");  // gap at the end: raw layout starts with text
  const char* data = gap.get_data();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data[0], 'a');
  EXPECT_EQ(data[3], 'd');
}

// ==================== gap_buffer<char> iterator smoke tests ==================

TEST(gap_buffer_iterator_smoke, DefaultConstructor) {
  gap_buffer<char>::iterator iter;
  std::ignore = iter;
}

TEST(gap_buffer_iterator_smoke, FromPointerAndSize) {
  gap_buffer<char>::iterator iter(nullptr, 0, 0);
  std::ignore = iter;
}

TEST(gap_buffer_iterator_smoke, FromIterator) {
  gap_buffer<char>::iterator iter1;
  gap_buffer<char>::iterator iter(iter1);
  gap_buffer<char>::iterator iter2(std::move(iter1));
  std::ignore = iter;
  std::ignore = iter2;
}

TEST(gap_buffer_iterator_smoke, AssignmentOperator) {
  gap_buffer<char>::iterator iter1;
  gap_buffer<char>::iterator iter2;
  iter2 = iter1;
  std::ignore = iter2;
}

TEST(gap_buffer_iterator_smoke, Swap) {
  gap_buffer<char>::iterator iter1;
  gap_buffer<char>::iterator iter2;
  std::swap(iter1, iter2);
}

TEST(gap_buffer_iterator_smoke, ConversionToConst) {
  gap_buffer<char>::iterator iter;
  gap_buffer<char>::const_iterator citer = iter;
  std::ignore = citer;
}

TEST(gap_buffer_iterator_smoke, ArithmeticOperations) {
  gap_buffer<char> gap("abcd");
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
  gap_buffer<char> gap("abcd");
  auto b = gap.begin();
  std::ignore = b[0];
  std::ignore = b[3];
}

TEST(gap_buffer_iterator_smoke, Comparison) {
  gap_buffer<char> gap("abcd");
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
  gap_buffer<char> gap("abcd");
  auto b = gap.begin();
  std::ignore = *b;
  std::ignore = b.operator->();
}

// ==================== gap_buffer<char> iterator work tests ===================

TEST(gap_buffer_iterator_other, Dereference) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  EXPECT_EQ(*b, 'a');
  const gap_buffer<char> cgap(gap);
  EXPECT_EQ(*cgap.begin(), 'a');
  EXPECT_EQ(*cgap.cbegin(), 'a');
}

TEST(gap_buffer_iterator_other, PreIncrement) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  EXPECT_EQ(*it, 'a');
  EXPECT_EQ(*++it, 'b');
  EXPECT_EQ(*++it, 'c');  // crosses the gap
  EXPECT_EQ(*++it, 'd');
}

TEST(gap_buffer_iterator_other, PostIncrement) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  EXPECT_EQ(*it++, 'a');
  EXPECT_EQ(*it++, 'b');
  EXPECT_EQ(*it++, 'c');  // crosses the gap
  EXPECT_EQ(*it, 'd');
}

TEST(gap_buffer_iterator_other, PreDecrement) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.end();
  EXPECT_EQ(*--it, 'd');
  EXPECT_EQ(*--it, 'c');
  EXPECT_EQ(*--it, 'b');  // crosses the gap
  EXPECT_EQ(*--it, 'a');
}

TEST(gap_buffer_iterator_other, PostDecrement) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.end();
  --it;
  EXPECT_EQ(*it--, 'd');
  EXPECT_EQ(*it--, 'c');
  EXPECT_EQ(*it--, 'b');  // crosses the gap
  EXPECT_EQ(*it, 'a');
}

TEST(gap_buffer_iterator_other, PlusEquals) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  it += 2;
  EXPECT_EQ(*it, 'c');  // jumped across the gap
  it += 1;
  EXPECT_EQ(*it, 'd');
}

TEST(gap_buffer_iterator_other, MinusEquals) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.end();
  it -= 2;
  EXPECT_EQ(*it, 'c');  // jumped across the gap
  it -= 1;
  EXPECT_EQ(*it, 'b');
}

TEST(gap_buffer_iterator_other, PlusN) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  EXPECT_EQ(*(b + 0), 'a');
  EXPECT_EQ(*(b + 1), 'b');
  EXPECT_EQ(*(b + 2), 'c');  // crosses the gap
  EXPECT_EQ(*(b + 3), 'd');
}

TEST(gap_buffer_iterator_other, MinusN) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto e = gap.end();
  EXPECT_EQ(*(e - 1), 'd');
  EXPECT_EQ(*(e - 2), 'c');
  EXPECT_EQ(*(e - 3), 'b');  // crosses the gap
  EXPECT_EQ(*(e - 4), 'a');
}

TEST(gap_buffer_iterator_other, Difference) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  auto e = gap.end();
  EXPECT_EQ(e - b, 4);
  EXPECT_EQ((b + 3) - (b + 1), 2);  // straddling the gap
  EXPECT_EQ(b - e, -4);
}

TEST(gap_buffer_iterator_other, SubscriptOperator) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto b = gap.begin();
  EXPECT_EQ(b[0], 'a');
  EXPECT_EQ(b[1], 'b');
  EXPECT_EQ(b[2], 'c');  // crosses the gap
  EXPECT_EQ(b[3], 'd');
}

TEST(gap_buffer_iterator_other, ForwardTraversal) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  std::string out;
  for (char c : gap) {
    out += c;
  }
  EXPECT_EQ(out, "abcd");
}

TEST(gap_buffer_iterator_other, ReverseTraversal) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  std::string out;
  for (auto it = gap.rbegin(); it != gap.rend(); ++it) {
    out += *it;
  }
  EXPECT_EQ(out, "dcba");
}

TEST(gap_buffer_iterator_other, Comparison) {
  gap_buffer<char> gap("abcd");
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
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  *(gap.begin() + 2) = 'Y';  // crosses the gap
  EXPECT_EQ(gap.to_string(), "abYd");
}

TEST(gap_buffer_iterator_other, ConstIteratorConversion) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.begin();
  ++it;
  gap_buffer<char>::const_iterator cit = it;
  EXPECT_EQ(*cit, 'b');
  ++cit;
  EXPECT_EQ(*cit, 'c');  // crosses the gap
}

TEST(gap_buffer_iterator_other, ConstTraversal) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  const gap_buffer<char> cgap(gap);
  std::string out;
  for (char c : cgap) {
    out += c;
  }
  EXPECT_EQ(out, "abcd");
}

// ===================== gap_buffer<char> regression tests =====================

// Regression: iterator copy-assignment must transfer gap_size_ (was a self-swap
// in iterator_impl::swap, so gap_size_ never propagated through operator=).
TEST(gap_buffer_iterator_regression, AssignmentTransfersGapSize) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto src = gap.begin();  // gap_size_ == capacity - size == 12
  ASSERT_NE(src.get_gap_size(), 0);
  gap_buffer<char>::iterator dst;  // gap_size_ == 0
  dst = src;
  EXPECT_EQ(dst.get_gap_size(), src.get_gap_size());
  EXPECT_EQ(dst.get_gap_index(), src.get_gap_index());
  EXPECT_EQ(*dst, 'a');
}

// Regression: with the cursor at logical position 0 the gap sits at the front,
// so begin() must point past the gap. Traversal used to read the raw buffer.
TEST(gap_buffer_iterator_regression, TraversalWithCursorAtFront) {
  gap_buffer<char> gap("abcd");
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
  gap_buffer<char> gap;
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
  gap_buffer<char> gap(text);
  gap.step(-static_cast<int>(text.size()));
  ASSERT_GT(gap.capacity(), 16u);
  ASSERT_EQ(gap.get_cursor_position_index(), 0);

  std::string out;
  for (char c : gap) {
    out += c;
  }
  EXPECT_EQ(out, text);
}

// Regression: iterator + (negative delta) must cross the gap like - (positive),
// and iterator[] must match at the gap boundary.
TEST(gap_buffer_iterator_regression, NegativeDeltaCrossesGap) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.begin() + 2;  // 'c', just past the gap
  ASSERT_EQ(*it, 'c');
  EXPECT_EQ(*(it + (-1)), 'b');  // crosses the gap backwards
  EXPECT_EQ(*(it + (-1)), *(it - 1));
  EXPECT_EQ(*(it + (-2)), 'a');
}

TEST(gap_buffer_iterator_regression, SubscriptAtGapBoundaryHeap) {
  std::string text = "abcdefghijklmnopqrst";  // 20 chars -> heap
  gap_buffer<char> gap(text);
  gap.step(-8);  // gap boundary sits at logical index 12
  auto b = gap.begin();
  for (std::size_t index = 0; index < text.size(); ++index) {
    EXPECT_EQ(b[index], text[index]);
  }
}

// =================== gap_buffer<char> multi-char insert tests ================

TEST(gap_buffer_insert, InsertConstStringRef) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  const std::string text = "XY";
  auto it = gap.insert(text);
  EXPECT_EQ(gap.to_string(), "abXYcd");
  EXPECT_EQ(gap.size(), 6);
  EXPECT_EQ(*it, 'Y');
}

TEST(gap_buffer_insert, InsertStringRValue) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.insert(std::string("XY"));
  EXPECT_EQ(gap.to_string(), "abXYcd");
  EXPECT_EQ(gap.size(), 6);
  EXPECT_EQ(*it, 'Y');
}

TEST(gap_buffer_insert, InsertCString) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);
  auto it = gap.insert("XY");
  EXPECT_EQ(gap.to_string(), "abXYcd");
  EXPECT_EQ(gap.size(), 6);
  EXPECT_EQ(*it, 'Y');
}

TEST(gap_buffer_insert, InsertStringGrowsCapacity) {
  gap_buffer<char> gap;
  gap.insert(std::string(40, 'z'));  // forces a reserve onto the heap
  EXPECT_EQ(gap.size(), 40);
  EXPECT_GT(gap.capacity(), 16u);
  EXPECT_EQ(gap.to_string(), std::string(40, 'z'));
}

// ======================= gap_buffer<char> heap-path tests ====================

TEST(gap_buffer_heap, LargeConstructors) {
  gap_buffer<char> from_count(25);
  EXPECT_EQ(from_count.size(), 25);
  EXPECT_EQ(from_count.to_string(), std::string(25, '\0'));

  gap_buffer<char> from_fill(25, 'q');
  EXPECT_EQ(from_fill.to_string(), std::string(25, 'q'));

  gap_buffer<char> from_str(std::string(20, 'w'));
  EXPECT_EQ(from_str.to_string(), std::string(20, 'w'));

  gap_buffer<char> from_cstr("abcdefghijklmnopqrst");  // 20 chars
  EXPECT_EQ(from_cstr.to_string(), "abcdefghijklmnopqrst");
  EXPECT_GT(from_cstr.capacity(), 16u);
}

TEST(gap_buffer_heap, MoveConstructorHeap) {
  gap_buffer<char> src(std::string(20, 'm'));
  ASSERT_GT(src.capacity(), 16u);
  gap_buffer<char> dst(std::move(src));
  EXPECT_EQ(dst.to_string(), std::string(20, 'm'));
  EXPECT_EQ(src.size(), 0);
  EXPECT_EQ(src.capacity(), 16);
}

TEST(gap_buffer_heap, SwapHeapWithHeap) {
  gap_buffer<char> a(std::string(20, 'a'));
  gap_buffer<char> b(std::string(30, 'b'));
  a.swap(b);
  EXPECT_EQ(a.to_string(), std::string(30, 'b'));
  EXPECT_EQ(b.to_string(), std::string(20, 'a'));
}

TEST(gap_buffer_heap, SwapStackWithHeap) {
  gap_buffer<char> small("abc");
  gap_buffer<char> large(std::string(20, 'x'));
  small.swap(large);
  EXPECT_EQ(small.to_string(), std::string(20, 'x'));
  EXPECT_EQ(large.to_string(), "abc");
}

TEST(gap_buffer_heap, ClearHeap) {
  gap_buffer<char> gap(std::string(20, 'y'));
  ASSERT_GT(gap.capacity(), 16u);
  gap.clear();
  EXPECT_EQ(gap.size(), 0);
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap.to_string(), "");
}

TEST(gap_buffer_heap, ReserveWithGapInMiddle) {
  gap_buffer<char> gap("abcd");
  gap.step(-2);  // gap sits between "ab" and "cd"
  gap.reserve(64);
  EXPECT_EQ(gap.capacity(), 64);
  EXPECT_EQ(gap.to_string(), "abcd");
  gap.insert('X');
  EXPECT_EQ(gap.to_string(), "abXcd");
}

TEST(gap_buffer_heap, ShrinkToFitHeapWithGapInMiddle) {
  gap_buffer<char> gap(std::string(20, 'a'));
  gap.reserve(64);
  gap.step(-5);  // gap in the middle, capacity has slack
  gap.shrink_to_fit();
  EXPECT_EQ(gap.capacity(), 20);
  EXPECT_EQ(gap.to_string(), std::string(20, 'a'));
}

TEST(gap_buffer_heap, ForwardAndReverseTraversalHeap) {
  std::string text = "abcdefghijklmnopqrst";  // 20 chars -> heap
  gap_buffer<char> gap(text);
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

// ================== gap_buffer<char> exception-safety tests ==================

TEST(gap_buffer_exception, ConstructorRollback) {
  ThrowingAllocator<char>::reset();
  ThrowingAllocator<char>::throw_after = 5;  // throw on the 6th construct
  ThrowingAllocator<char> alloc;
  EXPECT_THROW((gap_buffer<char, ThrowingAllocator<char>>(25, 'a', alloc)),
               std::runtime_error);
  EXPECT_EQ(ThrowingAllocator<char>::live_allocations, 0u);
  ThrowingAllocator<char>::reset();
}

TEST(gap_buffer_exception, ReserveRollbackKeepsBufferIntact) {
  ThrowingAllocator<char>::reset();
  ThrowingAllocator<char> alloc;
  gap_buffer<char, ThrowingAllocator<char>> gap(5, 'a', alloc);  // on the stack
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
  gap_buffer<char, ThrowingAllocator<char>> gap(20, 'a', alloc);  // on the heap
  gap.reserve(60);  // give capacity slack so resize does not reallocate

  ThrowingAllocator<char>::throw_after =
      ThrowingAllocator<char>::construct_calls + 5;
  EXPECT_THROW(gap.resize(60, 'z'), std::runtime_error);

  EXPECT_EQ(gap.size(), 20);
  EXPECT_EQ(gap.to_string(), std::string(20, 'a'));
  ThrowingAllocator<char>::reset();
}

// ========================= gap_buffer<std::string> ==========================
// The line-based hybrid model stores the whole file as a gap_buffer<std::string>
// (one element per line). These tests exercise the buffer with a non-trivial,
// movable element type: no char-specific method (to_string, insert(string), ...)
// is available for it, so element access and iteration carry the assertions.

namespace {
std::string join(const gap_buffer<std::string>& gap) {
  std::string out;
  for (const std::string& line : gap) {
    out += line;
    out += '|';
  }
  return out;
}
}  // namespace

TEST(gap_buffer_string, ConstructAndInsert) {
  gap_buffer<std::string> gap;
  EXPECT_TRUE(gap.empty());
  gap.insert(std::string("alpha"));
  gap.insert(std::string("beta"));
  gap.insert("gamma");  // const char* converts to the std::string element
  EXPECT_EQ(gap.size(), 3);
  EXPECT_EQ(gap[0], "alpha");
  EXPECT_EQ(gap[1], "beta");
  EXPECT_EQ(gap[2], "gamma");
  EXPECT_EQ(join(gap), "alpha|beta|gamma|");
}

TEST(gap_buffer_string, FillConstructor) {
  gap_buffer<std::string> gap(3, std::string("x"));
  EXPECT_EQ(gap.size(), 3);
  EXPECT_EQ(join(gap), "x|x|x|");

  gap_buffer<std::string> empties(4);  // value-initialised -> empty strings
  EXPECT_EQ(empties.size(), 4);
  EXPECT_EQ(join(empties), "||||");
}

TEST(gap_buffer_string, StepInsertErase) {
  gap_buffer<std::string> gap;
  gap.insert(std::string("a"));
  gap.insert(std::string("b"));
  gap.insert(std::string("c"));
  gap.step(-1);  // cursor between "b" and "c"
  gap.insert(std::string("X"));
  EXPECT_EQ(join(gap), "a|b|X|c|");
  gap.erase_in_front_of_cursor();  // removes "X"
  EXPECT_EQ(join(gap), "a|b|c|");
  gap.erase_in_back_of_cursor();  // removes "c"
  EXPECT_EQ(join(gap), "a|b|");
}

TEST(gap_buffer_string, IndexAtAndMutation) {
  gap_buffer<std::string> gap;
  gap.insert(std::string("one"));
  gap.insert(std::string("two"));
  gap[1] = "TWO";
  EXPECT_EQ(gap.at(1), "TWO");
  EXPECT_THROW((void)gap.at(2), std::out_of_range);
}

TEST(gap_buffer_string, HeapGrowthAndOrder) {
  gap_buffer<std::string> gap;
  for (int i = 0; i < 20; ++i) {
    gap.insert(std::string(1, static_cast<char>('a' + i)));
  }
  ASSERT_GT(gap.capacity(), 16u);
  EXPECT_EQ(gap.size(), 20);
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(gap[i], std::string(1, static_cast<char>('a' + i)));
  }
}

TEST(gap_buffer_string, CopyAndMove) {
  gap_buffer<std::string> gap;
  for (int i = 0; i < 20; ++i) gap.insert(std::string("line") + std::to_string(i));
  gap_buffer<std::string> copy(gap);
  EXPECT_EQ(copy.size(), gap.size());
  EXPECT_TRUE(AreListsEqual(copy, gap));
  gap_buffer<std::string> moved(std::move(copy));
  EXPECT_EQ(moved.size(), 20);
  EXPECT_EQ(moved[0], "line0");
  EXPECT_EQ(moved[19], "line19");
  EXPECT_EQ(copy.size(), 0);
}

TEST(gap_buffer_string, SwapStackAndHeap) {
  gap_buffer<std::string> small;
  small.insert(std::string("p"));
  small.insert(std::string("q"));
  gap_buffer<std::string> big;
  for (int i = 0; i < 20; ++i) big.insert(std::string("b") + std::to_string(i));
  ASSERT_GT(big.capacity(), 16u);

  small.swap(big);  // stack <-> heap with a non-trivial element type
  EXPECT_EQ(small.size(), 20);
  EXPECT_EQ(small[0], "b0");
  EXPECT_EQ(big.size(), 2);
  EXPECT_EQ(big[0], "p");
  EXPECT_EQ(big[1], "q");
}

TEST(gap_buffer_string, ReserveResizeShrink) {
  gap_buffer<std::string> gap;
  gap.insert(std::string("keep0"));
  gap.insert(std::string("keep1"));
  gap.reserve(64);
  EXPECT_EQ(gap.capacity(), 64);
  EXPECT_EQ(gap[0], "keep0");
  EXPECT_EQ(gap[1], "keep1");
  gap.resize(5, std::string("pad"));  // adds three "pad" lines at the cursor
  EXPECT_EQ(gap.size(), 5);
  gap.shrink_to_fit();  // size <= SSO, so it shrinks back onto the stack
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_EQ(gap[0], "keep0");
  EXPECT_EQ(gap[1], "keep1");
  EXPECT_EQ(gap[2], "pad");
}

TEST(gap_buffer_string, CopyWithGapInMiddleHeap) {
  gap_buffer<std::string> gap;
  for (int i = 0; i < 20; ++i) gap.insert(std::string("L") + std::to_string(i));
  gap.step(-7);  // gap in the middle of a heap buffer
  gap_buffer<std::string> copy(gap);
  EXPECT_TRUE(AreListsEqual(copy, gap));
  EXPECT_EQ(copy.size(), 20);
  EXPECT_EQ(copy[0], "L0");
  EXPECT_EQ(copy[13], "L13");
  EXPECT_EQ(copy[19], "L19");
}

TEST(gap_buffer_string, InsertAtFrontAndBoundaryErase) {
  gap_buffer<std::string> gap;
  gap.insert(std::string("b"));
  gap.insert(std::string("c"));
  gap.step(-100);  // cursor clamped to the front
  gap.insert(std::string("a"));
  EXPECT_EQ(join(gap), "a|b|c|");

  gap.step(-100);
  gap.erase_in_front_of_cursor();  // at the front -> no-op
  EXPECT_EQ(gap.size(), 3);
  gap.step(100);
  gap.erase_in_back_of_cursor();  // at the end -> no-op
  EXPECT_EQ(gap.size(), 3);
}

TEST(gap_buffer_string, SelfSwapIsNoOp) {
  gap_buffer<std::string> gap;
  gap.insert(std::string("long-heap-allocated-string-one"));
  gap.insert(std::string("long-heap-allocated-string-two"));
  gap.swap(gap);
  EXPECT_EQ(gap.size(), 2);
  EXPECT_EQ(gap[0], "long-heap-allocated-string-one");
  EXPECT_EQ(gap[1], "long-heap-allocated-string-two");
}

TEST(gap_buffer_string, ClearReleasesHeap) {
  gap_buffer<std::string> gap;
  for (int i = 0; i < 20; ++i) gap.insert(std::string("l") + std::to_string(i));
  gap.clear();
  EXPECT_EQ(gap.size(), 0);
  EXPECT_EQ(gap.capacity(), 16);
  EXPECT_TRUE(gap.empty());
}

TEST(gap_buffer_string, ExceptionRollbackOnFill) {
  ThrowingAllocator<std::string>::reset();
  ThrowingAllocator<std::string>::throw_after = 5;  // throw on the 6th construct
  ThrowingAllocator<std::string> alloc;
  EXPECT_THROW((gap_buffer<std::string, ThrowingAllocator<std::string>>(
                   25, std::string("x"), alloc)),
               std::runtime_error);
  EXPECT_EQ(ThrowingAllocator<std::string>::live_allocations, 0u);
  ThrowingAllocator<std::string>::reset();
}

// Regression: reserve() must give the strong guarantee for a non-trivial element
// type even when the allocator throws mid-transfer. A move-based transfer would
// leave the (moved-from) source corrupted; reserve copies to keep it intact.
TEST(gap_buffer_string, ReserveRollbackStrongGuarantee) {
  ThrowingAllocator<std::string>::reset();
  gap_buffer<std::string, ThrowingAllocator<std::string>> gap(20,
                                                              std::string("keep"));
  ASSERT_GT(gap.capacity(), 16u);

  ThrowingAllocator<std::string>::throw_after =
      ThrowingAllocator<std::string>::construct_calls + 10;  // throw mid-copy
  EXPECT_THROW(gap.reserve(200), std::runtime_error);

  // Strong guarantee: every original element is still intact.
  ASSERT_EQ(gap.size(), 20u);
  for (std::size_t index = 0; index < gap.size(); ++index) {
    EXPECT_EQ(gap[index], "keep");
  }
  ThrowingAllocator<std::string>::reset();
}

// ==================== gap_buffer<non-trivial instrumented> ===================
// TypeWithCounts is copyable/movable and every construct/destroy goes through
// the counting allocator, so after all buffers die the construct/destroy and
// allocate/deallocate tallies must balance -- proving the manual element
// lifetime (union storage, reserve, shrink, swap, copy, move) leaks nothing.

TEST(gap_buffer_complex, LifetimeIsBalanced) {
  SetupTest();
  {
    using Buf = gap_buffer<TypeWithCounts, AllocatorWithCount<TypeWithCounts>>;
    Buf gap;
    for (int i = 0; i < 20; ++i) gap.insert(TypeWithCounts(i));  // heap + reserves
    gap.step(-5);
    gap.insert(TypeWithCounts(99));
    gap.erase_in_front_of_cursor();
    gap.shrink_to_fit();

    Buf copy(gap);
    Buf moved(std::move(copy));

    Buf small;
    small.insert(TypeWithCounts(1));
    small.swap(moved);  // stack <-> heap
  }
  EXPECT_EQ(MemoryManager::allocator_constructed,
            MemoryManager::allocator_destroyed);
  EXPECT_EQ(MemoryManager::allocator_allocated,
            MemoryManager::allocator_deallocated);
}
