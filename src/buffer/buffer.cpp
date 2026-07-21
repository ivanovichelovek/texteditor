#include "buffer.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer() : allocator_(Alloc()) {};

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(Alloc allocator) : allocator_(allocator) {};

template <typename T, Allocator Alloc>
template <typename Generator>
void gap_buffer<T, Alloc>::fill_construct(std::size_t size,
                                          Generator generator) {
  if (size > capacity_) {
    reserve(size);
  }
  try {
    for (std::size_t index = 0; index < size; ++index) {
      construct(index, generator(index));
      ++size_;
    }
  } catch (...) {
    for (std::size_t index = 0; index < size_; ++index) {
      destroy(index);
      --size_;
    }
    if (capacity_ > kSSOBufferSize) {
      alloc_traits::deallocate(allocator_, data_.heap_buffer, capacity_);
      data_.heap_buffer = nullptr;
    }
    throw;
  }
  gap_index_ = size_;
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(std::size_t size, Alloc allocator)
    : allocator_(allocator) {
  fill_construct(size, [](std::size_t index) { return T{}; });
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(std::size_t size, T value, Alloc allocator)
    : allocator_(allocator) {
  fill_construct(size, [value](std::size_t index) { return value; });
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(const T* text, Alloc allocator)
  requires std::same_as<T, char>
    : allocator_(allocator) {
  fill_construct(std::strlen(text),
                 [text](std::size_t index) { return text[index]; });
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(const std::string& text, Alloc allocator)
  requires std::same_as<T, char>
    : allocator_(allocator) {
  fill_construct(text.size(),
                 [&text](std::size_t index) { return text[index]; });
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(std::string&& text, Alloc allocator)
  requires std::same_as<T, char>
    : allocator_(allocator) {
  fill_construct(text.size(),
                 [&text](std::size_t index) { return std::move(text[index]); });
  text.clear();
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(const gap_buffer& other)
    : allocator_(alloc_traits::select_on_container_copy_construction(
          other.allocator_)) {
  std::size_t size = other.size();
  if (other.capacity_ > capacity_) {
    reserve(other.capacity_);
  }
  std::size_t raw_after = other.gap_index_ + (other.capacity_ - size);
  try {
    for (std::size_t index = 0; index < other.gap_index_; ++index) {
      construct(index, other.choose_buffer()[index]);
      ++size_;
    }
    for (std::size_t index = raw_after; index < other.capacity_; ++index) {
      construct(index, other.choose_buffer()[index]);
      ++size_;
    }
  } catch (...) {
    for (std::size_t index = 0; index < other.gap_index_ && index < size_;
         ++index) {
      destroy(index);
    }
    std::size_t copied_after =
        size_ > other.gap_index_ ? size_ - other.gap_index_ : 0;
    for (std::size_t i = 0; i < copied_after; ++i) {
      destroy(raw_after + i);
    }
    size_ = 0;
    if (capacity_ > kSSOBufferSize) {
      alloc_traits::deallocate(allocator_, data_.heap_buffer, capacity_);
      data_.heap_buffer = nullptr;
    }
    throw;
  }
  gap_index_ = other.gap_index_;
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::gap_buffer(gap_buffer&& other) noexcept
  requires std::is_nothrow_move_constructible_v<T>
    : size_(other.size_),
      capacity_(other.capacity_),
      gap_index_(other.gap_index_),
      allocator_(std::move(other.allocator_)) {
  if (other.capacity_ > kSSOBufferSize) {
    data_.heap_buffer = other.data_.heap_buffer;
    other.data_.heap_buffer = nullptr;
  } else {
    relocate(allocator_, other.data_.stack_buffer, data_.stack_buffer, size_,
             gap_index_, capacity_);
  }
  other.size_ = 0;
  other.capacity_ = kSSOBufferSize;
  other.gap_index_ = 0;
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::swap(gap_buffer& other) {
  if (this == &other) {
    return;
  }
  bool this_heap = capacity_ > kSSOBufferSize;
  bool other_heap = other.capacity_ > kSSOBufferSize;

  if (this_heap && other_heap) {
    std::swap(data_.heap_buffer, other.data_.heap_buffer);
  } else if (!this_heap && !other_heap) {
    // Both on the stack: shuffle elements through temporary raw storage so each
    // buffer ends up holding the other's elements at the other's raw indices.
    alignas(T) unsigned char storage[sizeof(T) * kSSOBufferSize];
    T* tmp = reinterpret_cast<T*>(storage);
    relocate(allocator_, data_.stack_buffer, tmp, size_, gap_index_, capacity_);
    relocate(allocator_, other.data_.stack_buffer, data_.stack_buffer,
             other.size_, other.gap_index_, other.capacity_);
    relocate(allocator_, tmp, other.data_.stack_buffer, size_, gap_index_,
             capacity_);
  } else {
    // One heap, one stack: move the stack side's elements into the heap side's
    // (freed) stack storage, and hand the heap pointer to the stack side.
    gap_buffer& heap_side = this_heap ? *this : other;
    gap_buffer& stack_side = this_heap ? other : *this;
    T* ptr = heap_side.data_.heap_buffer;
    relocate(allocator_, stack_side.data_.stack_buffer,
             heap_side.data_.stack_buffer, stack_side.size_,
             stack_side.gap_index_, stack_side.capacity_);
    stack_side.data_.heap_buffer = ptr;
  }

  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
  std::swap(gap_index_, other.gap_index_);
  std::swap(allocator_, other.allocator_);
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>& gap_buffer<T, Alloc>::operator=(gap_buffer other) {
  swap(other);
  return *this;
}

template <typename T, Allocator Alloc>
gap_buffer<T, Alloc>::~gap_buffer() {
  clear();
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::clear() {
  destroy_elements();
  if (capacity_ > kSSOBufferSize) {
    alloc_traits::deallocate(allocator_, data_.heap_buffer, capacity_);
  }
  size_ = 0;
  capacity_ = kSSOBufferSize;
  gap_index_ = 0;
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::resize(std::size_t new_size, T value) {
  if (size_ >= new_size) {
    return;
  }
  if (new_size > capacity_) {
    reserve(new_size);
  }
  auto old_size = size_;
  try {
    while (size_ < new_size) {
      construct(gap_index_, value);
      ++gap_index_;
      ++size_;
    }
  } catch (...) {
    while (size_ > old_size) {
      destroy(--gap_index_);
      --size_;
    }
    throw;
  }
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::reserve(std::size_t new_capacity) {
  if (new_capacity <= capacity_) {
    return;
  }
  T* new_data = alloc_traits::allocate(allocator_, new_capacity);
  T* src = choose_buffer();
  std::size_t raw_after_old = gap_index_ + (capacity_ - size_);
  std::size_t raw_after_new = gap_index_ + (new_capacity - size_);
  std::size_t before = 0;  // constructed elements before the gap
  std::size_t after = 0;   // constructed elements after the gap
  // Copy (do not move) from the old storage so that a throw partway through --
  // including one raised by the allocator itself -- leaves the source intact,
  // giving reserve the strong exception guarantee.
  try {
    for (; before < static_cast<std::size_t>(gap_index_); ++before) {
      alloc_traits::construct(allocator_, new_data + before, src[before]);
    }
    for (; after < size_ - gap_index_; ++after) {
      alloc_traits::construct(allocator_, new_data + raw_after_new + after,
                              src[raw_after_old + after]);
    }
  } catch (...) {
    for (std::size_t index = 0; index < before; ++index) {
      alloc_traits::destroy(allocator_, new_data + index);
    }
    for (std::size_t index = 0; index < after; ++index) {
      alloc_traits::destroy(allocator_, new_data + raw_after_new + index);
    }
    alloc_traits::deallocate(allocator_, new_data, new_capacity);
    throw;
  }
  destroy_elements();
  if (capacity_ > kSSOBufferSize) {
    alloc_traits::deallocate(allocator_, data_.heap_buffer, capacity_);
  }
  capacity_ = new_capacity;
  data_.heap_buffer = new_data;
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::shrink_to_fit() {
  if (size_ == capacity_ || capacity_ <= kSSOBufferSize) {
    return;  // nothing to shrink, or already at the minimum (stack) capacity
  }
  // Source is the heap buffer (capacity_ > kSSOBufferSize).
  T* old = data_.heap_buffer;
  std::size_t gap = static_cast<std::size_t>(gap_index_);
  std::size_t raw_after_old = gap_index_ + (capacity_ - size_);
  if (size_ > kSSOBufferSize) {
    // Shrink to a smaller heap buffer with capacity_ == size_ (gap collapses).
    T* new_data = alloc_traits::allocate(allocator_, size_);
    std::size_t built = 0;
    try {
      for (; built < gap; ++built) {
        alloc_traits::construct(allocator_, new_data + built, old[built]);
      }
      for (std::size_t j = 0; j < size_ - gap; ++j, ++built) {
        alloc_traits::construct(allocator_, new_data + gap + j,
                                old[raw_after_old + j]);
      }
    } catch (...) {
      for (std::size_t index = 0; index < built; ++index) {
        alloc_traits::destroy(allocator_, new_data + index);
      }
      alloc_traits::deallocate(allocator_, new_data, size_);
      throw;
    }
    destroy_elements();
    alloc_traits::deallocate(allocator_, old, capacity_);
    capacity_ = size_;
    data_.heap_buffer = new_data;
  } else if (size_ == 0) {
    // Nothing to relocate: just release the heap and return to the stack.
    alloc_traits::deallocate(allocator_, old, capacity_);
    capacity_ = kSSOBufferSize;
  } else {
    // Shrink onto the stack. The stack_buffer aliases the heap pointer in the
    // union, so we cannot build in place: relocate through a temporary. Copying
    // into the temp is strong (a throw leaves *this untouched); only after the
    // temp holds a full compacted copy do we free the heap and move into the
    // union (moves are noexcept for the supported element types).
    T* tmp = alloc_traits::allocate(allocator_, size_);
    std::size_t built = 0;
    try {
      for (; built < gap; ++built) {
        alloc_traits::construct(allocator_, tmp + built, old[built]);
      }
      for (std::size_t j = 0; j < size_ - gap; ++j, ++built) {
        alloc_traits::construct(allocator_, tmp + built,
                                old[raw_after_old + j]);
      }
    } catch (...) {
      for (std::size_t index = 0; index < built; ++index) {
        alloc_traits::destroy(allocator_, tmp + index);
      }
      alloc_traits::deallocate(allocator_, tmp, size_);
      throw;
    }
    destroy_elements();  // choose_buffer() is still the heap (capacity_
                         // unchanged)
    alloc_traits::deallocate(allocator_, data_.heap_buffer, capacity_);
    std::size_t raw_after_new = gap_index_ + (kSSOBufferSize - size_);
    for (std::size_t index = 0; index < gap; ++index) {
      alloc_traits::construct(allocator_, data_.stack_buffer + index,
                              std::move(tmp[index]));
    }
    for (std::size_t j = 0; j < size_ - gap; ++j) {
      alloc_traits::construct(allocator_,
                              data_.stack_buffer + raw_after_new + j,
                              std::move(tmp[gap + j]));
    }
    for (std::size_t index = 0; index < size_; ++index) {
      alloc_traits::destroy(allocator_, tmp + index);
    }
    alloc_traits::deallocate(allocator_, tmp, size_);
    capacity_ = kSSOBufferSize;
  }
}

template <typename T, Allocator Alloc>
[[nodiscard]] std::size_t gap_buffer<T, Alloc>::size() const noexcept {
  return size_;
}

template <typename T, Allocator Alloc>
[[nodiscard]] std::size_t gap_buffer<T, Alloc>::capacity() const noexcept {
  return capacity_;
}

template <typename T, Allocator Alloc>
[[nodiscard]] bool gap_buffer<T, Alloc>::empty() const noexcept {
  return size_ == 0;
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::begin() -> iterator {
  if (gap_index_ == 0) {
    return iterator(choose_buffer() + (capacity_ - size_),
                    -static_cast<int>(capacity_ - size_), capacity_ - size_);
  }
  return iterator(choose_buffer(), gap_index_, capacity_ - size_);
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::begin() const -> const_iterator {
  if (gap_index_ == 0) {
    return const_iterator(choose_buffer() + (capacity_ - size_),
                          -static_cast<int>(capacity_ - size_),
                          capacity_ - size_);
  }
  return const_iterator(choose_buffer(), gap_index_, capacity_ - size_);
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::cbegin() const -> const_iterator {
  if (gap_index_ == 0) {
    return const_iterator(choose_buffer() + (capacity_ - size_),
                          -static_cast<int>(capacity_ - size_),
                          capacity_ - size_);
  }
  return const_iterator(choose_buffer(), gap_index_, capacity_ - size_);
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::end() -> iterator {
  return iterator(choose_buffer() + capacity_,
                  gap_index_ - static_cast<int>(capacity_), capacity_ - size_);
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::end() const -> const_iterator {
  return const_iterator(choose_buffer() + capacity_,
                        gap_index_ - static_cast<int>(capacity_),
                        capacity_ - size_);
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::cend() const -> const_iterator {
  return const_iterator(choose_buffer() + capacity_,
                        gap_index_ - static_cast<int>(capacity_),
                        capacity_ - size_);
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::rbegin() -> reverse_iterator {
  return std::make_reverse_iterator(end());
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::rbegin() const
    -> const_reverse_iterator {
  return std::make_reverse_iterator(end());
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::crbegin() const
    -> const_reverse_iterator {
  return std::make_reverse_iterator(cend());
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::rend() -> reverse_iterator {
  return std::make_reverse_iterator(begin());
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::rend() const
    -> const_reverse_iterator {
  return std::make_reverse_iterator(begin());
}

template <typename T, Allocator Alloc>
[[nodiscard]] auto gap_buffer<T, Alloc>::crend() const
    -> const_reverse_iterator {
  return std::make_reverse_iterator(cbegin());
}

template <typename T, Allocator Alloc>
[[nodiscard]] std::string gap_buffer<T, Alloc>::to_string() const
  requires std::same_as<T, char>
{
  std::string answer;
  for (std::size_t index = 0; index < size_; ++index) {
    if (index < gap_index_) {
      answer += choose_buffer()[index];
    } else {
      answer += choose_buffer()[index + (capacity_ - size_)];
    }
  }
  return answer;
}

template <typename T, Allocator Alloc>
auto gap_buffer<T, Alloc>::insert(T value) -> iterator {
  if (size_ == capacity_) {
    reserve(capacity_ << 1);
  }
  ++size_;
  construct(gap_index_, std::move(value));
  ++gap_index_;
  return begin() + (gap_index_ - 1);
}

template <typename T, Allocator Alloc>
auto gap_buffer<T, Alloc>::insert(const std::string& text) -> iterator
  requires std::same_as<T, char>
{
  if (size_ + text.size() > capacity_) {
    reserve((size_ + text.size()) << 1);
  }
  for (auto& value : text) {
    ++size_;
    construct(gap_index_, value);
    ++gap_index_;
  }
  return begin() + (gap_index_ - 1);
}

template <typename T, Allocator Alloc>
auto gap_buffer<T, Alloc>::insert(std::string&& text) -> iterator
  requires std::same_as<T, char>
{
  if (size_ + text.size() > capacity_) {
    reserve((size_ + text.size()) << 1);
  }
  for (auto& value : text) {
    ++size_;
    construct(gap_index_, std::move(value));
    ++gap_index_;
  }
  text.clear();
  return begin() + (gap_index_ - 1);
}

template <typename T, Allocator Alloc>
auto gap_buffer<T, Alloc>::insert(const T* text) -> iterator
  requires std::same_as<T, char>
{
  std::size_t len = std::strlen(text);
  if (size_ + len > capacity_) {
    reserve((size_ + len) << 1);
  }
  for (std::size_t index = 0; index < len; ++index) {
    ++size_;
    construct(gap_index_, text[index]);
    ++gap_index_;
  }
  return begin() + (gap_index_ - 1);
}

template <typename T, Allocator Alloc>
auto gap_buffer<T, Alloc>::erase_in_front_of_cursor() -> iterator {
  if (gap_index_ == 0) {
    return begin();
  }
  --gap_index_;
  destroy(gap_index_);
  --size_;
  return begin() + (gap_index_);
}

template <typename T, Allocator Alloc>
auto gap_buffer<T, Alloc>::erase_in_back_of_cursor() -> iterator {
  if (gap_index_ == size_) {
    return end();
  }
  destroy(gap_index_ + (capacity_ - size_));
  --size_;
  return begin() + (gap_index_);
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::step(int delta) {
  if (size_ == capacity_) {
    if (delta < 0) {
      gap_index_ -= std::min(-delta, static_cast<int>(gap_index_));
    } else {
      gap_index_ += std::min(delta, static_cast<int>(size_) - gap_index_);
    }
    return;
  }
  if (delta < 0) {
    int limit = std::min(-delta, gap_index_);
    for (int index = 0; index < limit; ++index) {
      construct(gap_index_ + (capacity_ - size_ - 1),
                choose_buffer()[gap_index_ - 1]);
      destroy(gap_index_ - 1);
      --gap_index_;
    }
  } else {
    int limit = std::min(delta, static_cast<int>(size_) - gap_index_);
    for (std::size_t index = 0; index < limit; ++index) {
      construct(gap_index_, choose_buffer()[gap_index_ + (capacity_ - size_)]);
      destroy(gap_index_ + (capacity_ - size_));
      ++gap_index_;
    }
  }
}

template <typename T, Allocator Alloc>
[[nodiscard]] T& gap_buffer<T, Alloc>::operator[](std::size_t index) {
  return *(begin() + index);
}

template <typename T, Allocator Alloc>
[[nodiscard]] T& gap_buffer<T, Alloc>::at(std::size_t index) {
  if (index >= size_) {
    throw std::out_of_range("Index out of range!");
  }
  return *(begin() + index);
}

template <typename T, Allocator Alloc>
[[nodiscard]] const T& gap_buffer<T, Alloc>::operator[](
    std::size_t index) const {
  return *(begin() + index);
}

template <typename T, Allocator Alloc>
[[nodiscard]] const T& gap_buffer<T, Alloc>::at(std::size_t index) const {
  if (index >= size_) {
    throw std::out_of_range("Index out of range!");
  }
  return *(begin() + index);
}

template <typename T, Allocator Alloc>
[[nodiscard]] std::size_t gap_buffer<T, Alloc>::get_cursor_position_index()
    const {
  return gap_index_;
}

template <typename T, Allocator Alloc>
T* gap_buffer<T, Alloc>::choose_buffer() {
  if (kSSOBufferSize < capacity_) {
    return data_.heap_buffer;
  }
  return data_.stack_buffer;
}

template <typename T, Allocator Alloc>
const T* gap_buffer<T, Alloc>::choose_buffer() const {
  if (kSSOBufferSize < capacity_) {
    return data_.heap_buffer;
  }
  return data_.stack_buffer;
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::construct(std::size_t index, T value) {
  alloc_traits::construct(allocator_, choose_buffer() + index,
                          std::move(value));
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::destroy(std::size_t index) {
  alloc_traits::destroy(allocator_, choose_buffer() + index);
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::destroy_elements() {
  T* buf = choose_buffer();
  std::size_t raw_after = gap_index_ + (capacity_ - size_);
  for (std::size_t index = 0; index < static_cast<std::size_t>(gap_index_);
       ++index) {
    alloc_traits::destroy(allocator_, buf + index);
  }
  for (std::size_t index = raw_after; index < capacity_; ++index) {
    alloc_traits::destroy(allocator_, buf + index);
  }
}

template <typename T, Allocator Alloc>
void gap_buffer<T, Alloc>::relocate(ReboundAlloc& alloc, T* src, T* dst,
                                    std::size_t size, int gap_index,
                                    std::size_t capacity) {
  std::size_t raw_after = gap_index + (capacity - size);
  for (std::size_t index = 0; index < static_cast<std::size_t>(gap_index);
       ++index) {
    alloc_traits::construct(alloc, dst + index, std::move(src[index]));
    alloc_traits::destroy(alloc, src + index);
  }
  for (std::size_t index = raw_after; index < capacity; ++index) {
    alloc_traits::construct(alloc, dst + index, std::move(src[index]));
    alloc_traits::destroy(alloc, src + index);
  }
}

template <typename T, Allocator Alloc>
const T* gap_buffer<T, Alloc>::get_data() const {
  return choose_buffer();
}

template <typename T, Allocator Alloc>
[[nodiscard]] bool operator==(const gap_buffer<T, Alloc>& lhs,
                              const gap_buffer<T, Alloc>& rhs) {
  if (!(lhs.capacity() == rhs.capacity() && lhs.size() == rhs.size() &&
        lhs.get_cursor_position_index() == rhs.get_cursor_position_index())) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (*(lhs.begin() + index) != *(rhs.begin() + index)) {
      return false;
    }
  }
  return true;
}

template <typename T, Allocator Alloc>
[[nodiscard]] bool operator!=(const gap_buffer<T, Alloc>& lhs,
                              const gap_buffer<T, Alloc>& rhs) {
  return !(lhs == rhs);
}

template <typename T, Allocator Alloc>
template <bool IsConst>
gap_buffer<T, Alloc>::iterator_impl<IsConst>::iterator_impl() = default;

template <typename T, Allocator Alloc>
template <bool IsConst>
gap_buffer<T, Alloc>::iterator_impl<IsConst>::iterator_impl(
    pointer data, std::size_t gap_index, std::size_t gap_size)
    : data_(data), gap_index_(gap_index), gap_size_(gap_size) {}

template <typename T, Allocator Alloc>
template <bool IsConst>
gap_buffer<T, Alloc>::iterator_impl<IsConst>::iterator_impl(
    const iterator_impl& other)
    : data_(other.data_),
      gap_index_(other.gap_index_),
      gap_size_(other.gap_size_) {}

template <typename T, Allocator Alloc>
template <bool IsConst>
gap_buffer<T, Alloc>::iterator_impl<IsConst>::iterator_impl(
    iterator_impl&& other) noexcept
    : data_(other.data_),
      gap_index_(other.gap_index_),
      gap_size_(other.gap_size_) {
  other.data_ = nullptr;
  other.gap_index_ = 0;
  other.gap_size_ = 0;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator=(
    iterator_impl other) -> iterator_impl& {
  swap(other);
  return *this;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
void gap_buffer<T, Alloc>::iterator_impl<IsConst>::swap(iterator_impl& other) {
  std::swap(data_, other.data_);
  std::swap(gap_index_, other.gap_index_);
  std::swap(gap_size_, other.gap_size_);
}

template <typename T, Allocator Alloc>
template <bool IsConst>
gap_buffer<T, Alloc>::iterator_impl<IsConst>::~iterator_impl() {
  data_ = nullptr;
  gap_index_ = 0;
  gap_size_ = 0;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator iterator_impl<true>()
    const {
  return gap_buffer<T, Alloc>::iterator_impl<true>(data_, gap_index_,
                                                   gap_size_);
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator++()
    -> iterator_impl& {
  if (gap_index_ == 1) {
    data_ += 1 + gap_size_;
    gap_index_ -= 1 + gap_size_;
  } else {
    ++data_;
    --gap_index_;
  }
  return *this;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator++(int)
    -> iterator_impl {
  auto iter_copy = *this;
  if (gap_index_ == 1) {
    data_ += 1 + gap_size_;
    gap_index_ -= 1 + gap_size_;
  } else {
    ++data_;
    --gap_index_;
  }
  return iter_copy;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator--(int)
    -> iterator_impl {
  auto iter_copy = *this;
  if (gap_index_ + gap_size_ == 0) {
    data_ -= 1 + gap_size_;
    gap_index_ += 1 + gap_size_;
  } else {
    --data_;
    ++gap_index_;
  }
  return iter_copy;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator--()
    -> iterator_impl& {
  if (gap_index_ + gap_size_ == 0) {
    data_ -= 1 + gap_size_;
    gap_index_ += 1 + gap_size_;
  } else {
    --data_;
    ++gap_index_;
  }
  return *this;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator+=(
    difference_type delta) -> iterator_impl& {
  // gap_index_ > 0 means we are before the gap, gap_index_ <= 0 after it.
  // A move crosses the gap when it changes which side we are on; crossing
  // forwards adds the gap, crossing backwards removes it.
  difference_type shift = delta;
  if (gap_index_ > 0) {
    if (delta >= gap_index_) {
      shift = delta + gap_size_;  // cross the gap forwards
    }
  } else if (delta < gap_index_ + gap_size_) {
    shift = delta - gap_size_;  // cross the gap backwards
  }
  data_ += shift;
  gap_index_ -= shift;
  return *this;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator-=(
    difference_type delta) -> iterator_impl& {
  return *this += -delta;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator+(
    difference_type delta) const -> iterator_impl {
  iterator_impl iter_copy(data_, gap_index_, gap_size_);
  iter_copy += delta;
  return iter_copy;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator-(
    difference_type delta) const -> iterator_impl {
  iterator_impl iter_copy(data_, gap_index_, gap_size_);
  iter_copy -= delta;
  return iter_copy;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator[](
    difference_type delta) const -> reference {
  return *(*this + delta);
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator*()
    const -> reference {
  return *data_;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] auto gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator->()
    const -> pointer {
  return data_;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] bool gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator==(
    const iterator_impl& other) const {
  return (data_ == other.data_ && gap_size_ == other.gap_size_ &&
          gap_index_ == other.gap_index_);
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] std::strong_ordering
gap_buffer<T, Alloc>::iterator_impl<IsConst>::operator<=>(
    const iterator_impl& other) const {
  return data_ <=> other.data_;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] int gap_buffer<T, Alloc>::iterator_impl<IsConst>::get_gap_index()
    const {
  return gap_index_;
}

template <typename T, Allocator Alloc>
template <bool IsConst>
[[nodiscard]] int gap_buffer<T, Alloc>::iterator_impl<IsConst>::get_gap_size()
    const {
  return gap_size_;
}

template <typename IterL, typename IterR>
  requires is_gap_buffer_iterator<IterL> && is_gap_buffer_iterator<IterR>
auto operator-(const IterL& lhs, const IterR& rhs)
    -> decltype(lhs.get_gap_index() - rhs.get_gap_index()) {
  auto logical_pos = [](const auto& it) {
    auto pos = -it.get_gap_index();
    if (it.get_gap_index() < 0) pos -= it.get_gap_size();
    return pos;
  };

  return logical_pos(lhs) - logical_pos(rhs);
}
