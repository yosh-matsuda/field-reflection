# field-reflection C++

Compile-time reflection for C++ to get field names and types from a struct/class.

[![CI](https://github.com/yosh-matsuda/field-reflection/actions/workflows/tests.yml/badge.svg)](https://github.com/yosh-matsuda/field-reflection/actions/workflows/tests.yml)

## Features

* compile-time reflection
* header-only single file
* no user-side macros
* no dependencies

## Requirements

C++20 compilers are required to use this library.

* GCC >= 11
* Clang >= 15
    * with libc++-16 or later
* MSVC >= 19.37
    * clang-cl >= 17

## Usage

```cpp
#include <array>
#include <cstdint>
#include <map>
#include <print>
#include <string>
#include "field_reflection.hpp"

using namespace field_reflection;

struct my_struct
{
    int i = 287;
    double d = 3.14;
    std::string hello = "Hello World";
    std::array<std::uint64_t, 3> arr = {1, 2, 3};
    std::map<std::string, int> map{{"one", 1}, {"two", 2}};
};

// get field names
constexpr auto my_struct_n0 = field_name<my_struct, 0>;  // "i"sv
constexpr auto my_struct_n1 = field_name<my_struct, 1>;  // "d"sv
constexpr auto my_struct_n2 = field_name<my_struct, 2>;  // "hello"sv
constexpr auto my_struct_n3 = field_name<my_struct, 3>;  // "arr"sv
constexpr auto my_struct_n4 = field_name<my_struct, 4>;  // "map"sv

// get field types
using my_struct_t0 = field_type<my_struct, 0>;  // int
using my_struct_t1 = field_type<my_struct, 1>;  // double
using my_struct_t2 = field_type<my_struct, 2>;  // std::string
using my_struct_t3 = field_type<my_struct, 3>;  // std::array<uint64_t, 3>
using my_struct_t4 = field_type<my_struct, 4>;  // std::map<std::string, int>

// get field values with index
auto s = my_struct{};
auto& my_struct_v0 = get_field<0>(s);  // s.i
auto& my_struct_v1 = get_field<1>(s);  // s.d
auto& my_struct_v2 = get_field<2>(s);  // s.hello
auto& my_struct_v3 = get_field<3>(s);  // s.arr
auto& my_struct_v4 = get_field<4>(s);  // s.map

// visit each field
for_each_field(s, [](std::string_view field, auto& value) {
    // i: 287
    // d: 3.14
    // hello: Hello World
    // arr: [1, 2, 3]
    // map: {"one": 1, "two": 2}
    std::println("{}: {}", field, value);
});
```

## API References

### Concepts

```cpp
template<typename T>
concept field_countable;
template<typename T>
concept field_referenceable;
template<typename T>
concept field_namable;
```

The `field_countable` is a concept that checks if the type `T` is a field-countable struct. Internally, it is equivalent to that `T` is [aggregate type](https://en.cppreference.com/w/cpp/types/is_aggregate) and the number of the field is less than or equal to `100`.

The `field_referenceable` is a concept that checks if a field of the type `T` can be referenced by index. This includes the `field_countable` concept. The implementation of the `field_referenceable` concept is the condition that the `field_countable` type `T` has no base class.

The `field_namable` is a concept that checks if a field name of the type `T` can be obtained by index statically. This includes the `field_referenceable` concept and also requires that the type `T` has a field and (practically) there is no reference type member.

### `field_count`

```cpp
template <field_countable T>
constexpr std::size_t field_count;
```

Get the number of fields from the `field_countable` type `T`.

### `field_name`

```cpp
template <field_namable T, std::size_t N>
constexpr std::string_view field_name;
```

Get the name of the `N`-th field as `std::string_view` from the `field_namable` type `T`.

### `field_type`

```cpp
template <field_referenceable T, std::size_t N>
using field_type;
```

Get the type of the `N`-th field from the `field_referenceable` type `T`.

### `get_field`

```cpp
// reference
template <std::size_t N, field_referenceable T>
constexpr auto& get_field(T& t) noexcept;

// const reference
template <std::size_t N, field_referenceable T>
constexpr const auto& get_field(const T& t) noexcept;

// rvalue reference
template <std::size_t N, field_referenceable T>
constexpr auto get_field(T&& t) noexcept;
```

Extracts the `N`-th element from the `field_referenceable` type `T` and returns a reference to it. It behaves like `std::get` for `std::tuple` but returns a lvalue value instead of a rvalue reference.

### `for_each_field`, `all_of_field`, `any_of_field`

```cpp
// unary operation
template <field_referenceable T, typename Func>
void for_each_field(T&& t, Func&& func);
template <field_referenceable T, typename Func>
bool all_of_field(T&& t, Func&& func);
template <field_referenceable T, typename Func>
bool any_of_field(T&& t, Func&& func);

// binary operation
template <field_referenceable T, typename Func>
void for_each_field(T&& t1, T&& t2, Func&& func);
template <field_referenceable T, typename Func>
bool all_of_field(T&& t1, T&& t2, Func&& func);
template <field_referenceable T, typename Func>
bool any_of_field(T&& t1, T&& t2, Func&& func);
```

Visits each field of the type `T` and applies the unary or binary operation `func`. The `func` must be a callable object that takes one of the following kinds of arguments:

* Arguments of one or two references to the field for the `field_referenceable` type `T`.
* Arguments of `std::string_view` and one or two references to the field for the `field_namable` type `T`.

The `for_each_field` just applies the `func` and returns `void`, while the `all_of_field` and `any_of_field` return `bool` indicating whether all or any of the `func` returns `true`.

For example, the following code prints the field names and values of the `my_struct` `s`:

```cpp
constexpr auto func = [](std::string_view field, auto& value) {
    std::println("{}: {}", field, value);
};
for_each_field(s, func);
```

The above is equivalent to:

```cpp
func("i"sv, s.i);
func("d"sv, s.d);
func("hello"sv, s.hello);
func("arr"sv, s.arr);
func("map"sv, s.map);
```

The first argument in the definition of the `func` can be omitted if it is not needed.

The binary operation version of `for_each_field` is useful for comparing each field of two objects of the same type:

```cpp
constexpr auto func = [](std::string_view field, auto& value1, auto& value2) {
    if (value1 != value2) {
        std::println("s1 and s2 have a different value: s1.{} = {}, s2.{} = {}",
                      field, value1, field, value2);
    }
};
for_each_field(s1, s2, func);
```

### `to_tuple`

```cpp
template <field_referenceable T>
constexpr std::tuple<...> to_tuple(T&& t);
```

Copy a `field_referenceable` type `T` object and convert it to `std::tuple` where each field has the same type as `T`. For example, `my_struct` object can be converted to  object of type `std::tuple<int, double, std::string, std::array<std::uint64_t, 3>, std::map<std::string, int>>`.

## Acknowledgments

This project is strongly inspired by the following and stands as

* an alternative to [visit_struct](https://github.com/cbeck88/visit_struct) without macros,
* a reflection library that is a partial reimplementation of [reflect-cpp](https://github.com/getml/reflect-cpp).

The C++20 implementation of the counting field in this library is partially referenced to [Boost.PFR](https://github.com/boostorg/pfr).
