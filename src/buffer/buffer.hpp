#pragma once
#include <compare>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

template <typename Alloc = std::allocator<char>>
class gap_buffer {
 private:
  using alloc_traits = std::allocator_traits<Alloc>;
  template <bool>
  class iterator_impl;

 public:
  using value_type = char;
  using allocator_type = Alloc;
  using size_type = std::size_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = iterator_impl<false>;
  using const_iterator = iterator_impl<true>;
  using reverse_iterator = std::reverse_iterator<iterator_impl<false>>;
  using const_reverse_iterator = std::reverse_iterator<iterator_impl<true>>;

  gap_buffer();
  gap_buffer(std::size_t);
  gap_buffer(std::size_t, char);
  gap_buffer(const std::string&);
  gap_buffer(const char*);
  gap_buffer(std::string&&);
  gap_buffer(const gap_buffer&);
  gap_buffer(gap_buffer&&);

  gap_buffer(Alloc);
  gap_buffer(std::size_t, Alloc);
  gap_buffer(std::size_t, char, Alloc);
  gap_buffer(const std::string&, Alloc);
  gap_buffer(const char*, Alloc);
  gap_buffer(std::string&&, Alloc);
  gap_buffer(const gap_buffer&, Alloc);
  gap_buffer(gap_buffer&&, Alloc);

  void swap(gap_buffer&);
  gap_buffer& operator=(gap_buffer);

  ~gap_buffer();

  void clear();

  void resize(std::size_t, char);

  void reserve(std::size_t);
  void shrink_to_fit();

  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::size_t capacity() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] iterator begin();
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator cbegin() const;

  [[nodiscard]] iterator end();
  [[nodiscard]] const_iterator end() const;
  [[nodiscard]] const_iterator cend() const;

  [[nodiscard]] reverse_iterator rbegin();
  [[nodiscard]] const_reverse_iterator rbegin() const;
  [[nodiscard]] const_reverse_iterator crbegin() const;

  [[nodiscard]] reverse_iterator rend();
  [[nodiscard]] const_reverse_iterator rend() const;
  [[nodiscard]] const_reverse_iterator crend() const;

  [[nodiscard]] std::string to_string() const;

  iterator insert(char);
  iterator erase_in_front_of_cursor();
  iterator erase_in_back_of_cursor();

  iterator step(int);

  [[nodiscard]] char& operator[](std::size_t);
  [[nodiscard]] char& at(std::size_t);
  [[nodiscard]] const char& operator[](std::size_t) const;
  [[nodiscard]] const char& at(std::size_t) const;

  [[nodiscard]] bool operator==(const gap_buffer&) const;
  [[nodiscard]] bool operator!=(const gap_buffer&) const;

  [[nodiscard]] std::size_t GetCursorPositionIndex() const;

 private:
  char* choose_buffer();

  static const std::size_t kSSOBufferSize = 16;

  union {
    char stack_buffer[kSSOBufferSize];
    char* heap_buffer{nullptr};
  } data_;
  std::size_t size_{0};
  std::size_t capacity_{16};
  char* gap_begin_{data_.stack_buffer};
  Alloc allocator_{Alloc()};
};

template <typename Alloc>
template <bool IsConst>
class gap_buffer<Alloc>::iterator_impl {
 public:
  using value_type = std::conditional_t<IsConst, const char, char>;
  using pointer = std::conditional_t<IsConst, const char*, char*>;
  using iterator_category = std::random_access_iterator_tag;
  using reference = std::conditional_t<IsConst, const char&, char&>;
  using difference_type = std::ptrdiff_t;

  iterator_impl();
  iterator_impl(char*);
  iterator_impl(char*, char*, std::size_t);

  operator iterator_impl<true>() const;

  iterator_impl& operator++();
  iterator_impl operator++(int);

  iterator_impl& operator--();
  iterator_impl operator--(int);

  iterator_impl& operator+=(difference_type);
  iterator_impl& operator-=(difference_type);
  iterator_impl operator+(difference_type) const;
  iterator_impl operator-(difference_type) const;
  difference_type operator-(const iterator_impl&) const;

  reference operator[](difference_type) const;

  reference operator*() const;
  pointer operator->() const;

  std::strong_ordering operator<=>(const iterator_impl) const;
  bool operator==(const iterator_impl) const;

 private:
  pointer data_;
  pointer gap_begin_;
  std::size_t gap_size_;
};
