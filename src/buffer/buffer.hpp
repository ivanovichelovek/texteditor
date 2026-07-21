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

template <typename T, Allocator Alloc = std::allocator<T>>
class gap_buffer {
 private:
  using ReboundAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
  using alloc_traits = std::allocator_traits<ReboundAlloc>;
  template <bool>
  class iterator_impl;

 public:
  using value_type = T;
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
  gap_buffer(Alloc allocator);
  template <typename Generator>
  void fill_construct(std::size_t size, Generator generator);
  gap_buffer(std::size_t size, Alloc allocator = Alloc());
  gap_buffer(std::size_t size, T value, Alloc allocator = Alloc());
  gap_buffer(const std::string& text, Alloc allocator = Alloc())
    requires std::same_as<T, char>;
  gap_buffer(const T* text, Alloc allocator = Alloc())
    requires std::same_as<T, char>;
  gap_buffer(std::string&& text, Alloc allocator = Alloc())
    requires std::same_as<T, char>;
  gap_buffer(const gap_buffer& other);
  gap_buffer(gap_buffer&& other) noexcept
    requires std::is_nothrow_move_constructible_v<T>;

  void swap(gap_buffer& other);
  gap_buffer& operator=(gap_buffer other);

  ~gap_buffer();

  void clear();

  void resize(std::size_t new_size, T value);

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

  [[nodiscard]] std::string to_string() const
    requires std::same_as<T, char>;

  iterator insert(T value);
  iterator insert(const std::string& text)
    requires std::same_as<T, char>;
  iterator insert(std::string&& text)
    requires std::same_as<T, char>;
  iterator insert(const T* text)
    requires std::same_as<T, char>;
  iterator erase_in_front_of_cursor();
  iterator erase_in_back_of_cursor();

  void step(int delta);

  [[nodiscard]] T& operator[](std::size_t index);
  [[nodiscard]] T& at(std::size_t index);
  [[nodiscard]] const T& operator[](std::size_t index) const;
  [[nodiscard]] const T& at(std::size_t index) const;

  [[nodiscard]] std::size_t get_cursor_position_index() const;
  [[nodiscard]] const T* get_data() const;

 private:
  T* choose_buffer();
  const T* choose_buffer() const;
  void construct(std::size_t index, T value);
  void destroy(std::size_t index);
  // Destroy the live (real) elements of *this in gap layout, without freeing
  // storage. Works for both the stack and the heap buffer.
  void destroy_elements();
  // Move-construct the live elements of src (in gap layout for the given
  // size/gap_index/capacity) into dst at the same raw indices, destroying the
  // sources. Used by swap to shuffle elements between raw buffers.
  static void relocate(ReboundAlloc& alloc, T* src, T* dst, std::size_t size,
                       int gap_index, std::size_t capacity);

  static const std::size_t kSSOBufferSize = 16;

  // Raw storage: gap_buffer manages element lifetimes explicitly (only the
  // "real" elements in gap layout are ever alive), so the union neither
  // constructs nor destroys its members.
  union DataUnion {
    T stack_buffer[kSSOBufferSize];
    T* heap_buffer;

    DataUnion() {}
    ~DataUnion() {}
  } data_;
  std::size_t size_{0};
  std::size_t capacity_{kSSOBufferSize};
  int gap_index_{0};
  [[no_unique_address]] ReboundAlloc allocator_{ReboundAlloc()};
};

template <typename T, Allocator Alloc>
[[nodiscard]] bool operator==(const gap_buffer<T, Alloc>& lhs,
                              const gap_buffer<T, Alloc>& rhs);

template <typename T, Allocator Alloc>
[[nodiscard]] bool operator!=(const gap_buffer<T, Alloc>& lhs,
                              const gap_buffer<T, Alloc>& rhs);

template <typename T, Allocator Alloc>
template <bool IsConst>
class gap_buffer<T, Alloc>::iterator_impl {
 public:
  using gap_buffer_iterator_tag = void;
  using value_type = std::conditional_t<IsConst, const T, T>;
  using pointer = std::conditional_t<IsConst, const T*, T*>;
  using iterator_category = std::random_access_iterator_tag;
  using reference = std::conditional_t<IsConst, const T&, T&>;
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

#include "buffer.cpp"
