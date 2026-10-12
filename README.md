# field-reflection

Header-only, macro-free reflection of C++20 aggregates and enums, with a C++26 static reflection ([P2996](https://wg21.link/p2996)) backend behind the same API.

[![CI](https://github.com/yosh-matsuda/field-reflection/actions/workflows/tests.yml/badge.svg)](https://github.com/yosh-matsuda/field-reflection/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![C++26 reflection](https://img.shields.io/badge/C%2B%2B26-P2996-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-single%20file-green.svg)

field-reflection gives you the number, names, types, and values of the fields of a plain struct, and the names of enumerators, without macros or registration. It is a small building block for serializers, loggers, test utilities, and the like; for example, [cpp-yyjson](https://github.com/yosh-matsuda/cpp-yyjson) uses it to convert structs to and from JSON. With a compiler that implements C++26 static reflection, the same API is implemented with `std::meta`, which removes the limits of the C++20 backend and adds annotations.

## Quick start

```cpp
#include <field_reflection.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

using namespace field_reflection;

struct user
{
    int id = 42;
    std::string name = "Alice";
    double score = 3.14;
};

enum class color { red, green, blue };

static_assert(field_count<user> == 3);
static_assert(field_name<user, 1> == "name");
static_assert(std::is_same_v<field_type<user, 2>, double>);

static_assert(enum_name(color::green) == "green");
static_assert(enum_cast<color>("blue") == color::blue);

int main()
{
    auto u = user{};
    get_field<0>(u) = 7;  // u.id = 7

    // id: 7
    // name: Alice
    // score: 3.14
    for_each_field(u, [](std::string_view name, const auto& value) { std::cout << name << ": " << value << '\n'; });
}
```

By default, the library reflects aggregates, i.e., structs and classes without user-declared constructors, virtual functions, and private or protected direct non-static data members. See [Concepts](#concepts) for the exact conditions and [`reflect_options`](#reflect_options) to reflect other classes with the C++26 backend.

## Features

* Fields of aggregates: count, names, types, and values, with no macros or registration
* Names and values of enumerators: enum to string and string to enum
* Usable in constant expressions
* Header-only single file with no dependencies
* Two backends with the same API: a C++20 backend, and a C++26 backend based on static reflection ([P2996](https://wg21.link/p2996)), which is selected automatically when available
* With the C++26 backend: base classes, any number of fields, all enumerators, annotations ([P3394](https://wg21.link/p3394)), and opt-in reflection of non-aggregate classes, private members, and bit-fields

## Why field-reflection?

field-reflection is for generic code that receives structs and enums it does not know in advance, such as serializers, loggers, and test utilities.

* **Nothing to register.** Plain structs and enums work as they are, including types from other libraries.
* **Fields and enums from one header.** A function that walks the fields of a struct can also write its enum members by name (see [A tiny JSON writer](#a-tiny-json-writer)).
* **Concepts to branch on.** Each function is constrained by a concept, such as `field_namable<T>`, so generic code can test a type with `if constexpr` and choose another conversion.
* **One API for C++20 and C++26.** With a compiler that implements static reflection, the same calls use `std::meta` and also reflect base classes, more than 100 fields, all enumerators, and annotations. [Differences between the backends](#differences-between-the-backends) lists where the results differ.

For example, the following function prints the fields that differ between two objects of any aggregate type, such as `user` in [Quick start](#quick-start):

```cpp
template <typename T>
void print_changes(const T& before, const T& after)
{
    for_each_field(before, after, [](std::string_view name, const auto& a, const auto& b) {
        if (a != b) std::cout << name << ": " << a << " -> " << b << '\n';
    });
}

auto u = user{};
auto v = u;
v.score = 2.5;
print_changes(u, v);  // score: 3.14 -> 2.5
```

Similar libraries reflect either fields or enums:

| | field-reflection | [Boost.PFR](https://github.com/boostorg/pfr) | [magic_enum](https://github.com/Neargye/magic_enum) | [enchantum](https://github.com/ZXShady/enchantum) |
| --- | --- | --- | --- | --- |
| Fields of structs | count, names, types, values | count, names (C++20), types, values | – | – |
| Enumerators | names and values | – | names and values | names and values |
| Minimum standard | C++20 | C++14 | C++17 | C++17 |
| C++26 static reflection | automatic, in a release | experimental, opt-in | on the master branch | automatic |

All four libraries are header-only and need no registration. The table is based on Boost 1.92, magic_enum v0.9.8, and enchantum at commit `6ada6cb` (October 2026). Corrections are welcome.

field-reflection is not a serialization library. For JSON, see [cpp-yyjson](https://github.com/yosh-matsuda/cpp-yyjson), which uses field-reflection to convert structs.

### Performance

* **Run time.** At `-O2`, `for_each_field` and `all_of_field` compile to the same instructions as hand-written member access (up to the order of independent instructions). `enum_name` finds a name by an index if the values of the enumerators are consecutive, and by a binary search otherwise.
* **Compile time.** A struct with 8 fields adds 19–23 ms, about the same as Boost.PFR. An enum with 10 enumerators adds 9–14 ms, less than enchantum (16–18 ms) and magic_enum (17–102 ms). Including the header takes 0.23–0.34 s.
* **Binary size.** A program contains only the names that it uses, and no function signatures from the reflection machinery. A struct adds about the same size as with Boost.PFR. An enum with 10 enumerators adds 221–271 B, less than enchantum (375–525 B) and magic_enum (736–857 B).

The results are for GCC 15 and Clang 22 in C++20, and GCC 16 in C++26 with `-freflection`, on x86-64 Linux.

<details>
<summary>Measurement details</summary>

The compile time added per type is the slope between 10 and 200 types in one translation unit (`-c -O0`, best of 3):

| Added compile time per type | GCC 15, C++20 | Clang 22, C++20 | GCC 16, C++26 with `-freflection` |
| --- | ---: | ---: | ---: |
| Struct with 8 fields, `for_each_field` with names | 23 ms | 19 ms | 20 ms |
| &nbsp;&nbsp;Boost.PFR `for_each_field_with_name` | 22 ms | 27 ms | 18 ms |
| Enum with 10 enumerators, `enum_name` and `enum_cast` | 14 ms | 14 ms | 9 ms |
| &nbsp;&nbsp;magic_enum | 68 ms | 102 ms | 17 ms |
| &nbsp;&nbsp;enchantum | 16 ms | 17 ms | 18 ms |

For example, 100 such structs add about 2 seconds. Including the header takes 0.23 s with GCC 15, 0.30 s with Clang 22, and 0.34 s with GCC 16 and `-freflection`. Without static reflection, field-reflection and enchantum search the enumerators in [-128, 128], and magic_enum in [-128, 127]; enchantum takes 19–21 ms per enum with its default range [-256, 256].

The binary size added per type is the sum of the sections for code, data, unwind information, and relocations (`-O2`, stripped):

| Added size per type | GCC 15, C++20 | Clang 22, C++20 | GCC 16, C++26 with `-freflection` |
| --- | ---: | ---: | ---: |
| Struct with 8 fields, `for_each_field` with names | 368 B | 616 B | 357 B |
| &nbsp;&nbsp;Boost.PFR `for_each_field_with_name` | 371 B | 616 B | 350 B |
| Enum with 10 enumerators, `enum_name` and `enum_cast` | 271 B | 244 B | 221 B |
| &nbsp;&nbsp;magic_enum | 736 B | 857 B | 829 B |
| &nbsp;&nbsp;enchantum | 440 B | 375 B | 525 B |

The libraries are Boost.PFR at commit `562a0cb`, magic_enum at commit `5de969c`, which uses static reflection with GCC 16, and enchantum at commit `6ada6cb`. Clang 22 uses libstdc++ 15.

</details>

## Requirements

| Compiler | C++20 backend | C++26 backend |
| --- | --- | --- |
| GCC | 11 or later | 16 or later, with `-std=c++26 -freflection` |
| Clang | 15 or later | – |
| MSVC | 19.37 (Visual Studio 2022 17.7) or later | – |
| clang-cl | 17 or later | – |

* With Clang and libc++, libc++ 16 or later is required.
* With MSVC, the standard-conforming preprocessor (`/Zc:preprocessor`) is required. The CMake target adds it automatically; add it yourself if you use the header without CMake.

The C++20 backend reflects at most 100 fields, does not reflect classes with base classes, and finds the enumerators only in the range [-128, 128] by default. See [Known limitations of the C++20 backend](#known-limitations-of-the-c20-backend) and [Backends](#backends).

## Installation

This library is header-only. Copy `include/field_reflection.hpp` into your include path, or use the CMake target `field_reflection::field_reflection`.

```cmake
# as a subdirectory
add_subdirectory(field-reflection)
target_link_libraries(your_target PRIVATE field_reflection::field_reflection)

# with FetchContent
include(FetchContent)
FetchContent_Declare(
  field_reflection
  GIT_REPOSITORY https://github.com/yosh-matsuda/field-reflection.git
  GIT_TAG v0.4.0)
FetchContent_MakeAvailable(field_reflection)
target_link_libraries(your_target PRIVATE field_reflection::field_reflection)

# as an installed package
find_package(field_reflection CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE field_reflection::field_reflection)
```

## Backends

field-reflection has two implementations (backends) with the same API:

* The C++20 backend uses aggregate initialization and parses `__PRETTY_FUNCTION__` (or `__FUNCSIG__`).
* The C++26 backend uses the static reflection of [P2996](https://wg21.link/p2996).

The C++26 backend is selected automatically when the compiler defines both `__cpp_impl_reflection` and `__cpp_lib_reflection` (e.g., GCC 16 with `-std=c++26 -freflection`). It has no limit on the number of fields, reflects base classes and more types, and lists all enumerators.

### Differences between the backends

The differences are described with the three [concepts](#concepts) that tell what the library can do with a type. Each concept includes the previous one:

| Concept | What you can get |
| --- | --- |
| `field_countable<T>` | the number of fields |
| `field_referenceable<T>` | also the types and values of the fields |
| `field_namable<T>` | also the names of the fields |

For example, an aggregate with bit-fields is only `field_countable` by default, because a bit-field cannot be bound to a reference, and an aggregate with an anonymous union is not `field_namable`, because the union has no name.

A type that satisfies a concept with the C++20 backend satisfies it with the C++26 backend, and the number, types, and names of the fields are the same. The exceptions are:

* a class whose first base class is constructible from an argument of any type, which the C++20 backend cannot distinguish from a class without bases;
* compiler extensions such as flexible array members.

The C++26 backend reflects more types and finds more enumerators. Generic code that checks the concepts, e.g., `if constexpr (field_namable<T>)`, may take a different branch for such types.

<details>
<summary>Results for typical types</summary>

| Type shape | C++20 backend | C++26 backend |
| --- | --- | --- |
| Aggregate with ordinary data members | `field_namable` | `field_namable` |
| Empty aggregate (including `std::monostate`) | `field_namable` | `field_namable` |
| Aggregate with C array data members | `field_namable` | `field_namable` |
| Aggregate with more than 100 fields | none | `field_namable` |
| Aggregate with lvalue reference data members | `field_referenceable` | `field_namable` |
| Aggregate with rvalue reference data members | none | `field_namable` |
| Aggregate with `volatile` data members | `field_referenceable` on GCC, `field_namable` otherwise | `field_namable` |
| Aggregate with a member whose type rejects arguments of arbitrary types (e.g., a deleted constructor template) | none | `field_namable` |
| Aggregate with base classes | none | `field_namable`; the fields of the bases come first in declaration order |
| Aggregate with a non-aggregate base class | none | none (`field_namable` with `non_aggregate`) |
| Aggregate with an anonymous union | `field_countable` (none with GCC 11 and 12) | `field_referenceable`; the anonymous member is a single field without a name |
| Aggregate with bit-fields | `field_countable` (compile error with Clang 15) | `field_countable` (`field_namable` with `bit_fields`) |
| Non-aggregate class | none | none (`field_namable` with `non_aggregate`) |
| Union, C array, tuple-like type, non-class type | none | none |

"none" means that the type satisfies no concept. See [Concepts](#concepts) for the conditions on base classes.

The enum API also differs:

| Enumeration | C++20 backend | C++26 backend |
| --- | --- | --- |
| Enumerator out of [`enum_range<E>`](#enum-names-and-values) | no name | named |
| `enum_cast` with the name of an alias (an enumerator with the same value as a previous one) | `std::nullopt` | the value |

</details>

Use the same compiler flags (and the same [configuration macros](#configuration-macros)) in all translation units of a program. Otherwise, templates using this library may be defined differently in different translation units, which violates the one-definition rule. The two backends are defined in different inline namespaces to avoid a conflict in this library itself.

### Configuration macros

| Macro | Description |
| --- | --- |
| `FIELD_RFL_DISABLE_STD_REFLECTION` | Define it as empty or a nonzero value before including the header to use the C++20 backend even if C++26 reflection is available. Defining it as `0` keeps the C++26 backend. |
| `FIELD_RFL_STD_REFLECTION` | Defined by the header to `1` if the C++26 backend is used, and to `0` otherwise. Do not define it yourself. |
| `FIELD_RFL_ANNOTATIONS` | Defined by the header to `1` if the [annotation API](#annotations-c26-backend-only) is available, and to `0` otherwise. |

## API Reference

All entities are declared in the namespace `field_reflection`.

* [Concepts](#concepts) and [`reflect_options`](#reflect_options)
* Fields: [`field_count`](#field_count), [`field_name`](#field_name), [`field_type`](#field_type), [`get_field`](#get_field), [`set_field`](#set_field), [`for_each_field`, `all_of_field`, `any_of_field`](#for_each_field-all_of_field-any_of_field), [`to_tuple`](#to_tuple)
* Types: [`type_name`](#type_name)
* Enumerations: [`enum_count`, `enum_values`, `enum_names`, `enum_name`, `enum_cast`, `enum_range`](#enum-names-and-values)
* Annotations: [`field_annotations`, `field_annotation`, `type_annotations`, `type_annotation`](#annotations-c26-backend-only)

### Concepts

```cpp
template <typename T, reflect_options Opts = reflect_options{}>
concept field_countable;
template <typename T, reflect_options Opts = reflect_options{}>
concept field_referenceable;
template <typename T, reflect_options Opts = reflect_options{}>
concept field_namable;
```

The concepts are three levels of reflection. Each concept includes the previous one, i.e., `field_namable<T>` implies `field_referenceable<T>`, which implies `field_countable<T>`. Check the concept for the functions that you call: e.g., `field_referenceable` to compare or copy the values of the fields, and `field_namable` to print the names. Since a more specific concept takes priority in overload resolution, a function constrained with `field_namable` can be overloaded with one constrained with `field_referenceable` as a fallback for types without names.

| Concept | Enables | Additional requirements |
| --- | --- | --- |
| `field_countable` | `field_count` | `T` is an [aggregate](https://en.cppreference.com/w/cpp/types/is_aggregate) class type that is neither a union nor a tuple-like type (a type for which `std::tuple_size<T>` is defined, e.g., `std::array`). |
| `field_referenceable` | `field_type`, `get_field`, `set_field`, `to_tuple`, and the visitors | `T` has no bit-field. |
| `field_namable` | `field_name` and the visitors with names | All fields have distinct names, i.e., no anonymous member and no field of a base class hidden by a field with the same name. |

The backends add the following conditions:

* C++20 backend: `T` has no base class, and the number of fields is at most `min(100, sizeof(T) * CHAR_BIT)`, where each element of a C array data member counts as a field. `field_referenceable` also requires no anonymous union, and `field_namable` also requires no reference member (and no `volatile` member on GCC).
* C++26 backend: all base classes recursively satisfy the same conditions and are neither duplicated nor virtual. The [options](#reflect_options) allow non-aggregate classes, private and protected members, and bit-fields.

See [Differences between the backends](#differences-between-the-backends) for the results for typical types.

### `reflect_options`

```cpp
struct reflect_options
{
    bool non_aggregate = false;  // allow non-aggregate classes
    bool non_public = false;     // include private and protected members and bases
    bool bit_fields = false;     // allow bit-fields (accessed by value)
};
```

The options extend the set of reflected types. Each option only adds types and does not change the results for the types reflected by default. The options take effect only with the C++26 backend. The C++20 backend ignores them, so generic code can pass the options regardless of the backend.

* `non_aggregate`: Allow non-aggregate classes, e.g., classes with user-declared constructors. A class with private or protected data members or bases (directly or in its bases) is reflected only if `non_public` is also enabled. It is not reflected with only its public members, because they would not represent its whole value.
* `non_public`: Include private and protected data members and bases. This has an effect only together with `non_aggregate`, because an aggregate has no private or protected direct data members and its bases with such members are not aggregates. Note that this bypasses the access control, and modifying such members may break the invariants of the class.
* `bit_fields`: Allow bit-fields. Since a bit-field cannot be referenced, `get_field` returns its value, and `set_field` assigns a value to it. The visitor functions, such as `for_each_field`, pass a copy of a bit-field as an lvalue and assign it back to the bit-field when the visitor returns (see [`for_each_field`](#for_each_field-all_of_field-any_of_field)).

Every API that takes the type `T` has a template parameter for the options, e.g., `field_count<T, Opts>` and `field_namable<T, Opts>`. The functions have overloads with and without the options. The overloads without options take the same template parameters as v0.3, e.g., `get_field<N, T>(t)`, and the overloads with options take `Opts` as the second template parameter, e.g., `get_field<N, opts>(t)`. Use the same options for the concepts and the API.

Example (C++26 backend only):

```cpp
class point
{
    int x_ = 0, y_ = 0;
public:
    point(int x, int y) : x_(x), y_(y) {}
};

constexpr auto opts = reflect_options{.non_aggregate = true, .non_public = true};
static_assert(!field_namable<point>);
static_assert(field_namable<point, opts>);
static_assert(field_name<point, 0, opts> == "x_");

auto p = point{1, 2};
for_each_field<opts>(p, [](std::string_view name, auto& value) { /* ... */ });
```

<details>
<summary>Why are only aggregates reflected by default?</summary>

The members of an aggregate are its value: an aggregate is initialized directly from the list of its members, and it has no user-declared constructors, no virtual functions, and no private or protected direct members. Reading and writing the members one by one is therefore a faithful way to serialize it. A non-aggregate class does not have this guarantee:

* Its author defines how to construct it. For example, `struct meters { double value; meters(double v); };` is meant to be constructed from a number, not from `{"value": ...}`.
* Many non-aggregate classes are not data, e.g., lambdas, function objects, and `std::allocator<T>`. They would be reflected as empty structs.

Enable `non_aggregate` only for the types whose members represent their values.

</details>

### `field_count`

```cpp
template <typename T, reflect_options Opts = reflect_options{}>
requires field_countable<T, Opts>
constexpr std::size_t field_count;
```

Get the number of fields of the `field_countable` type `T`.

### `field_name`

```cpp
template <typename T, std::size_t N, reflect_options Opts = reflect_options{}>
requires field_namable<T, Opts> && (N < field_count<T, Opts>)
constexpr std::string_view field_name;
```

Get the name of the `N`-th field of the `field_namable` type `T`.

### `field_type`

```cpp
template <typename T, std::size_t N, reflect_options Opts = reflect_options{}>
requires field_referenceable<T, Opts> && (N < field_count<T, Opts>)
using field_type;
```

Get the type of the `N`-th field of the `field_referenceable` type `T`. If `T` is cv-qualified, the field type has the same cv-qualifiers unless it is a reference or a `mutable` member.

This operation inspects only the field type. It does not extract the field value or convert the object to a tuple, so it can be used in cases where value-based operations are not available. Examples include aggregates with non-copyable or non-movable fields, and packed aggregates (e.g., with `__attribute__((packed))`) whose fields cannot be referenced as ordinary lvalues.

### `get_field`

```cpp
// U = std::remove_cvref_t<T>

// lvalue reference (const or non-const)
template <std::size_t N, typename T>
requires field_referenceable<U> && (N < field_count<U>)
constexpr decltype(auto) get_field(T& t) noexcept;
template <std::size_t N, reflect_options Opts, typename T>
requires field_referenceable<U, Opts> && (N < field_count<U, Opts>)
constexpr decltype(auto) get_field(T& t) noexcept;

// rvalue reference
template <std::size_t N, typename T>
requires field_referenceable<U> && (N < field_count<U>) && /* the field is not a C array */
constexpr auto get_field(T&& t) noexcept(/* see below */);
template <std::size_t N, reflect_options Opts, typename T>
requires field_referenceable<U, Opts> && (N < field_count<U, Opts>) && /* the field is not a C array */
constexpr auto get_field(T&& t) noexcept(/* see below */);
```

Get the `N`-th field of `t`. The lvalue overloads return a reference to the field (a bit-field is returned by value). The rvalue overloads return the field by value, moving it from `t` when possible. They are `noexcept` if the move (or the copy, e.g., for a `const` rvalue) of the field does not throw.

### `set_field`

```cpp
// U = std::remove_cvref_t<T>
template <std::size_t N, typename T, typename V>
requires field_referenceable<U> && (N < field_count<U>) && /* value is assignable to the field */
constexpr void set_field(T& t, V&& value);
template <std::size_t N, reflect_options Opts, typename T, typename V>
requires field_referenceable<U, Opts> && (N < field_count<U, Opts>) && /* value is assignable to the field */
constexpr void set_field(T& t, V&& value);
```

Assign `value` to the `N`-th field of `t`. This is equivalent to `get_field<N, Opts>(t) = std::forward<V>(value)` except that it also works with bit-fields.

### `for_each_field`, `all_of_field`, `any_of_field`

```cpp
// unary operation (U = std::remove_cvref_t<T>)
template <typename T, typename Func>
requires field_referenceable<U> && /* see below */
constexpr void for_each_field(T&& t, Func&& func);
template <typename T, typename Func>
requires field_referenceable<U> && /* see below */
constexpr bool all_of_field(T&& t, Func&& func);
template <typename T, typename Func>
requires field_referenceable<U> && /* see below */
constexpr bool any_of_field(T&& t, Func&& func);

// binary operation (U1 = std::remove_cvref_t<T1>, U2 = std::remove_cvref_t<T2>)
template <typename T1, typename T2, typename Func>
requires field_referenceable<U1> && std::is_same_v<U1, U2> && /* see below */
constexpr void for_each_field(T1&& t1, T2&& t2, Func&& func);
template <typename T1, typename T2, typename Func>
requires field_referenceable<U1> && std::is_same_v<U1, U2> && /* see below */
constexpr bool all_of_field(T1&& t1, T2&& t2, Func&& func);
template <typename T1, typename T2, typename Func>
requires field_referenceable<U1> && std::is_same_v<U1, U2> && /* see below */
constexpr bool any_of_field(T1&& t1, T2&& t2, Func&& func);

// with options, e.g., for_each_field<opts>(t, func)
template <reflect_options Opts, typename T, typename Func>
requires field_referenceable<U, Opts> && /* see below */
constexpr void for_each_field(T&& t, Func&& func);
// ...
```

Visit each field of `t` (or the corresponding fields of `t1` and `t2`, which may differ in cv-qualifiers and value categories) and apply `func`. The `func` must be callable with one of the following arguments for every field:

* one or two fields;
* the name of the field as `std::string_view` and one or two fields, if the type is `field_namable`.

The fields are passed as lvalues, or as rvalues if the object is an rvalue. The `for_each_field` returns `void`. The `all_of_field` and `any_of_field` require that `func` returns a value convertible to `bool`, and return whether it returns `true` for all or any of the fields, with short-circuit evaluation.

A bit-field (with the `bit_fields` option) of an lvalue is passed as a copy in a local variable. If the object is not `const`, the copy is assigned back to the bit-field when `func` returns or exits by an exception. Therefore, `func` can modify a bit-field like other fields, but the object does not see the modification until `func` returns.

For example, the following code prints the fields of the `user` object `u` in [Quick start](#quick-start):

```cpp
constexpr auto func = [](std::string_view name, const auto& value) { std::cout << name << ": " << value << '\n'; };
for_each_field(u, func);
```

The above is equivalent to:

```cpp
func(std::string_view("id"), u.id);
func(std::string_view("name"), u.name);
func(std::string_view("score"), u.score);
```

The binary operation is useful for comparing each field of two objects of the same type:

```cpp
for_each_field(u1, u2, [](std::string_view name, const auto& value1, const auto& value2) {
    if (value1 != value2) std::cout << name << " differs\n";
});
```

### `to_tuple`

```cpp
// U = std::remove_cvref_t<T>
template <typename T>
requires field_referenceable<U> && /* see below */
constexpr std::tuple<...> to_tuple(T&& t);
template <reflect_options Opts, typename T>
requires field_referenceable<U, Opts> && /* see below */
constexpr std::tuple<...> to_tuple(T&& t);
```

Convert `t` to a `std::tuple` whose elements have the same types as the fields. For example, a `user` object is converted to `std::tuple<int, std::string, double>`. The elements are copied from the fields, or moved if `t` is an rvalue, so `to_tuple(std::move(t))` works with move-only fields such as `std::unique_ptr`.

A type with C array members is not supported, as a C array cannot be a tuple element, nor is a type with anonymous members in the C++26 backend. The fields must be copyable (or movable if `t` is an rvalue). Use `get_field` or the field visitor functions instead in these cases.

### `type_name`

```cpp
template <typename T>
constexpr std::string_view type_name;
```

Get the name of the type `T`, e.g., `"my_struct"` or `"ns::my_struct"`. The exact spelling depends on the compiler, especially for templates and standard library types.

### Enum names and values

```cpp
template <typename E>
constexpr std::size_t enum_count;
template <typename E>
constexpr std::array<std::remove_cv_t<E>, enum_count<E>> enum_values;
template <typename E>
constexpr std::array<std::string_view, enum_count<E>> enum_names;

template <typename E>
constexpr std::string_view enum_name(E value) noexcept;
template <typename E>
constexpr std::optional<std::remove_cv_t<E>> enum_cast(std::string_view name) noexcept;

template <typename E>
struct enum_range
{
    static constexpr int min = -128;
    static constexpr int max = 128;
};
```

Get the enumerators of the enumeration type `E`. The `enum_values` holds the distinct values of the enumerators in ascending order, and the `enum_names` holds the corresponding names. When two or more enumerators have the same value, the name declared first is used. The `enum_name` returns the name of the enumerator whose value is equal to `value`, or an empty string if there is no such enumerator (e.g., a combination of flags). The `enum_cast` returns the value of the enumerator named `name`, or `std::nullopt` if there is no such enumerator. An enumeration declared without enumerators (e.g., `enum class E : int;`) has no enumerators.

```cpp
enum class color { red = 1, green = 2, blue = 4 };

static_assert(enum_count<color> == 3);
static_assert(enum_names<color>[1] == "green");
static_assert(enum_name(color::blue) == "blue");
static_assert(enum_name(static_cast<color>(3)).empty());
static_assert(enum_cast<color>("red") == color::red);
```

The C++26 backend lists all enumerators. The C++20 backend searches only the values in the range [`enum_range<E>::min`, `enum_range<E>::max`] (clamped to the underlying type), so the enumerators out of the range have no name, and the `enum_cast` with the name of an alias returns `std::nullopt`. Specialize `enum_range` to change the range for your enumeration type, while a wider range takes longer to compile:

```cpp
template <>
struct field_reflection::enum_range<my_enum>
{
    static constexpr int min = 0;
    static constexpr int max = 1000;
};
```

### Annotations (C++26 backend only)

```cpp
template <typename T, std::size_t N, typename A, reflect_options Opts = reflect_options{}>
requires field_countable<T, Opts> && (N < field_count<T, Opts>)
constexpr std::array<A, /* number of annotations */> field_annotations;
template <typename T, std::size_t N, typename A, reflect_options Opts = reflect_options{}>
requires field_countable<T, Opts> && (N < field_count<T, Opts>)
constexpr std::optional<A> field_annotation;

template <typename T, typename A>
constexpr std::array<A, /* number of annotations */> type_annotations;
template <typename T, typename A>
constexpr std::optional<A> type_annotation;
```

Get the annotations ([P3394](https://wg21.link/p3394)) of type `A` attached to the `N`-th field of `T` or to the declaration of `T`. The `field_annotations` and `type_annotations` hold all annotations of type `A` in declaration order, and the `field_annotation` and `type_annotation` hold the first one if any. These are available when the macro `FIELD_RFL_ANNOTATIONS` is defined to `1`.

An annotation must be a value of a structural type. To hold a string, use `std::define_static_string`:

```cpp
struct alias_name
{
    const char* name;
    consteval alias_name(std::string_view n) : name(std::define_static_string(n)) {}
};

struct [[= alias_name("User")]] user
{
    [[= alias_name("ID")]] int id;
    std::string name;
};

static_assert(std::string_view(field_annotation<user, 0, alias_name>->name) == "ID");
static_assert(!field_annotation<user, 1, alias_name>.has_value());
static_assert(std::string_view(type_annotation<user, alias_name>->name) == "User");
```

## Examples

### A tiny JSON writer

A recursive function handles nested structs, enums, and strings:

```cpp
#include <field_reflection.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace field_reflection;

template <typename T>
void write_json(std::ostream& os, const T& value)
{
    if constexpr (std::is_enum_v<T>)
        os << '"' << enum_name(value) << '"';
    else if constexpr (std::is_arithmetic_v<T>)
        os << value;
    else if constexpr (std::is_convertible_v<const T&, std::string_view>)
        os << '"' << std::string_view(value) << '"';
    else if constexpr (field_namable<T>)
    {
        os << '{';
        const char* separator = "";
        for_each_field(value, [&](std::string_view name, const auto& field) {
            os << std::exchange(separator, ",") << '"' << name << "\":";
            write_json(os, field);
        });
        os << '}';
    }
}

enum class role { member, admin };

struct account
{
    std::string name;
    role access;
};

struct team
{
    std::string name;
    account leader;
    int size;
};

int main()
{
    // {"name":"core","leader":{"name":"Alice","access":"admin"},"size":3}
    write_json(std::cout, team{"core", {"Alice", role::admin}, 3});
}
```

### Parsing an enum from a string

```cpp
#include <field_reflection.hpp>
#include <iostream>

enum class log_level { debug, info, warning, error };

int main(int argc, char* argv[])
{
    const auto level = field_reflection::enum_cast<log_level>(argc > 1 ? argv[1] : "info");
    if (!level)
    {
        std::cerr << "log level must be one of:";
        for (auto name : field_reflection::enum_names<log_level>) std::cerr << ' ' << name;
        std::cerr << '\n';
        return 1;
    }
    std::cout << "log level: " << field_reflection::enum_name(*level) << '\n';
}
```

### Formatting with `std::format` (C++23)

The following code defines a `std::formatter` for the types that opt in:

```cpp
#include <field_reflection.hpp>
#include <format>
#include <print>
#include <string_view>
#include <utility>

template <typename T>
constexpr bool reflect_format = false;

template <typename T>
requires reflect_format<T>
struct std::formatter<T>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const T& t, std::format_context& ctx) const
    {
        auto out = std::format_to(ctx.out(), "{}{{", field_reflection::type_name<T>);
        auto separator = "";
        field_reflection::for_each_field(t, [&](std::string_view name, const auto& value) {
            out = std::format_to(out, "{}.{}={}", std::exchange(separator, ", "), name, value);
        });
        return std::format_to(out, "}}");
    }
};

struct point
{
    int x;
    int y;
};

template <>
constexpr bool reflect_format<point> = true;

int main()
{
    std::println("{}", point{1, 2});  // point{.x=1, .y=2}
}
```

## Known limitations of the C++20 backend

* The number of fields is limited to `min(100, sizeof(T) * CHAR_BIT)`, where each element of a C array data member counts as a field.
* A class with base classes is not reflected.
* An aggregate with bit-fields or an anonymous union satisfies only `field_countable`. With Clang 15, evaluating `field_referenceable` for an aggregate with bit-fields results in a compile error. With GCC 11 and 12, an aggregate with an anonymous union is not `field_countable`.
* An aggregate satisfies no concept if the number of fields cannot be determined by aggregate initialization, e.g., when the type of a member has a constructor template that is deleted or constrained to reject arbitrary arguments.
* The names of reference data members cannot be obtained. With GCC, the names of `volatile` data members cannot be obtained either.
* The names of enumerators are available only in the range of [`enum_range<E>`](#enum-names-and-values).
* With Clang, no enumerator is found for a scoped enumeration declared in a class template (e.g., `S<int>::E`) unless the definition of the enumeration has been instantiated, e.g., by naming one of its enumerators, before the first use of the enum API. Since the result depends on the order of the code, declare such enumerations at namespace scope.

## License

field-reflection is distributed under the [MIT License](LICENSE).

## Acknowledgments

This project is strongly inspired by the following and stands as

* an alternative to [visit_struct](https://github.com/cbeck88/visit_struct) without macros,
* a reflection library that is a partial reimplementation of [reflect-cpp](https://github.com/getml/reflect-cpp).

The C++20 implementation of the counting field in this library is partially referenced to [Boost.PFR](https://github.com/boostorg/pfr).
