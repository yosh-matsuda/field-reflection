#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include "field_reflection.hpp"

// NOLINTBEGIN
using namespace field_reflection;

// Some results differ between the C++20 backend and the C++26 reflection backend because of the limitations of the
// C++20 implementation, such as base classes and reference members.
constexpr bool std_rfl = FIELD_RFL_STD_REFLECTION;

template <typename, typename>
concept test_castable = true;

template <field_referenceable T>
constexpr bool all_field_types_are_test_castable = []<std::size_t... Is>(std::index_sequence<Is...>) {
    return (test_castable<int, field_type<T, Is>> && ...);
}(std::make_index_sequence<field_count<T>>{});

struct my_struct1
{
    int i = 287;
    double d = 3.14;
    std::string hello = "Hello World";
    std::array<uint64_t, 3> arr = {1, 2, 3};
    std::map<std::string, int> map{{"one", 1}, {"two", 2}};
    bool operator==(const my_struct1&) const = default;
};

// struct with reference
struct my_struct2
{
    int& i;
    double& d;
};

// struct with const reference
struct my_struct3
{
    const int& i = 10;
    const double& d = 3.14;
};

// struct with pointer member
struct my_struct4
{
    const int* i = nullptr;
};

// nested struct
struct my_struct5
{
    my_struct1 m;
};

// derived struct
struct my_struct6 : public my_struct1
{
};

// nested derived struct
struct my_struct7 : public my_struct1
{
    struct X
    {
        int a;
        std::optional<double> b;
        std::string c;
    } x;
};

// struct with lvalue and rvalue reference
struct my_struct8
{
    int&& i;
    double& d;
};

// local struct
auto func()
{
    struct local_struct
    {
        int a;
    };
    return local_struct{};
}
using my_struct9 = decltype(func());

// struct in namespace
namespace named
{
    struct my_struct10
    {
        int x0;
        double y0;
    };
}  // namespace named

// struct in unnamed namespace
namespace
{
    struct my_struct11
    {
        int x1;
        double y1;
    };
}  // namespace

struct my_struct12
{
    std::unique_ptr<int> value;
};

struct my_struct13
{
    const int value;
};

struct my_struct14
{
    struct non_movable
    {
        non_movable() = default;
        non_movable(const non_movable&) = delete;
        non_movable(non_movable&&) = delete;
    } value;
};

struct my_struct15
{
};

union my_union
{
    int i;
    float f;
};

enum class my_enum
{
    red = 2,
    green = -1,
    blue = 1,
    alias = 2,
};

enum unscoped_enum : unsigned char
{
    ue_a,
    ue_b = 5,
};

// no fixed underlying type: the range of values is [0, 15]
enum plain_enum
{
    pe_a,
    pe_b = 10,
};

#if defined(__GNUC__) || defined(__clang__)
struct packed_other_elements
{
    uint8_t elementa;
    uint8_t elemento;
} __attribute__((__packed__));

struct packed_element_size
{
    packed_other_elements chif;
    uint64_t element;
    uint32_t element3;
} __attribute__((__packed__));

struct packed_const_element
{
    const uint8_t value;
} __attribute__((__packed__));
#endif

TEST(field_reflection, concept)
{
    static_assert(field_countable<my_struct1>);
    static_assert(field_countable<my_struct2>);
    static_assert(field_countable<my_struct3>);
    static_assert(field_countable<my_struct4>);
    static_assert(field_countable<my_struct5>);
    static_assert(field_countable<my_struct6>);
    static_assert(field_countable<my_struct7>);
    static_assert(field_countable<my_struct8> == std_rfl);
    static_assert(field_countable<my_struct9>);
    static_assert(field_countable<named::my_struct10>);
    static_assert(field_countable<my_struct11>);
    static_assert(field_countable<my_struct12>);
    static_assert(field_countable<my_struct13>);
    static_assert(field_countable<my_struct14>);
    static_assert(field_countable<my_struct15>);
    static_assert(field_countable<std::monostate>);
#if defined(__GNUC__) || defined(__clang__)
    static_assert(field_countable<packed_element_size>);
    static_assert(field_countable<packed_const_element>);
#endif

    static_assert(field_referenceable<my_struct1>);
    static_assert(field_referenceable<my_struct2>);
    static_assert(field_referenceable<my_struct3>);
    static_assert(field_referenceable<my_struct4>);
    static_assert(field_referenceable<my_struct5>);
    static_assert(field_referenceable<my_struct6> == std_rfl);
    static_assert(field_referenceable<my_struct7> == std_rfl);
    static_assert(field_referenceable<my_struct8> == std_rfl);
    static_assert(field_referenceable<my_struct9>);
    static_assert(field_referenceable<named::my_struct10>);
    static_assert(field_referenceable<my_struct11>);
    static_assert(field_referenceable<my_struct12>);
    static_assert(field_referenceable<my_struct13>);
    static_assert(field_referenceable<my_struct14>);
    static_assert(field_referenceable<my_struct15>);
    static_assert(field_referenceable<std::monostate>);
#if defined(__GNUC__) || defined(__clang__)
    static_assert(field_referenceable<packed_element_size>);
    static_assert(field_referenceable<packed_const_element>);
#endif

    static_assert(field_namable<my_struct1>);
    static_assert(field_namable<my_struct2> == std_rfl);
    static_assert(field_namable<my_struct3> == std_rfl);
    static_assert(field_namable<my_struct4>);
    static_assert(field_namable<my_struct5>);
    static_assert(field_namable<my_struct6> == std_rfl);
    static_assert(field_namable<my_struct7> == std_rfl);
    static_assert(field_namable<my_struct8> == std_rfl);
    static_assert(field_namable<my_struct9>);
    static_assert(field_namable<named::my_struct10>);
    static_assert(field_namable<my_struct11>);
    static_assert(field_namable<my_struct12>);
    static_assert(field_namable<my_struct13>);
    static_assert(field_namable<my_struct14>);
    static_assert(field_namable<my_struct15>);
    static_assert(field_namable<std::monostate>);
#if defined(__GNUC__) || defined(__clang__)
    static_assert(field_namable<packed_element_size>);
    static_assert(field_namable<packed_const_element>);
#endif

    // non-class types and tuple-like types are not reflectable
    static_assert(!field_countable<int>);
    static_assert(!field_countable<int[3]>);
    static_assert(!field_countable<my_struct1[3]>);
    static_assert(!field_countable<my_union>);
    static_assert(!field_countable<std::array<int, 3>>);
    static_assert(!field_countable<std::array<int, 0>>);
    static_assert(!field_countable<std::pair<int, double>>);
    static_assert(!field_countable<std::tuple<int, double>>);
    static_assert(!field_countable<std::string>);
    static_assert(!field_countable<my_enum>);

    // the options only extend the set of types, and the C++20 backend ignores them
    static_assert(field_namable<my_struct1, reflect_options{}>);
    static_assert(field_namable<my_struct1, reflect_options{.non_aggregate = true}>);
    static_assert(field_namable<my_struct1, reflect_options{.non_public = true}>);
    static_assert(field_namable<my_struct1, reflect_options{.bit_fields = true}>);
    static_assert(
        field_namable<my_struct1, reflect_options{.non_aggregate = true, .non_public = true, .bit_fields = true}>);
}

TEST(field_reflection, field_count)
{
    static_assert(field_count<my_struct1> == 5);
    static_assert(field_count<my_struct2> == 2);
    static_assert(field_count<my_struct3> == 2);
    static_assert(field_count<my_struct4> == 1);
    static_assert(field_count<my_struct5> == 1);
    static_assert(field_count<my_struct6> == 5);
    static_assert(field_count<my_struct7> == 6);
#if FIELD_RFL_STD_REFLECTION
    static_assert(field_count<my_struct8> == 2);
#endif
    static_assert(field_count<my_struct9> == 1);
    static_assert(field_count<named::my_struct10> == 2);
    static_assert(field_count<my_struct11> == 2);
    static_assert(field_count<my_struct12> == 1);
    static_assert(field_count<my_struct13> == 1);
    static_assert(field_count<my_struct14> == 1);
    static_assert(field_count<my_struct15> == 0);
    static_assert(field_count<std::monostate> == 0);
#if defined(__GNUC__) || defined(__clang__)
    static_assert(field_count<packed_element_size> == 3);
    static_assert(field_count<packed_const_element> == 1);
#endif
}

TEST(field_reflection, field_type)
{
    static_assert(std::is_same_v<field_type<my_struct1, 0>, decltype(std::declval<my_struct1>().i)>);
    static_assert(std::is_same_v<field_type<my_struct1, 1>, decltype(std::declval<my_struct1>().d)>);
    static_assert(std::is_same_v<field_type<my_struct1, 2>, decltype(std::declval<my_struct1>().hello)>);
    static_assert(std::is_same_v<field_type<my_struct1, 3>, decltype(std::declval<my_struct1>().arr)>);
    static_assert(std::is_same_v<field_type<my_struct1, 4>, decltype(std::declval<my_struct1>().map)>);

    static_assert(std::is_same_v<field_type<my_struct2, 0>, decltype(std::declval<my_struct2>().i)>);
    static_assert(std::is_same_v<field_type<my_struct2, 1>, decltype(std::declval<my_struct2>().d)>);

    static_assert(std::is_same_v<field_type<my_struct3, 0>, decltype(std::declval<my_struct3>().i)>);
    static_assert(std::is_same_v<field_type<my_struct3, 1>, decltype(std::declval<my_struct3>().d)>);

    static_assert(std::is_same_v<field_type<my_struct4, 0>, decltype(std::declval<my_struct4>().i)>);
    static_assert(std::is_same_v<field_type<my_struct5, 0>, decltype(std::declval<my_struct5>().m)>);

    static_assert(std::is_same_v<field_type<my_struct9, 0>, decltype(std::declval<my_struct9>().a)>);

    static_assert(std::is_same_v<field_type<named::my_struct10, 0>, decltype(std::declval<named::my_struct10>().x0)>);
    static_assert(std::is_same_v<field_type<named::my_struct10, 1>, decltype(std::declval<named::my_struct10>().y0)>);

    static_assert(std::is_same_v<field_type<my_struct11, 0>, decltype(std::declval<my_struct11>().x1)>);
    static_assert(std::is_same_v<field_type<my_struct11, 1>, decltype(std::declval<my_struct11>().y1)>);

    static_assert(std::is_same_v<field_type<my_struct12, 0>, decltype(std::declval<my_struct12>().value)>);
    static_assert(std::is_same_v<field_type<my_struct13, 0>, decltype(std::declval<my_struct13>().value)>);
    static_assert(std::is_same_v<field_type<my_struct14, 0>, decltype(std::declval<my_struct14>().value)>);

#if FIELD_RFL_STD_REFLECTION
    static_assert(std::is_same_v<field_type<my_struct6, 0>, int>);
    static_assert(std::is_same_v<field_type<my_struct6, 4>, decltype(std::declval<my_struct1>().map)>);
    static_assert(std::is_same_v<field_type<my_struct7, 0>, int>);
    static_assert(std::is_same_v<field_type<my_struct7, 5>, my_struct7::X>);
    static_assert(std::is_same_v<field_type<my_struct8, 0>, int&&>);
    static_assert(std::is_same_v<field_type<my_struct8, 1>, double&>);
#endif

    static_assert(all_field_types_are_test_castable<my_struct15>);
    static_assert(all_field_types_are_test_castable<std::monostate>);

#if defined(__GNUC__) || defined(__clang__)
    static_assert(std::is_same_v<field_type<packed_element_size, 0>, packed_other_elements>);
    static_assert(std::is_same_v<field_type<packed_element_size, 1>, uint64_t>);
    static_assert(std::is_same_v<field_type<packed_element_size, 2>, uint32_t>);
    static_assert(std::is_same_v<field_type<packed_const_element, 0>, const uint8_t>);
#endif
}

TEST(field_reflection, field_name)
{
    static_assert(field_name<my_struct1, 0> == "i");
    static_assert(field_name<my_struct1, 1> == "d");
    static_assert(field_name<my_struct1, 2> == "hello");
    static_assert(field_name<my_struct1, 3> == "arr");
    static_assert(field_name<my_struct1, 4> == "map");

    static_assert(field_name<my_struct4, 0> == "i");
    static_assert(field_name<my_struct5, 0> == "m");

    static_assert(field_name<my_struct9, 0> == "a");

    static_assert(field_name<named::my_struct10, 0> == "x0");
    static_assert(field_name<named::my_struct10, 1> == "y0");
    static_assert(field_name<my_struct11, 0> == "x1");
    static_assert(field_name<my_struct11, 1> == "y1");
    static_assert(field_name<my_struct12, 0> == "value");
    static_assert(field_name<my_struct13, 0> == "value");
    static_assert(field_name<my_struct14, 0> == "value");
#if FIELD_RFL_STD_REFLECTION
    static_assert(field_name<my_struct2, 0> == "i");
    static_assert(field_name<my_struct2, 1> == "d");
    static_assert(field_name<my_struct3, 0> == "i");
    static_assert(field_name<my_struct3, 1> == "d");
    static_assert(field_name<my_struct6, 0> == "i");
    static_assert(field_name<my_struct6, 4> == "map");
    static_assert(field_name<my_struct7, 0> == "i");
    static_assert(field_name<my_struct7, 4> == "map");
    static_assert(field_name<my_struct7, 5> == "x");
    static_assert(field_name<my_struct8, 0> == "i");
    static_assert(field_name<my_struct8, 1> == "d");
#endif
#if defined(__GNUC__) || defined(__clang__)
    static_assert(field_name<packed_element_size, 0> == "chif");
    static_assert(field_name<packed_element_size, 1> == "element");
    static_assert(field_name<packed_element_size, 2> == "element3");
    static_assert(field_name<packed_const_element, 0> == "value");
#endif
}

TEST(field_reflection, type_name)
{
    static_assert(type_name<my_struct1> == "my_struct1");
    static_assert(type_name<named::my_struct10> == "named::my_struct10");
#if defined(__GNUC__) || defined(__clang__)
    static_assert(type_name<std::pair<int, double>> == "std::pair<int, double>");
#else
    static_assert(type_name<std::pair<int, double>> == "std::pair<int,double>");
#endif
}

TEST(field_reflection, get_field)
{
    {
        auto ms1 = my_struct1{};
        EXPECT_EQ(&get_field<0>(ms1), &ms1.i);
        EXPECT_EQ(&get_field<1>(ms1), &ms1.d);
        EXPECT_EQ(&get_field<2>(ms1), &ms1.hello);
        EXPECT_EQ(&get_field<3>(ms1), &ms1.arr);
        EXPECT_EQ(&get_field<4>(ms1), &ms1.map);
        static_assert(std::is_same_v<decltype(get_field<0>(ms1)), decltype(ms1.i)&>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms1)), decltype(ms1.d)&>);
        static_assert(std::is_same_v<decltype(get_field<2>(ms1)), decltype(ms1.hello)&>);
        static_assert(std::is_same_v<decltype(get_field<3>(ms1)), decltype(ms1.arr)&>);
        static_assert(std::is_same_v<decltype(get_field<4>(ms1)), decltype(ms1.map)&>);

        auto& ms1r = ms1;
        EXPECT_EQ(&get_field<0>(ms1r), &ms1.i);
        EXPECT_EQ(&get_field<1>(ms1r), &ms1.d);
        EXPECT_EQ(&get_field<2>(ms1r), &ms1.hello);
        EXPECT_EQ(&get_field<3>(ms1r), &ms1.arr);
        EXPECT_EQ(&get_field<4>(ms1r), &ms1.map);
        static_assert(std::is_same_v<decltype(get_field<0>(ms1r)), decltype((ms1r.i))>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms1r)), decltype((ms1r.d))>);
        static_assert(std::is_same_v<decltype(get_field<2>(ms1r)), decltype((ms1r.hello))>);
        static_assert(std::is_same_v<decltype(get_field<3>(ms1r)), decltype((ms1r.arr))>);
        static_assert(std::is_same_v<decltype(get_field<4>(ms1r)), decltype((ms1r.map))>);

        const auto& ms1cr = ms1;
        EXPECT_EQ(&get_field<0>(ms1cr), &ms1.i);
        EXPECT_EQ(&get_field<1>(ms1cr), &ms1.d);
        EXPECT_EQ(&get_field<2>(ms1cr), &ms1.hello);
        EXPECT_EQ(&get_field<3>(ms1cr), &ms1.arr);
        EXPECT_EQ(&get_field<4>(ms1cr), &ms1.map);
        static_assert(std::is_same_v<decltype(get_field<0>(ms1cr)), decltype((ms1cr.i))>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms1cr)), decltype((ms1cr.d))>);
        static_assert(std::is_same_v<decltype(get_field<2>(ms1cr)), decltype((ms1cr.hello))>);
        static_assert(std::is_same_v<decltype(get_field<3>(ms1cr)), decltype((ms1cr.arr))>);
        static_assert(std::is_same_v<decltype(get_field<4>(ms1cr)), decltype((ms1cr.map))>);

        EXPECT_EQ(get_field<0>(my_struct1{}), ms1.i);
        EXPECT_EQ(get_field<1>(my_struct1{}), ms1.d);
        EXPECT_EQ(get_field<2>(my_struct1{}), ms1.hello);
        EXPECT_EQ(get_field<3>(my_struct1{}), ms1.arr);
        EXPECT_EQ(get_field<4>(my_struct1{}), ms1.map);
        static_assert(std::is_same_v<decltype(get_field<0>(my_struct1{})), decltype(my_struct1{}.i)>);
        static_assert(std::is_same_v<decltype(get_field<1>(my_struct1{})), decltype(my_struct1{}.d)>);
        static_assert(std::is_same_v<decltype(get_field<2>(my_struct1{})), decltype(my_struct1{}.hello)>);
        static_assert(std::is_same_v<decltype(get_field<3>(my_struct1{})), decltype(my_struct1{}.arr)>);
        static_assert(std::is_same_v<decltype(get_field<4>(my_struct1{})), decltype(my_struct1{}.map)>);
    }

    {
        int i = 1;
        double d = 3.14;
        auto ms2 = my_struct2{i, d};
        EXPECT_EQ(get_field<0>(ms2), ms2.i);
        EXPECT_EQ(get_field<1>(ms2), ms2.d);
        static_assert(std::is_same_v<decltype(get_field<0>(ms2)), decltype(ms2.i)&>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms2)), decltype(ms2.d)&>);
    }

    {
        auto ms3 = my_struct3{};
        EXPECT_EQ(get_field<0>(ms3), ms3.i);
        EXPECT_EQ(get_field<1>(ms3), ms3.d);
        static_assert(std::is_same_v<decltype(get_field<0>(ms3)), decltype(ms3.i)&>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms3)), decltype(ms3.d)&>);
    }

    {
        auto ms4 = my_struct4{};
        EXPECT_EQ(get_field<0>(ms4), ms4.i);
        static_assert(std::is_same_v<decltype(get_field<0>(ms4)), decltype(ms4.i)&>);
    }

    {
        auto ms5 = my_struct5{};
        EXPECT_EQ(get_field<0>(ms5), ms5.m);
        static_assert(std::is_same_v<decltype(get_field<0>(ms5)), decltype(ms5.m)&>);
    }

    {
        auto ms9 = my_struct9{};
        EXPECT_EQ(get_field<0>(ms9), ms9.a);
        static_assert(std::is_same_v<decltype(get_field<0>(ms9)), decltype(ms9.a)&>);
    }

    {
        auto ms10 = named::my_struct10{};
        EXPECT_EQ(get_field<0>(ms10), ms10.x0);
        EXPECT_EQ(get_field<1>(ms10), ms10.y0);
        static_assert(std::is_same_v<decltype(get_field<0>(ms10)), decltype(ms10.x0)&>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms10)), decltype(ms10.y0)&>);
    }

    {
        auto ms11 = my_struct11{};
        EXPECT_EQ(get_field<0>(ms11), ms11.x1);
        EXPECT_EQ(get_field<1>(ms11), ms11.y1);
        static_assert(std::is_same_v<decltype(get_field<0>(ms11)), decltype(ms11.x1)&>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms11)), decltype(ms11.y1)&>);
    }

#if FIELD_RFL_STD_REFLECTION
    {
        auto ms7 = my_struct7{};
        EXPECT_EQ(&get_field<0>(ms7), &ms7.i);
        EXPECT_EQ(&get_field<4>(ms7), &ms7.map);
        EXPECT_EQ(&get_field<5>(ms7), &ms7.x);
        static_assert(std::is_same_v<decltype(get_field<0>(ms7)), int&>);
        static_assert(std::is_same_v<decltype(get_field<5>(ms7)), my_struct7::X&>);
        static_assert(std::is_same_v<decltype(get_field<5>(my_struct7{})), my_struct7::X>);
    }

    {
        int i = 1;
        double d = 3.14;
        auto ms8 = my_struct8{std::move(i), d};
        EXPECT_EQ(&get_field<0>(ms8), &i);
        EXPECT_EQ(&get_field<1>(ms8), &d);
        static_assert(std::is_same_v<decltype(get_field<0>(ms8)), int&>);
        static_assert(std::is_same_v<decltype(get_field<1>(ms8)), double&>);
    }
#endif

#if defined(__GNUC__) || defined(__clang__)
    {
        auto pes = packed_element_size{{1, 2}, 3, 4};
        EXPECT_EQ(get_field<0>(pes).elemento, 2);
        EXPECT_EQ(get_field<1>(pes), 3U);
        EXPECT_EQ(get_field<2>(pes), 4U);
    }
#endif
}

TEST(field_reflection, set_field)
{
    auto ms1 = my_struct1{};
    set_field<0>(ms1, 42);
    set_field<2>(ms1, "abc");
    EXPECT_EQ(ms1.i, 42);
    EXPECT_EQ(ms1.hello, "abc");

    int i = 1;
    double d = 3.14;
    auto ms2 = my_struct2{i, d};
    set_field<0>(ms2, 2);
    EXPECT_EQ(i, 2);
}

TEST(field_reflection, to_tuple)
{
    auto ms1 = my_struct1{};
    auto& ms1r = ms1;
    const auto& ms1cr = ms1;

    auto t = to_tuple(ms1);
    EXPECT_EQ(std::get<0>(t), ms1.i);
    EXPECT_EQ(std::get<1>(t), ms1.d);
    EXPECT_EQ(std::get<2>(t), ms1.hello);
    EXPECT_EQ(std::get<3>(t), ms1.arr);
    EXPECT_EQ(std::get<4>(t), ms1.map);
    EXPECT_NE(&std::get<0>(t), &ms1.i);
    EXPECT_NE(&std::get<1>(t), &ms1.d);
    EXPECT_NE(&std::get<2>(t), &ms1.hello);
    EXPECT_NE(&std::get<3>(t), &ms1.arr);
    EXPECT_NE(&std::get<4>(t), &ms1.map);

    auto tr = to_tuple(ms1r);
    EXPECT_EQ(std::get<0>(tr), ms1.i);
    EXPECT_EQ(std::get<1>(tr), ms1.d);
    EXPECT_EQ(std::get<2>(tr), ms1.hello);
    EXPECT_EQ(std::get<3>(tr), ms1.arr);
    EXPECT_EQ(std::get<4>(tr), ms1.map);
    EXPECT_NE(&std::get<0>(tr), &ms1.i);
    EXPECT_NE(&std::get<1>(tr), &ms1.d);
    EXPECT_NE(&std::get<2>(tr), &ms1.hello);
    EXPECT_NE(&std::get<3>(tr), &ms1.arr);
    EXPECT_NE(&std::get<4>(tr), &ms1.map);

    auto tcr = to_tuple(ms1cr);
    EXPECT_EQ(std::get<0>(tcr), ms1.i);
    EXPECT_EQ(std::get<1>(tcr), ms1.d);
    EXPECT_EQ(std::get<2>(tcr), ms1.hello);
    EXPECT_EQ(std::get<3>(tcr), ms1.arr);
    EXPECT_EQ(std::get<4>(tcr), ms1.map);
    EXPECT_NE(&std::get<0>(tcr), &ms1.i);
    EXPECT_NE(&std::get<1>(tcr), &ms1.d);
    EXPECT_NE(&std::get<2>(tcr), &ms1.hello);
    EXPECT_NE(&std::get<3>(tcr), &ms1.arr);
    EXPECT_NE(&std::get<4>(tcr), &ms1.map);

    auto trr = to_tuple(std::move(ms1));
    auto ms1_a = my_struct1{};
    EXPECT_EQ(std::get<0>(trr), ms1_a.i);
    EXPECT_EQ(std::get<1>(trr), ms1_a.d);
    EXPECT_EQ(std::get<2>(trr), ms1_a.hello);
    EXPECT_EQ(std::get<3>(trr), ms1_a.arr);
    EXPECT_EQ(std::get<4>(trr), ms1_a.map);
}

TEST(field_reflection, for_each_field)
{
    {
        auto ms1 = my_struct1{};
        std::string names;
        for_each_field(ms1, [&](std::string_view name, auto&) {
            names += name;
            names += ',';
        });
        EXPECT_EQ(names, "i,d,hello,arr,map,");
    }
#if FIELD_RFL_STD_REFLECTION
    {
        auto ms7 = my_struct7{};
        std::string names;
        for_each_field(ms7, [&](std::string_view name, auto&) {
            names += name;
            names += ',';
        });
        EXPECT_EQ(names, "i,d,hello,arr,map,x,");
    }
#endif

    auto ms1_a = my_struct1{};
    auto ms1_b = my_struct1{};
    for_each_field(ms1_a, [](auto&) {});
    for_each_field(ms1_a, [](std::string_view, auto&) {});
    for_each_field(ms1_a, ms1_b, [](auto&, auto&) {});
    for_each_field(ms1_a, ms1_b, [](std::string_view, auto&, auto&) {});

    auto ms3_a = my_struct3{};
    auto ms3_b = my_struct3{};
    for_each_field(ms3_a, [](auto&) {});
    for_each_field(ms3_a, ms3_b, [](auto&, auto&) {});

    auto ms4_a = my_struct4{};
    auto ms4_b = my_struct4{};
    for_each_field(ms4_a, [](auto&) {});
    for_each_field(ms4_a, [](std::string_view, auto&) {});
    for_each_field(ms4_a, ms4_b, [](auto&, auto&) {});
    for_each_field(ms4_a, ms4_b, [](std::string_view, auto&, auto&) {});

    auto ms5_a = my_struct5{};
    auto ms5_b = my_struct5{};
    for_each_field(ms5_a, [](auto&) {});
    for_each_field(ms5_a, [](std::string_view, auto&) {});
    for_each_field(ms5_a, ms5_b, [](auto&, auto&) {});
    for_each_field(ms5_a, ms5_b, [](std::string_view, auto&, auto&) {});

    for_each_field(my_struct12{std::make_unique<int>(42)}, [](std::unique_ptr<int>&& value) { EXPECT_EQ(*value, 42); });
    for_each_field(my_struct12{std::make_unique<int>(42)}, [](std::string_view name, std::unique_ptr<int>&& value) {
        EXPECT_EQ(name, "value");
        EXPECT_EQ(*value, 42);
    });
    for_each_field(my_struct12{std::make_unique<int>(42)}, my_struct12{std::make_unique<int>(43)},
                   [](std::unique_ptr<int>&& value1, std::unique_ptr<int>&& value2) {
                       EXPECT_EQ(*value1, 42);
                       EXPECT_EQ(*value2, 43);
                   });
    for_each_field(my_struct12{std::make_unique<int>(42)}, my_struct12{std::make_unique<int>(43)},
                   [](std::string_view name, std::unique_ptr<int>&& value1, std::unique_ptr<int>&& value2) {
                       EXPECT_EQ(name, "value");
                       EXPECT_EQ(*value1, 42);
                       EXPECT_EQ(*value2, 43);
                   });
}

TEST(field_reflection, all_of_field)
{
    auto ms1_a = my_struct1{};
    auto ms1_b = my_struct1{};
    EXPECT_TRUE(all_of_field(ms1_a, [](auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms1_a, [](std::string_view, auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms1_a, ms1_b, [](auto&, auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms1_a, ms1_b, [](std::string_view, auto&, auto&) { return true; }));

    auto ms3_a = my_struct3{};
    auto ms3_b = my_struct3{};
    EXPECT_TRUE(all_of_field(ms3_a, [](auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms3_a, ms3_b, [](auto&, auto&) { return true; }));

    auto ms4_a = my_struct4{};
    auto ms4_b = my_struct4{};
    EXPECT_TRUE(all_of_field(ms4_a, [](auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms4_a, [](std::string_view, auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms4_a, ms4_b, [](auto&, auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms4_a, ms4_b, [](std::string_view, auto&, auto&) { return true; }));

    auto ms5_a = my_struct5{};
    auto ms5_b = my_struct5{};
    EXPECT_TRUE(all_of_field(ms5_a, [](auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms5_a, [](std::string_view, auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms5_a, ms5_b, [](auto&, auto&) { return true; }));
    EXPECT_TRUE(all_of_field(ms5_a, ms5_b, [](std::string_view, auto&, auto&) { return true; }));

    EXPECT_TRUE(all_of_field(my_struct12{std::make_unique<int>(42)},
                             [](std::unique_ptr<int>&& value) { return *value == 42; }));
    EXPECT_TRUE(all_of_field(
        my_struct12{std::make_unique<int>(42)},
        [](std::string_view name, std::unique_ptr<int>&& value) { return name == "value" && *value == 42; }));
    EXPECT_TRUE(all_of_field(
        my_struct12{std::make_unique<int>(42)}, my_struct12{std::make_unique<int>(43)},
        [](std::unique_ptr<int>&& value1, std::unique_ptr<int>&& value2) { return *value1 == 42 && *value2 == 43; }));
    EXPECT_TRUE(all_of_field(my_struct12{std::make_unique<int>(42)}, my_struct12{std::make_unique<int>(43)},
                             [](std::string_view name, std::unique_ptr<int>&& value1, std::unique_ptr<int>&& value2) {
                                 return name == "value" && *value1 == 42 && *value2 == 43;
                             }));
}

TEST(field_reflection, any_of_field)
{
    auto ms1_a = my_struct1{};
    auto ms1_b = my_struct1{};
    EXPECT_FALSE(any_of_field(ms1_a, [](auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms1_a, [](std::string_view, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms1_a, ms1_b, [](auto&, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms1_a, ms1_b, [](std::string_view, auto&, auto&) { return false; }));

    auto ms3_a = my_struct3{};
    auto ms3_b = my_struct3{};
    EXPECT_FALSE(any_of_field(ms3_a, [](auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms3_a, ms3_b, [](auto&, auto&) { return false; }));

    auto ms4_a = my_struct4{};
    auto ms4_b = my_struct4{};
    EXPECT_FALSE(any_of_field(ms4_a, [](auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms4_a, [](std::string_view, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms4_a, ms4_b, [](auto&, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms4_a, ms4_b, [](std::string_view, auto&, auto&) { return false; }));

    auto ms5_a = my_struct5{};
    auto ms5_b = my_struct5{};
    EXPECT_FALSE(any_of_field(ms5_a, [](auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms5_a, [](std::string_view, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms5_a, ms5_b, [](auto&, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms5_a, ms5_b, [](std::string_view, auto&, auto&) { return false; }));

    auto ms9_a = my_struct9{};
    auto ms9_b = my_struct9{};
    EXPECT_FALSE(any_of_field(ms9_a, [](auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms9_a, [](std::string_view, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms9_a, ms9_b, [](auto&, auto&) { return false; }));
    EXPECT_FALSE(any_of_field(ms9_a, ms9_b, [](std::string_view, auto&, auto&) { return false; }));

    EXPECT_TRUE(any_of_field(my_struct12{std::make_unique<int>(42)},
                             [](std::unique_ptr<int>&& value) { return *value == 42; }));
    EXPECT_TRUE(any_of_field(
        my_struct12{std::make_unique<int>(42)},
        [](std::string_view name, std::unique_ptr<int>&& value) { return name == "value" && *value == 42; }));
    EXPECT_TRUE(any_of_field(
        my_struct12{std::make_unique<int>(42)}, my_struct12{std::make_unique<int>(43)},
        [](std::unique_ptr<int>&& value1, std::unique_ptr<int>&& value2) { return *value1 == 42 && *value2 == 43; }));
    EXPECT_TRUE(any_of_field(my_struct12{std::make_unique<int>(42)}, my_struct12{std::make_unique<int>(43)},
                             [](std::string_view name, std::unique_ptr<int>&& value1, std::unique_ptr<int>&& value2) {
                                 return name == "value" && *value1 == 42 && *value2 == 43;
                             }));
}
TEST(field_reflection, enum_api)
{
    static_assert(enum_count<my_enum> == 3);
    static_assert(enum_values<my_enum> == std::array{my_enum::green, my_enum::blue, my_enum::red});
    static_assert(enum_names<my_enum> == std::array<std::string_view, 3>{"green", "blue", "red"});
    static_assert(enum_name(my_enum::red) == "red");
    static_assert(enum_name(my_enum::alias) == "red");
    static_assert(enum_name(my_enum::green) == "green");
    static_assert(enum_name(static_cast<my_enum>(100)).empty());
    static_assert(enum_cast<my_enum>("blue") == my_enum::blue);
    // the C++20 backend cannot see aliases
    static_assert(enum_cast<my_enum>("alias") == (std_rfl ? std::optional(my_enum::red) : std::nullopt));
    static_assert(!enum_cast<my_enum>("yellow").has_value());

    static_assert(enum_count<unscoped_enum> == 2);
    static_assert(enum_name(ue_b) == "ue_b");
    static_assert(enum_cast<unscoped_enum>("ue_a") == ue_a);

    static_assert(enum_count<plain_enum> == 2);
    static_assert(enum_names<plain_enum> == std::array<std::string_view, 2>{"pe_a", "pe_b"});
    static_assert(enum_name(pe_b) == "pe_b");
    static_assert(enum_cast<plain_enum>("pe_b") == pe_b);

    EXPECT_EQ(enum_name(my_enum::blue), "blue");
    EXPECT_EQ(enum_cast<my_enum>("green"), my_enum::green);
}
struct with_mutable
{
    int i;
    mutable int m;
    const int& r;
};
TEST(field_reflection, cv_field_type)
{
    static_assert(std::is_same_v<field_type<const my_struct1, 0>, const int>);
    static_assert(std::is_same_v<field_type<const my_struct1, 2>, const std::string>);
    static_assert(std::is_same_v<field_type<volatile my_struct1, 1>, volatile double>);
    static_assert(std::is_same_v<field_type<const with_mutable, 0>, const int>);
    static_assert(std::is_same_v<field_type<const with_mutable, 1>, int>);
    static_assert(std::is_same_v<field_type<const with_mutable, 2>, const int&>);
}
// The explicit template arguments of the previous versions are still accepted.
TEST(field_reflection, explicit_template_arguments)
{
    auto s = my_struct1{};
    const auto& cs = s;
    EXPECT_EQ((get_field<0, my_struct1>(s)), 287);
    EXPECT_EQ((get_field<0, const my_struct1>(cs)), 287);
    EXPECT_EQ((get_field<2, my_struct1>(my_struct1{})), "Hello World");
    EXPECT_EQ(std::get<0>(to_tuple<my_struct1&>(s)), 287);
    std::size_t count = 0;
    for_each_field<my_struct1&>(s, [&](auto&) { ++count; });
    EXPECT_EQ(count, field_count<my_struct1>);
    EXPECT_TRUE((all_of_field<my_struct1&>(s, [](auto&) { return true; })));
    EXPECT_TRUE((any_of_field<my_struct1&, my_struct1&>(s, s, [](auto&, auto&) { return true; })));
    set_field<0, my_struct1>(s, 1);
    EXPECT_EQ(s.i, 1);

    constexpr auto opts = reflect_options{};
    EXPECT_EQ((get_field<0, opts>(s)), 1);
    set_field<0, opts>(s, 2);
    EXPECT_EQ(std::get<0>(to_tuple<opts>(s)), 2);
    for_each_field<opts>(s, [&](auto&) { ++count; });
    EXPECT_EQ(count, 2 * field_count<my_struct1>);
}
struct array_members
{
    int a[3];
    int b;
    char s[4][2];
    std::string strs[2];
};
struct nested_array_members
{
    array_members m;
    my_struct15 e;
    double d[1];
    std::unique_ptr<int> p[2];
};
TEST(field_reflection, c_array_members)
{
    static_assert(field_namable<array_members>);
    static_assert(field_count<array_members> == 4);
    static_assert(std::is_same_v<field_type<array_members, 0>, int[3]>);
    static_assert(std::is_same_v<field_type<array_members, 2>, char[4][2]>);
    static_assert(field_name<array_members, 0> == "a");
    static_assert(field_name<array_members, 3> == "strs");
    static_assert(field_namable<nested_array_members>);
    static_assert(field_count<nested_array_members> == 4);
    static_assert(std::is_same_v<field_type<nested_array_members, 3>, std::unique_ptr<int>[2]>);

    auto a = array_members{{1, 2, 3}, 4, {{'a', 'b'}}, {"x", "y"}};
    static_assert(std::is_same_v<decltype(get_field<0>(a)), int (&)[3]>);
    EXPECT_EQ(get_field<0>(a)[2], 3);
    EXPECT_EQ(get_field<1>(a), 4);
    EXPECT_EQ(get_field<2>(a)[0][1], 'b');
    EXPECT_EQ(get_field<1>(std::move(a)), 4);

    auto n = nested_array_members{};
    n.p[1] = std::make_unique<int>(5);
    std::unique_ptr<int> moved;
    for_each_field(std::move(n), [&](std::string_view name, auto&& field) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(field)>, std::unique_ptr<int>[2]>)
        {
            static_assert(std::is_rvalue_reference_v<decltype(field)>);
            EXPECT_EQ(name, "p");
            moved = std::move(field[1]);
        }
    });
    EXPECT_EQ(*moved, 5);
}
// NOLINTEND
