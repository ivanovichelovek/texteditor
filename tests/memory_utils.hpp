// [genius_naming][nolint]
#pragma once
#include <cstdio>
#include <cstdlib>
#include <memory>

struct MemoryManager {
  static size_t type_new_allocated;
  static size_t type_new_deleted;
  static size_t allocator_allocated;
  static size_t allocator_deallocated;

  static size_t allocator_constructed;
  static size_t allocator_destroyed;

  static void TypeNewAllocate(size_t n) { type_new_allocated += n; }

  static void TypeNewDelete(size_t n) { type_new_deleted += n; }

  static void AllocatorAllocate(size_t n) { allocator_allocated += n; }

  static void AllocatorDeallocate(size_t n) { allocator_deallocated += n; }

  static void AllocatorConstruct(size_t n) { allocator_constructed += n; }

  static void AllocatorDestroy(size_t n) { allocator_destroyed += n; }
};

inline void* operator new(size_t n, bool from_my_allocator) {
  (void)from_my_allocator;
  return malloc(n);
}

inline void operator delete(void* ptr, size_t n,
                            bool from_my_allocator) noexcept {
  (void)n;
  (void)from_my_allocator;
  free(ptr);
}

template <typename T>
struct AllocatorWithCount {
  using value_type = T;

  AllocatorWithCount() = default;

  template <typename U>
  AllocatorWithCount(const AllocatorWithCount<U>&) {}

  T* allocate(size_t n) {
    MemoryManager::AllocatorAllocate(n * sizeof(T));
    allocator_allocated += n * sizeof(T);
    return reinterpret_cast<T*>(operator new(n * sizeof(T), true));
  }

  void deallocate(T* ptr, size_t n) {
    MemoryManager::AllocatorDeallocate(n * sizeof(T));
    allocator_deallocated += n * sizeof(T);
    operator delete(ptr, n, true);
  }

  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args) {
    MemoryManager::AllocatorConstruct(1);
    allocator_constructed += 1;
    ::new (ptr) U(std::forward<Args>(args)...);
  }

  template <typename U>
  void destroy(U* ptr) noexcept {
    MemoryManager::AllocatorDestroy(1);
    allocator_destroyed += 1;
    ptr->~U();
  }

  size_t allocator_allocated = 0;
  size_t allocator_deallocated = 0;
  size_t allocator_constructed = 0;
  size_t allocator_destroyed = 0;
};

template <typename T>
bool operator==(const AllocatorWithCount<T>& lhs,
                const AllocatorWithCount<T>& rhs) {
  return lhs.allocator_allocated == rhs.allocator_allocated &&
         lhs.allocator_deallocated == rhs.allocator_deallocated &&
         lhs.allocator_constructed == rhs.allocator_constructed &&
         lhs.allocator_destroyed == rhs.allocator_destroyed;
}

template <typename T, bool PropagateOnConstruct, bool PropagateOnAssign>
struct WhimsicalAllocator : public std::allocator<T> {
  std::shared_ptr<int> number;

  auto select_on_container_copy_construction() const {
    return PropagateOnConstruct ? WhimsicalAllocator<T, PropagateOnConstruct,
                                                     PropagateOnAssign>()
                                : *this;
  }

  struct propagate_on_container_copy_assignment
      : std::conditional_t<PropagateOnAssign, std::true_type, std::false_type> {
  };

  template <typename U>
  struct rebind {
    using other =
        WhimsicalAllocator<U, PropagateOnConstruct, PropagateOnAssign>;
  };

  WhimsicalAllocator() : number(std::make_shared<int>(counter)) { ++counter; }

  template <typename U>
  WhimsicalAllocator(const WhimsicalAllocator<U, PropagateOnConstruct,
                                              PropagateOnAssign>& another)
      : number(another.number) {}

  template <typename U>
  auto& operator=(const WhimsicalAllocator<U, PropagateOnConstruct,
                                           PropagateOnAssign>& another) {
    number = another.number;
    return *this;
  }

  template <typename U>
  bool operator==(const WhimsicalAllocator<U, PropagateOnConstruct,
                                           PropagateOnAssign>& another) const {
    return *number == *another.number;
  }

  template <typename U>
  bool operator!=(const WhimsicalAllocator<U, PropagateOnConstruct,
                                           PropagateOnAssign>& another) const {
    return *number != *another.number;
  }

  static size_t counter;
};
