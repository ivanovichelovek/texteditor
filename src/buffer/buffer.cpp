#include "buffer.hpp"

#include <alloca.h>

#include <cstring>
#include <stdexcept>

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer() : allocator_(Alloc()) {};

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(Alloc allocator)
  requires Allocator<Alloc>
    : allocator_(allocator) {};

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(std::size_t size, Alloc allocator)
    : allocator_(allocator) {
  if (size > capacity_) {
    reserve(size);
  }
  try {
    for (std::size_t index = 0; index < size; ++index) {
      construct(index, '\0');
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

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(std::size_t size, char character, Alloc allocator)
    : allocator_(allocator) {
  if (size > capacity_) {
    reserve(size);
  }
  try {
    for (std::size_t index = 0; index < size; ++index) {
      construct(index, character);
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

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(const std::string& text, Alloc allocator)
    : allocator_(allocator) {
  std::size_t size = text.size();
  if (size > capacity_) {
    reserve(size);
  }
  try {
    for (std::size_t index = 0; index < size; ++index) {
      construct(index, text[index]);
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

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(std::string&& text, Alloc allocator)
    : allocator_(allocator) {
  std::size_t size = text.size();
  if (size > capacity_) {
    reserve(size);
  }
  try {
    for (std::size_t index = 0; index < size; ++index) {
      construct(index, std::move(text[index]));
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
  text = "";
}

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(const char* text, Alloc allocator)
    : allocator_(allocator) {
  std::size_t size = strlen(text);
  if (size > capacity_) {
    reserve(size);
  }
  try {
    for (std::size_t index = 0; index < size; ++index) {
      construct(index, text[index]);
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

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(const gap_buffer& other)
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

template <typename Alloc>
gap_buffer<Alloc>::gap_buffer(gap_buffer&& other)
    : data_(std::move(other.data_)),
      size_(other.size_),
      capacity_(other.capacity_),
      gap_index_(other.gap_index_),
      allocator_(std::move(other.allocator_)) {
  if (capacity_ > kSSOBufferSize) {
    other.data_.heap_buffer = nullptr;
  }
  other.size_ = 0;
  other.capacity_ = kSSOBufferSize;
  other.gap_index_ = 0;
}

template <typename Alloc>
void gap_buffer<Alloc>::swap(gap_buffer& other) {
  bool this_on_stack = capacity_ <= kSSOBufferSize;
  bool other_on_stack = other.capacity_ <= kSSOBufferSize;

  if (this_on_stack && other_on_stack) {
    std::swap(data_.stack_buffer, other.data_.stack_buffer);
  } else if (!this_on_stack && !other_on_stack) {
    std::swap(data_.heap_buffer, other.data_.heap_buffer);
  } else if (this_on_stack && !other_on_stack) {
    char tmp[kSSOBufferSize];
    std::memcpy(tmp, data_.stack_buffer, kSSOBufferSize);
    data_.heap_buffer = other.data_.heap_buffer;
    std::memcpy(other.data_.stack_buffer, tmp, kSSOBufferSize);
  } else {
    char tmp[kSSOBufferSize];
    std::memcpy(tmp, other.data_.stack_buffer, kSSOBufferSize);
    other.data_.heap_buffer = data_.heap_buffer;
    std::memcpy(data_.stack_buffer, tmp, kSSOBufferSize);
  }

  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
  std::swap(gap_index_, other.gap_index_);
  std::swap(allocator_, other.allocator_);
}

template <typename Alloc>
gap_buffer<Alloc>& gap_buffer<Alloc>::operator=(gap_buffer other) {
  swap(other);
  return *this;
}

template <typename Alloc>
gap_buffer<Alloc>::~gap_buffer() {
  clear();
}

template <typename Alloc>
void gap_buffer<Alloc>::clear() {
  if (capacity_ > kSSOBufferSize) {
    for (std::size_t index = 0; index < size_; ++index) {
      destroy(index);
    }
    alloc_traits::deallocate(allocator_, data_.heap_buffer, capacity_);
  }
  size_ = 0;
  capacity_ = 16;
  gap_index_ = 0;
}

template <typename Alloc>
void gap_buffer<Alloc>::resize(std::size_t new_size, char character) {
  if (size_ >= new_size) {
    return;
  }
  if (new_size > capacity_) {
    reserve(new_size);
  }
  auto old_size = size_;
  try {
    while (size_ < new_size) {
      construct(gap_index_, character);
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

template <typename Alloc>
void gap_buffer<Alloc>::reserve(std::size_t new_capacity) {
  if (new_capacity <= capacity_) {
    return;
  }
  char* new_data = alloc_traits::allocate(allocator_, new_capacity);
  try {
    for (std::size_t index = 0; index < size_; ++index) {
      if (gap_index_ > index) {
        alloc_traits::construct(allocator_, new_data + index,
                                choose_buffer()[index]);
      } else {
        alloc_traits::construct(allocator_,
                                new_data + index + (new_capacity - size_),
                                choose_buffer()[index + (capacity_ - size_)]);
      }
    }
  } catch (...) {
    alloc_traits::deallocate(allocator_, new_data, new_capacity);
    throw;
  }
  if (capacity_ > kSSOBufferSize) {
    for (std::size_t index = 0; index < capacity_; ++index) {
      if (gap_index_ > index || (gap_index_ + (capacity_ - size_)) < index) {
        alloc_traits::destroy(allocator_, choose_buffer() + index);
      }
    }
    alloc_traits::deallocate(allocator_, choose_buffer(), capacity_);
  }
  capacity_ = new_capacity;
  data_.heap_buffer = new_data;
}

template <typename Alloc>
void gap_buffer<Alloc>::shrink_to_fit() {
  if (size_ == capacity_) {
    return;
  }
  if (size_ > kSSOBufferSize) {
    char* new_data = alloc_traits::allocate(allocator_, size_);
    try {
      for (std::size_t index = 0; index < size_; ++index) {
        if (gap_index_ > index) {
          alloc_traits::construct(allocator_, new_data + index,
                                  choose_buffer()[index]);
        } else {
          alloc_traits::construct(allocator_, new_data + index,
                                  choose_buffer()[index + (capacity_ - size_)]);
        }
      }
    } catch (...) {
      alloc_traits::deallocate(allocator_, new_data, size_);
      throw;
    }
    if (capacity_ > kSSOBufferSize) {
      for (std::size_t index = 0; index < capacity_; ++index) {
        if (gap_index_ > index || (gap_index_ + (capacity_ - size_)) < index) {
          alloc_traits::destroy(allocator_, choose_buffer() + index);
        }
      }
      alloc_traits::deallocate(allocator_, choose_buffer(), capacity_);
    }
    capacity_ = size_;
    data_.heap_buffer = new_data;
  } else {
    char data_copy[kSSOBufferSize];
    std::memset(data_copy, 0, sizeof(data_copy));
    for (std::size_t index = 0; index < size_; ++index) {
      if (gap_index_ > index) {
        data_copy[index] = choose_buffer()[index];
      } else {
        data_copy[index] = choose_buffer()[index + (capacity_ - size_)];
      }
    }
    if (capacity_ > kSSOBufferSize) {
      for (std::size_t index = 0; index < capacity_; ++index) {
        if (gap_index_ > index || (gap_index_ + (capacity_ - size_)) < index) {
          alloc_traits::destroy(allocator_, choose_buffer() + index);
        }
      }
      alloc_traits::deallocate(allocator_, choose_buffer(), capacity_);
    }
    capacity_ = kSSOBufferSize;
    for (std::size_t index = 0; index < kSSOBufferSize; ++index) {
      data_.stack_buffer[index] = data_copy[index];
    }
  }
}

template <typename Alloc>
[[nodiscard]] std::size_t gap_buffer<Alloc>::size() const noexcept {
  return size_;
}

template <typename Alloc>
[[nodiscard]] std::size_t gap_buffer<Alloc>::capacity() const noexcept {
  return capacity_;
}

template <typename Alloc>
[[nodiscard]] bool gap_buffer<Alloc>::empty() const noexcept {
  return size_ == 0;
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::begin() -> iterator {
  if (gap_index_ == 0) {
    return iterator(choose_buffer() + (capacity_ - size_),
                    -static_cast<int>(capacity_ - size_), capacity_ - size_);
  }
  return iterator(choose_buffer(), gap_index_, capacity_ - size_);
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::begin() const -> const_iterator {
  if (gap_index_ == 0) {
    return const_iterator(choose_buffer() + (capacity_ - size_),
                          -static_cast<int>(capacity_ - size_),
                          capacity_ - size_);
  }
  return const_iterator(choose_buffer(), gap_index_, capacity_ - size_);
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::cbegin() const -> const_iterator {
  if (gap_index_ == 0) {
    return const_iterator(choose_buffer() + (capacity_ - size_),
                          -static_cast<int>(capacity_ - size_),
                          capacity_ - size_);
  }
  return const_iterator(choose_buffer(), gap_index_, capacity_ - size_);
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::end() -> iterator {
  return iterator(choose_buffer() + capacity_,
                  gap_index_ - static_cast<int>(capacity_), capacity_ - size_);
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::end() const -> const_iterator {
  return const_iterator(choose_buffer() + capacity_,
                        gap_index_ - static_cast<int>(capacity_),
                        capacity_ - size_);
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::cend() const -> const_iterator {
  return const_iterator(choose_buffer() + capacity_,
                        gap_index_ - static_cast<int>(capacity_),
                        capacity_ - size_);
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::rbegin() -> reverse_iterator {
  return std::make_reverse_iterator(end());
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::rbegin() const -> const_reverse_iterator {
  return std::make_reverse_iterator(end());
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::crbegin() const
    -> const_reverse_iterator {
  return std::make_reverse_iterator(cend());
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::rend() -> reverse_iterator {
  return std::make_reverse_iterator(begin());
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::rend() const -> const_reverse_iterator {
  return std::make_reverse_iterator(begin());
}

template <typename Alloc>
[[nodiscard]] auto gap_buffer<Alloc>::crend() const -> const_reverse_iterator {
  return std::make_reverse_iterator(cbegin());
}

template <typename Alloc>
[[nodiscard]] std::string gap_buffer<Alloc>::to_string() const {
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

template <typename Alloc>
auto gap_buffer<Alloc>::insert(char character) -> iterator {
  if (size_ == capacity_) {
    reserve(capacity_ << 1);
  }
  ++size_;
  construct(gap_index_, character);
  ++gap_index_;
  return begin() + (gap_index_ - 1);
}

template <typename Alloc>
auto gap_buffer<Alloc>::insert(const std::string& text) -> iterator {
  if (size_ + text.size() > capacity_) {
    reserve((size_ + text.size()) << 1);
  }
  for (auto& character : text) {
    ++size_;
    construct(gap_index_, character);
    ++gap_index_;
  }
  return begin() + (gap_index_ - 1);
}

template <typename Alloc>
auto gap_buffer<Alloc>::insert(std::string&& text) -> iterator {
  if (size_ + text.size() > capacity_) {
    reserve((size_ + text.size()) << 1);
  }
  for (auto& character : text) {
    ++size_;
    construct(gap_index_, std::move(character));
    ++gap_index_;
  }
  return begin() + (gap_index_ - 1);
}

template <typename Alloc>
auto gap_buffer<Alloc>::insert(const char* text) -> iterator {
  int len = std::strlen(text);
  if (size_ + len > capacity_) {
    reserve((size_ + len) << 1);
  }
  for (int index = 0; index < len; ++index) {
    ++size_;
    construct(gap_index_, text[index]);
    ++gap_index_;
  }
  return begin() + (gap_index_ - 1);
}

template <typename Alloc>
auto gap_buffer<Alloc>::erase_in_front_of_cursor() -> iterator {
  if (gap_index_ == 0) {
    return begin();
  }
  --gap_index_;
  destroy(gap_index_);
  --size_;
  return begin() + (gap_index_ + 1);
}

template <typename Alloc>
auto gap_buffer<Alloc>::erase_in_back_of_cursor() -> iterator {
  if (gap_index_ == size_) {
    return end();
  }
  destroy(gap_index_ + (capacity_ - size_));
  --size_;
  return begin() + (gap_index_ + 1);
}

template <typename Alloc>
void gap_buffer<Alloc>::step(int delta) {
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

template <typename Alloc>
[[nodiscard]] char& gap_buffer<Alloc>::operator[](std::size_t index) {
  return *(begin() + index);
}

template <typename Alloc>
[[nodiscard]] char& gap_buffer<Alloc>::at(std::size_t index) {
  if (index >= size_) {
    throw std::out_of_range("Index out of range!");
  }
  return *(begin() + index);
}

template <typename Alloc>
[[nodiscard]] const char& gap_buffer<Alloc>::operator[](
    std::size_t index) const {
  return *(begin() + index);
}

template <typename Alloc>
[[nodiscard]] const char& gap_buffer<Alloc>::at(std::size_t index) const {
  if (index >= size_) {
    throw std::out_of_range("Index out of range!");
  }
  return *(begin() + index);
}

template <typename Alloc>
[[nodiscard]] std::size_t gap_buffer<Alloc>::get_cursor_position_index() const {
  return gap_index_;
}

template <typename Alloc>
char* gap_buffer<Alloc>::choose_buffer() {
  if (kSSOBufferSize < capacity_) {
    return data_.heap_buffer;
  }
  return data_.stack_buffer;
}

template <typename Alloc>
const char* gap_buffer<Alloc>::choose_buffer() const {
  if (kSSOBufferSize < capacity_) {
    return data_.heap_buffer;
  }
  return data_.stack_buffer;
}

template <typename Alloc>
void gap_buffer<Alloc>::construct(std::size_t index, char character) {
  if (capacity_ > kSSOBufferSize) {
    alloc_traits::construct(allocator_, data_.heap_buffer + index, character);
  } else {
    data_.stack_buffer[index] = character;
  }
}

template <typename Alloc>
void gap_buffer<Alloc>::destroy(std::size_t index) {
  if (capacity_ > kSSOBufferSize) {
    alloc_traits::destroy(allocator_, choose_buffer() + index);
  }
}

template <typename Alloc>
const char* gap_buffer<Alloc>::get_data() const {
  return choose_buffer();
}

template <typename Alloc>
[[nodiscard]] bool operator==(const gap_buffer<Alloc>& lhs,
                              const gap_buffer<Alloc>& rhs) {
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

template <typename Alloc>
[[nodiscard]] bool operator!=(const gap_buffer<Alloc>& lhs,
                              const gap_buffer<Alloc>& rhs) {
  return !(lhs == rhs);
}

template <typename Alloc>
template <bool IsConst>
gap_buffer<Alloc>::iterator_impl<IsConst>::iterator_impl() = default;

template <typename Alloc>
template <bool IsConst>
gap_buffer<Alloc>::iterator_impl<IsConst>::iterator_impl(pointer data,
                                                         std::size_t gap_index,
                                                         std::size_t gap_size)
    : data_(data), gap_index_(gap_index), gap_size_(gap_size) {}

template <typename Alloc>
template <bool IsConst>
gap_buffer<Alloc>::iterator_impl<IsConst>::iterator_impl(
    const iterator_impl& other)
    : data_(other.data_),
      gap_index_(other.gap_index_),
      gap_size_(other.gap_size_) {}

template <typename Alloc>
template <bool IsConst>
gap_buffer<Alloc>::iterator_impl<IsConst>::iterator_impl(iterator_impl&& other)
    : data_(other.data_),
      gap_index_(other.gap_index_),
      gap_size_(other.gap_size_) {
  other.data_ = nullptr;
  other.gap_index_ = 0;
  other.gap_size_ = 0;
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator=(iterator_impl other)
    -> iterator_impl& {
  swap(other);
  return *this;
}

template <typename Alloc>
template <bool IsConst>
void gap_buffer<Alloc>::iterator_impl<IsConst>::swap(iterator_impl& other) {
  std::swap(data_, other.data_);
  std::swap(gap_index_, other.gap_index_);
  std::swap(other.gap_size_, other.gap_size_);
}

template <typename Alloc>
template <bool IsConst>
gap_buffer<Alloc>::iterator_impl<IsConst>::~iterator_impl() {
  data_ = nullptr;
  gap_index_ = 0;
  gap_size_ = 0;
}

template <typename Alloc>
template <bool IsConst>
gap_buffer<Alloc>::iterator_impl<IsConst>::operator iterator_impl<true>()
    const {
  return gap_buffer<Alloc>::iterator_impl<true>(data_, gap_index_, gap_size_);
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator++() -> iterator_impl& {
  if (gap_index_ == 1) {
    data_ += 1 + gap_size_;
    gap_index_ -= 1 + gap_size_;
  } else {
    ++data_;
    --gap_index_;
  }
  return *this;
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator++(int)
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

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator--(int)
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

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator--() -> iterator_impl& {
  if (gap_index_ + gap_size_ == 0) {
    data_ -= 1 + gap_size_;
    gap_index_ += 1 + gap_size_;
  } else {
    --data_;
    ++gap_index_;
  }
  return *this;
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator+=(
    difference_type delta) -> iterator_impl& {
  if (gap_index_ <= static_cast<int>(delta) && gap_index_ > 0) {
    data_ += delta + gap_size_;
    gap_index_ -= delta + gap_size_;
  } else {
    data_ += delta;
    gap_index_ -= delta;
  }
  return *this;
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator-=(
    difference_type delta) -> iterator_impl& {
  if (gap_index_ + gap_size_ + delta >= 0 && gap_index_ < 0) {
    data_ -= delta + gap_size_;
    gap_index_ += delta + gap_size_;
  } else {
    data_ -= delta;
    gap_index_ += delta;
  }
  return *this;
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator+(
    difference_type delta) const -> iterator_impl {
  iterator_impl iter_copy(data_, gap_index_, gap_size_);
  iter_copy += delta;
  return iter_copy;
}

template <typename Alloc>
template <bool IsConst>
auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator-(
    difference_type delta) const -> iterator_impl {
  iterator_impl iter_copy(data_, gap_index_, gap_size_);
  iter_copy -= delta;
  return iter_copy;
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator[](
    difference_type delta) const -> reference {
  if (gap_index_ < delta) {
    return data_[delta + gap_size_];
  }
  return data_[delta];
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator*() const
    -> reference {
  return *data_;
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] auto gap_buffer<Alloc>::iterator_impl<IsConst>::operator->() const
    -> pointer {
  return data_;
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] bool gap_buffer<Alloc>::iterator_impl<IsConst>::operator==(
    const iterator_impl& other) const {
  return (data_ == other.data_ && gap_size_ == other.gap_size_ &&
          gap_index_ == other.gap_index_);
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] std::strong_ordering
gap_buffer<Alloc>::iterator_impl<IsConst>::operator<=>(
    const iterator_impl& other) const {
  return data_ <=> other.data_;
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] int gap_buffer<Alloc>::iterator_impl<IsConst>::get_gap_index()
    const {
  return gap_index_;
}

template <typename Alloc>
template <bool IsConst>
[[nodiscard]] int gap_buffer<Alloc>::iterator_impl<IsConst>::get_gap_size()
    const {
  return gap_size_;
}

template <typename Alloc>
gap_buffer<Alloc>::iterator::difference_type operator-(
    const typename gap_buffer<Alloc>::const_iterator& lhs,
    const typename gap_buffer<Alloc>::const_iterator& rhs) {
  auto logical_pos = [](const auto& it) {
    auto pos = -it.get_gap_index();
    if (it.get_gap_index() < 0) pos -= it.get_gap_size();
    return pos;
  };

  return logical_pos(lhs) - logical_pos(rhs);
}

gap_buffer<>::iterator::difference_type operator-(
    const typename gap_buffer<>::const_iterator& lhs,
    const typename gap_buffer<>::const_iterator& rhs) {
  auto logical_pos = [](const auto& it) {
    auto pos = -it.get_gap_index();
    if (it.get_gap_index() < 0) pos -= it.get_gap_size();
    return pos;
  };

  return logical_pos(lhs) - logical_pos(rhs);
}
