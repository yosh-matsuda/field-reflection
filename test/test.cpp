#include <gtest/gtest.h>
#include <array>
#include <map>
#include <optional>
#include <string>
#include "field_reflection.hpp"

// NOLINTBEGIN
using namespace field_reflection;

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

TEST(field_reflection, concept)
{
    static_assert(field_countable<my_struct1>);
    static_assert(field_countable<my_struct2>);
    static_assert(field_countable<my_struct3>);
    static_assert(field_countable<my_struct4>);
    static_assert(field_countable<my_struct5>);
    static_assert(field_countable<my_struct6>);
    static_assert(field_countable<my_struct7>);
    static_assert(!field_countable<my_struct8>);

    static_assert(field_referenceable<my_struct1>);
    static_assert(field_referenceable<my_struct2>);
    static_assert(field_referenceable<my_struct3>);
    static_assert(field_referenceable<my_struct4>);
    static_assert(field_referenceable<my_struct5>);
    static_assert(!field_referenceable<my_struct6>);
    static_assert(!field_referenceable<my_struct7>);
    static_assert(!field_referenceable<my_struct8>);

    static_assert(field_namable<my_struct1>);
    static_assert(!field_namable<my_struct2>);
    static_assert(!field_namable<my_struct3>);
    static_assert(field_namable<my_struct4>);
    static_assert(field_namable<my_struct5>);
    static_assert(!field_namable<my_struct6>);
    static_assert(!field_namable<my_struct7>);
    static_assert(!field_namable<my_struct8>);
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
}
// NOLINTEND
