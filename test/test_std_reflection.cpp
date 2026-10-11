#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <vector>
#include "field_reflection.hpp"

#if defined(FIELD_RFL_TEST_REQUIRE_STD_REFLECTION) && !FIELD_RFL_STD_REFLECTION
#error "C++26 reflection is required by FIELD_RFL_TEST_REQUIRE_STD_REFLECTION but not enabled"
#endif

#if FIELD_RFL_STD_REFLECTION

// NOLINTBEGIN
using namespace field_reflection;

namespace std_rfl_test
{
    // more than 100 fields
#define FIELD_RFL_TEST_FIELDS10(p) \
    int p##0;                      \
    int p##1;                      \
    int p##2;                      \
    int p##3;                      \
    int p##4;                      \
    int p##5;                      \
    int p##6;                      \
    int p##7;                      \
    int p##8;                      \
    int p##9;
    struct many_fields
    {
        FIELD_RFL_TEST_FIELDS10(a)
        FIELD_RFL_TEST_FIELDS10(b)
        FIELD_RFL_TEST_FIELDS10(c)
        FIELD_RFL_TEST_FIELDS10(d)
        FIELD_RFL_TEST_FIELDS10(e)
        FIELD_RFL_TEST_FIELDS10(f)
        FIELD_RFL_TEST_FIELDS10(g)
        FIELD_RFL_TEST_FIELDS10(h)
        FIELD_RFL_TEST_FIELDS10(i)
        FIELD_RFL_TEST_FIELDS10(j)
        FIELD_RFL_TEST_FIELDS10(k)
        FIELD_RFL_TEST_FIELDS10(l)
    };
#undef FIELD_RFL_TEST_FIELDS10

    struct base1
    {
        int x;
        int y;
    };

    struct empty_base
    {
    };

    struct derived1 : base1
    {
        int z;
    };

    struct derived2 : empty_base
    {
        int z;
    };

    struct derived3 : derived1
    {
        std::string w;
    };

    struct shadowing : base1
    {
        int x;
    };

    struct virtual_base : virtual base1
    {
        int z;
    };

    struct diamond_left : base1
    {
    };
    struct diamond_right : base1
    {
    };
    struct diamond : diamond_left, diamond_right
    {
    };

    struct anonymous_union
    {
        int tag;
        union
        {
            int i;
            float f;
        };
    };

    struct bit_field
    {
        int a : 3;
        unsigned b : 5;
        int c;
    };

    class non_aggregate
    {
    public:
        explicit non_aggregate(int v) : value(v) {}
        int value;
    };

    class private_members
    {
    public:
        private_members() = default;
        private_members(int a, int b) : a_(a), b_(b) {}
        int a() const { return a_; }

    private:
        int a_ = 0;
        int b_ = 0;
    };

    class private_base : private base1
    {
    public:
        int z = 0;
        int x() const { return base1::x; }
    };

    struct with_const_member
    {
        const int c;
        int d;
    };

    enum class wide_enum : std::uint32_t
    {
        zero = 0,
        large = 1u << 20,
    };

    enum class signed_enum : int
    {
        neg = -1000,
        pos = 1000,
    };

    struct with_reference_members
    {
        int&& r;
        double& l;
    };
}  // namespace std_rfl_test

using namespace std_rfl_test;

TEST(std_reflection, many_fields)
{
    static_assert(field_namable<many_fields>);
    static_assert(field_count<many_fields> == 120);
    static_assert(field_name<many_fields, 0> == "a0");
    static_assert(field_name<many_fields, 119> == "l9");

    many_fields m{};
    get_field<119>(m) = 42;
    EXPECT_EQ(m.l9, 42);
    int sum = 0;
    for_each_field(m, [&](int v) { sum += v; });
    EXPECT_EQ(sum, 42);
}

TEST(std_reflection, base_classes)
{
    static_assert(field_namable<derived1>);
    static_assert(field_count<derived1> == 3);
    static_assert(field_name<derived1, 0> == "x");
    static_assert(field_name<derived1, 1> == "y");
    static_assert(field_name<derived1, 2> == "z");

    static_assert(field_namable<derived2>);
    static_assert(field_count<derived2> == 1);

    static_assert(field_namable<derived3>);
    static_assert(field_count<derived3> == 4);
    static_assert(field_name<derived3, 3> == "w");

    // duplicate names are not namable
    static_assert(field_referenceable<shadowing>);
    static_assert(!field_namable<shadowing>);
    static_assert(field_count<shadowing> == 3);

    // virtual or repeated bases are not supported
    static_assert(!field_countable<virtual_base>);
    static_assert(!field_countable<diamond>);

    derived3 d{{{1, 2}, 3}, "four"};
    EXPECT_EQ(get_field<0>(d), 1);
    EXPECT_EQ(get_field<2>(d), 3);
    EXPECT_EQ(get_field<3>(d), "four");
    EXPECT_EQ(&get_field<0>(d), &d.x);

    std::string names;
    for_each_field(d, [&](std::string_view name, const auto&) { names += name; });
    EXPECT_EQ(names, "xyzw");

    auto [x, y, z, w] = to_tuple(d);
    EXPECT_EQ(x + y + z, 6);
    EXPECT_EQ(w, "four");

    // rvalue
    auto moved = get_field<3>(std::move(d));
    EXPECT_EQ(moved, "four");
}

TEST(std_reflection, anonymous_union)
{
    static_assert(field_referenceable<anonymous_union>);
    static_assert(!field_namable<anonymous_union>);
    static_assert(field_count<anonymous_union> == 2);

    anonymous_union u{1, {2}};
    EXPECT_EQ(get_field<0>(u), 1);
    EXPECT_EQ(get_field<1>(u).i, 2);
}

TEST(std_reflection, const_member)
{
    static_assert(field_namable<with_const_member>);
    static_assert(std::is_same_v<field_type<with_const_member, 0>, const int>);
    with_const_member c{1, 2};
    static_assert(std::is_same_v<decltype(get_field<0>(c)), const int&>);
    static_assert(std::is_same_v<decltype(get_field<1>(std::as_const(c))), const int&>);
    auto t = to_tuple(c);
    static_assert(std::is_same_v<decltype(t), std::tuple<const int, int>>);
    EXPECT_EQ(std::get<0>(t), 1);
}

TEST(std_reflection, reference_members)
{
    static_assert(field_namable<with_reference_members>);
    static_assert(field_name<with_reference_members, 0> == "r");
    int i = 1;
    double d = 2.0;
    with_reference_members s{std::move(i), d};
    auto t = to_tuple(s);
    static_assert(std::is_same_v<decltype(t), std::tuple<int&&, double&>>);
    EXPECT_EQ(&std::get<1>(t), &d);
    std::get<0>(t) = 3;
    EXPECT_EQ(i, 3);
    auto u = to_tuple(std::move(s));
    static_assert(std::is_same_v<decltype(u), std::tuple<int&&, double&>>);
}

TEST(std_reflection, bit_fields)
{
    constexpr auto opt = reflect_options{.bit_fields = true};
    static_assert(field_countable<bit_field>);
    static_assert(field_count<bit_field> == 3);
    static_assert(!field_referenceable<bit_field>);
    static_assert(!field_namable<bit_field>);

    static_assert(field_namable<bit_field, opt>);
    static_assert(field_name<bit_field, 1, opt> == "b");
    static_assert(std::is_same_v<field_type<bit_field, 1, opt>, unsigned>);

    bit_field b{-1, 7, 3};
    static_assert(std::is_same_v<decltype(get_field<0, opt>(b)), int>);
    EXPECT_EQ((get_field<0, opt>(b)), -1);
    EXPECT_EQ((get_field<1, opt>(b)), 7U);
    set_field<1, opt>(b, 31U);
    EXPECT_EQ(b.b, 31U);
    set_field<1, opt>(b, 32U);  // truncated
    EXPECT_EQ(b.b, 0U);

    int sum = 0;
    for_each_field<opt>(b, [&](std::string_view, auto v) { sum += static_cast<int>(v); });
    EXPECT_EQ(sum, -1 + 0 + 3);
    EXPECT_TRUE(all_of_field<opt>(b, b, [](auto l, auto r) { return l == r; }));

    auto t = to_tuple<opt>(b);
    static_assert(std::is_same_v<decltype(t), std::tuple<int, unsigned, int>>);
    EXPECT_EQ(std::get<2>(t), 3);
}

TEST(std_reflection, bit_fields_in_visitors)
{
    constexpr auto opt = reflect_options{.bit_fields = true};

    // a visitor gets a bit-field of a non-const lvalue as an lvalue and its assignment is written back
    bit_field b{1, 2, 3};
    for_each_field<opt>(b, [](auto& v) { v += 1; });
    EXPECT_EQ(b.a, 2);
    EXPECT_EQ(b.b, 3U);
    EXPECT_EQ(b.c, 4);
    for_each_field<opt>(b, [](std::string_view name, auto&& v) {
        if (name == "b") v = 9U;
    });
    EXPECT_EQ(b.b, 9U);

    // the value is written back when the visitor returns, before the next field is visited
    for_each_field<opt>(b, [&](std::string_view name, auto& v) {
        if (name == "a") v = 0;
        if (name == "b") EXPECT_EQ(b.a, 0);
    });

    // with two objects
    bit_field src{-2, 17, 5};
    for_each_field<opt>(b, src, [](auto& l, const auto& r) { l = r; });
    EXPECT_EQ(b.a, -2);
    EXPECT_EQ(b.b, 17U);
    EXPECT_EQ(b.c, 5);

    // predicates
    EXPECT_FALSE(all_of_field<opt>(b, [](auto& v) { return (v = 0) != 0; }));
    EXPECT_EQ(b.a, 0);
    EXPECT_EQ(b.b, 17U);  // not visited
    EXPECT_TRUE(any_of_field<opt>(b, [](std::string_view name, auto& v) { return name == "b" && (v = 1); }));
    EXPECT_EQ(b.b, 1U);

    // a bit-field of a const lvalue is a const lvalue, and of an rvalue is an xvalue
    const bit_field cb{1, 2, 3};
    constexpr auto assignable = []<typename T>(T&& t) {
        return requires { for_each_field<opt>(std::forward<T>(t), [](auto& v) -> decltype(void(v = {})) {}); };
    };
    static_assert(assignable(b));
    static_assert(!assignable(cb));
    for_each_field<opt>(cb,
                        [](const auto& v) { static_assert(std::is_const_v<std::remove_reference_t<decltype(v)>>); });
    for_each_field<opt>(bit_field{1, 2, 3}, [](auto&& v) { static_assert(std::is_rvalue_reference_v<decltype(v)>); });

    // in constant evaluation
    static_assert([] {
        bit_field c{1, 2, 3};
        for_each_field<opt>(c, [](auto& v) { v *= 2; });
        return c.a == 2 && c.b == 4U && c.c == 6;
    }());
}

TEST(std_reflection, non_aggregate)
{
    constexpr auto opt = reflect_options{.non_aggregate = true};
    static_assert(!field_countable<non_aggregate>);
    static_assert(field_namable<non_aggregate, opt>);
    static_assert(field_name<non_aggregate, 0, opt> == "value");

    // non-public members still require non_public
    static_assert(!field_countable<private_members, opt>);
    // tuple-like types are always excluded
    static_assert(!field_countable<std::pair<int, int>, opt>);

    non_aggregate n{5};
    EXPECT_EQ((get_field<0, opt>(n)), 5);
    for_each_field<opt>(n, [](std::string_view name, int& v) {
        EXPECT_EQ(name, "value");
        v = 6;
    });
    EXPECT_EQ(n.value, 6);
}

TEST(std_reflection, non_public)
{
    constexpr auto opt = reflect_options{.non_aggregate = true, .non_public = true};
    static_assert(!field_countable<private_members>);
    static_assert(!field_countable<private_members, reflect_options{.non_public = true}>);
    static_assert(field_namable<private_members, opt>);
    static_assert(field_count<private_members, opt> == 2);
    static_assert(field_name<private_members, 0, opt> == "a_");
    static_assert(field_name<private_members, 1, opt> == "b_");

    private_members p{1, 2};
    EXPECT_EQ((get_field<1, opt>(p)), 2);
    set_field<0, opt>(p, 10);
    EXPECT_EQ(p.a(), 10);

    static_assert(!field_countable<private_base, reflect_options{.non_aggregate = true}>);
    static_assert(field_namable<private_base, opt>);
    static_assert(field_count<private_base, opt> == 3);
    private_base pb;
    set_field<0, opt>(pb, 7);
    EXPECT_EQ(pb.x(), 7);
}

TEST(std_reflection, enum_api)
{
    // all enumerators are listed regardless of enum_range
    static_assert(enum_count<wide_enum> == 2);
    static_assert(enum_name(wide_enum::large) == "large");
    static_assert(enum_cast<wide_enum>("large") == wide_enum::large);
    static_assert(enum_values<signed_enum> == std::array{signed_enum::neg, signed_enum::pos});
    static_assert(enum_name(signed_enum::neg) == "neg");

    enum class empty_enum
    {
    };
    static_assert(enum_count<empty_enum> == 0);
    static_assert(enum_name(empty_enum{}).empty());
}

#if FIELD_RFL_ANNOTATIONS
namespace std_rfl_test
{
    struct alias_name
    {
        const char* name;
        consteval alias_name(std::string_view n) : name(std::define_static_string(n)) {}
    };

    struct skip
    {
    };

    struct[[= alias_name("Annotated")]] annotated
    {
        [[= alias_name("ID")]] int id;
        [[ = skip{}, = 1, = 2 ]] int internal;
        std::string text;
    };

    struct annotated_derived : annotated
    {
        [[= alias_name("Extra")]] int extra;
    };

    struct annotated_bit_field
    {
        int a;
        [[= alias_name("B")]] int b : 3;
    };
}  // namespace std_rfl_test

TEST(std_reflection, annotations)
{
    static_assert(field_annotation<annotated, 0, alias_name>.has_value());
    static_assert(std::string_view(field_annotation<annotated, 0, alias_name>->name) == "ID");
    static_assert(!field_annotation<annotated, 1, alias_name>.has_value());
    static_assert(field_annotations<annotated, 1, skip>.size() == 1);
    static_assert(field_annotations<annotated, 1, int> == std::array{1, 2});
    static_assert(field_annotations<annotated, 2, int>.empty());

    static_assert(std::string_view(field_annotation<annotated_derived, 0, alias_name>->name) == "ID");
    static_assert(std::string_view(field_annotation<annotated_derived, 3, alias_name>->name) == "Extra");

    static_assert(std::string_view(type_annotation<annotated, alias_name>->name) == "Annotated");
    static_assert(type_annotations<annotated_derived, alias_name>.empty());

    // annotations do not require references to the fields
    static_assert(!field_referenceable<annotated_bit_field>);
    static_assert(std::string_view(field_annotation<annotated_bit_field, 1, alias_name>->name) == "B");
}
#endif
// NOLINTEND

#endif
