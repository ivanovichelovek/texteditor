#pragma once
#include <new>

#include "../tests/memory_utils.hpp"

struct TypeWithFancyNewDeleteOperators {
  TypeWithFancyNewDeleteOperators() = default;
  explicit TypeWithFancyNewDeleteOperators(int value) : value(value) {}

  static void* operator new(size_t n) {
    MemoryManager::TypeNewAllocate(n);
    return ::operator new(n);
  }

  static void operator delete(void* ptr, size_t n) {
    MemoryManager::TypeNewDelete(n);
    ::operator delete(ptr);
  }

  int value = 0;
};

struct OnlyMovable : public TypeWithFancyNewDeleteOperators {
  OnlyMovable(int a) { std::ignore = a; }
  OnlyMovable() = delete;
  OnlyMovable(const OnlyMovable&) = delete;
  OnlyMovable(OnlyMovable&& other) noexcept { std::ignore = other; }
};

struct TypeWithCounts : public TypeWithFancyNewDeleteOperators {
  using smart_counter = std::shared_ptr<size_t>;

  TypeWithCounts(int v) {
    value = v;
    *int_c += 1;
  }

  TypeWithCounts() {
    value = 0;
    *default_c += 1;
  }

  TypeWithCounts(const TypeWithCounts& other)
      : TypeWithFancyNewDeleteOperators(other.value) {
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *copy_c += 1;
  }

  TypeWithCounts(TypeWithCounts&& other) {
    value = other.value;
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *move_c += 1;
  }

  TypeWithCounts& operator=(const TypeWithCounts& other) {
    value = other.value;
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *ass_copy += 1;
    return *this;
  }

  TypeWithCounts& operator=(TypeWithCounts&& other) noexcept {
    value = other.value;
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *ass_move += 1;
    return *this;
  }

  smart_counter default_c = std::make_shared<size_t>(0);
  smart_counter copy_c = std::make_shared<size_t>(0);
  smart_counter move_c = std::make_shared<size_t>(0);
  smart_counter int_c = std::make_shared<size_t>(0);
  smart_counter ass_copy = std::make_shared<size_t>(0);
  smart_counter ass_move = std::make_shared<size_t>(0);
};

inline bool operator==(const TypeWithCounts& lhs, const TypeWithCounts& rhs) {
  return lhs.default_c == rhs.default_c && lhs.copy_c == rhs.copy_c &&
         lhs.move_c == rhs.move_c && lhs.int_c == rhs.int_c &&
         lhs.ass_copy == rhs.ass_copy && lhs.ass_move == rhs.ass_move;
}

inline bool operator!=(const TypeWithCounts& lhs, const TypeWithCounts& rhs) {
  return !(lhs == rhs);
}

struct Accountant {
  // Some field of strange size
  char arr[40];

  static size_t ctor_calls;
  static size_t dtor_calls;

  // NO LINT
  static void reset() {
    ctor_calls = 0;
    dtor_calls = 0;
  }

  Accountant() { ++ctor_calls; }

  // NO LINT
  Accountant(const Accountant&) { ++ctor_calls; }

  // NO LINT
  Accountant& operator=(const Accountant&) {
    // Actually, when it comes to assign one list to another,
    // list can use element-wise assignment instead of destroying nodes and
    // creating new ones
    ++ctor_calls;
    ++dtor_calls;
    return *this;
  }

  Accountant(Accountant&&) = delete;
  Accountant& operator=(Accountant&&) = delete;

  ~Accountant() { ++dtor_calls; }
};

struct ThrowingAccountant : public Accountant {
  static bool need_throw;

  int value = 0;

  // NO LINT
  ThrowingAccountant(int value = 0) : Accountant(), value(value) {
    if (need_throw && ctor_calls % 5 == 4)
      throw std::string("Ahahahaha you have been cocknut");
  }

  // NO LINT
  ThrowingAccountant(const ThrowingAccountant& other)
      : Accountant(), value(other.value) {
    if (need_throw && ctor_calls % 5 == 4)
      throw std::string("Ahahahaha you have been cocknut");
  }

  // NO LINT
  ThrowingAccountant& operator=(const ThrowingAccountant& other) {
    value = other.value;
    ++ctor_calls;
    ++dtor_calls;
    if (need_throw && ctor_calls % 5 == 4)
      throw std::string("Ahahahaha you have been cocknut");
    return *this;
  }
};

template <typename FirstList, typename SecondList>
bool AreListsEqual(const FirstList& first, const SecondList& second) {
  if (first.size() != second.size()) {
    return false;
  }

  auto first_it = first.begin();
  auto second_it = second.begin();

  for (; first_it != first.end() && second_it != second.end();
       ++first_it, ++second_it) {
    if (*first_it != *second_it) {
      return false;
    }
  }

  return true;
}
