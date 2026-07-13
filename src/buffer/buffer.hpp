#pragma once
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

template <typename Alloc>
concept Allocator =
    requires(Alloc a, typename Alloc::value_type* p, std::size_t n) {
      typename Alloc::value_type;
      { a.allocate(n) } -> std::same_as<typename Alloc::value_type*>;
      a.deallocate(p, n);
      { Alloc(a) } -> std::same_as<Alloc>;
      { a == a } -> std::convertible_to<bool>;
    };

template <typename Alloc = std::allocator<char>>
class gap_buffer {
 private:
  using ReboundAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<char>;
  using alloc_traits = std::allocator_traits<ReboundAlloc>;
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
  gap_buffer(Alloc allocator)
    requires Allocator<Alloc>;
  template <typename Generator>
  void fill_construct(std::size_t size, Generator generator);
  gap_buffer(std::size_t size, Alloc allocator = Alloc());
  gap_buffer(std::size_t size, char character, Alloc allocator = Alloc());
  gap_buffer(const std::string& text, Alloc allocator = Alloc());
  gap_buffer(const char* text, Alloc allocator = Alloc());
  gap_buffer(std::string&& text, Alloc allocator = Alloc());
  gap_buffer(const gap_buffer& other);
  gap_buffer(gap_buffer&& other) noexcept;

  void swap(gap_buffer& other);
  gap_buffer& operator=(gap_buffer other);

  ~gap_buffer();

  void clear();

  void resize(std::size_t new_size, char character);

  void destroy_old_storage();

  void reserve(std::size_t new_capacity);
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

  iterator insert(char character);
  iterator insert(const std::string& text);
  iterator insert(std::string&& text);
  iterator insert(const char* text);
  iterator erase_in_front_of_cursor();
  iterator erase_in_back_of_cursor();

  void step(int delta);

  [[nodiscard]] char& operator[](std::size_t index);
  [[nodiscard]] char& at(std::size_t index);
  [[nodiscard]] const char& operator[](std::size_t index) const;
  [[nodiscard]] const char& at(std::size_t index) const;

  [[nodiscard]] std::size_t get_cursor_position_index() const;
  [[nodiscard]] const char* get_data() const;

 private:
  char* choose_buffer();
  const char* choose_buffer() const;
  void construct(std::size_t index, char character);
  void destroy(std::size_t index);

  static const std::size_t kSSOBufferSize = 16;

  union DataUnion {
    char stack_buffer[kSSOBufferSize];
    char* heap_buffer;

    DataUnion() : stack_buffer{} {}
  } data_;
  std::size_t size_{0};
  std::size_t capacity_{kSSOBufferSize};
  int gap_index_{0};
  [[no_unique_address]] ReboundAlloc allocator_{ReboundAlloc()};
};

template <typename Alloc>
[[nodiscard]] bool operator==(const gap_buffer<Alloc>& lhs,
                              const gap_buffer<Alloc>& rhs);

template <typename Alloc>
[[nodiscard]] bool operator!=(const gap_buffer<Alloc>& lhs,
                              const gap_buffer<Alloc>& rhs);

template <typename Alloc>
template <bool IsConst>
class gap_buffer<Alloc>::iterator_impl {
 public:
  using gap_buffer_iterator_tag = void;
  using value_type = std::conditional_t<IsConst, const char, char>;
  using pointer = std::conditional_t<IsConst, const char*, char*>;
  using iterator_category = std::random_access_iterator_tag;
  using reference = std::conditional_t<IsConst, const char&, char&>;
  using difference_type = std::ptrdiff_t;

  iterator_impl();
  iterator_impl(pointer data, std::size_t gap_index, std::size_t gap_size);

  iterator_impl(const iterator_impl& other);
  iterator_impl(iterator_impl&& other) noexcept;

  iterator_impl& operator=(iterator_impl other);
  void swap(iterator_impl& other);

  ~iterator_impl();

  operator iterator_impl<true>() const;

  iterator_impl& operator++();
  [[nodiscard]] iterator_impl operator++(int);

  iterator_impl& operator--();
  [[nodiscard]] iterator_impl operator--(int);

  iterator_impl& operator+=(difference_type delta);
  iterator_impl& operator-=(difference_type delta);
  [[nodiscard]] iterator_impl operator+(difference_type delta) const;
  [[nodiscard]] iterator_impl operator-(difference_type delta) const;

  [[nodiscard]] reference operator[](difference_type index) const;

  [[nodiscard]] reference operator*() const;
  [[nodiscard]] pointer operator->() const;

  [[nodiscard]] std::strong_ordering operator<=>(
      const iterator_impl& other) const;
  [[nodiscard]] bool operator==(const iterator_impl& other) const;

  [[nodiscard]] int get_gap_index() const;
  [[nodiscard]] int get_gap_size() const;

 private:
  pointer data_{nullptr};
  int gap_index_{0};
  int gap_size_{0};
};

template <typename T>
concept is_gap_buffer_iterator =
    requires { typename T::gap_buffer_iterator_tag; };

template <typename IterL, typename IterR>
  requires is_gap_buffer_iterator<IterL> && is_gap_buffer_iterator<IterR>
auto operator-(const IterL& lhs, const IterR& rhs)
    -> decltype(lhs.get_gap_index() - rhs.get_gap_index());

gap_buffer() -> gap_buffer<>;
gap_buffer(std::size_t) -> gap_buffer<>;
gap_buffer(std::size_t, char) -> gap_buffer<>;
gap_buffer(const std::string&) -> gap_buffer<>;
gap_buffer(std::string&&) -> gap_buffer<>;
gap_buffer(const char*) -> gap_buffer<>;

#include "buffer.cpp"
