#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>

namespace compat
{
    using result_map = std::map<std::string, std::string>;
    result_map describe_legacy();
    result_map describe_std();
    bool std_reflection_enabled();
}  // namespace compat

// Both backends are linked into the same program, so this test also checks that they do not violate the ODR.
TEST(compat, backends_agree)
{
    const auto legacy = compat::describe_legacy();
    const auto std = compat::describe_std();
    ASSERT_EQ(legacy.size(), std.size());

    // Differences caused by the limitations of the C++20 backend
    const auto expected_diff =
        compat::std_reflection_enabled()
            ? std::set<std::string>{"with_lref",          "with_cref",         "with_rref",      "derived",
                                    "derived_empty_base", "derived_no_member", "enum plain_enum"}
            : std::set<std::string>{};
    for (const auto& [key, value] : legacy)
    {
        ASSERT_TRUE(std.contains(key)) << key;
        if (expected_diff.contains(key))
        {
            EXPECT_NE(value, std.at(key)) << key;
        }
        else
        {
            EXPECT_EQ(value, std.at(key)) << key;
        }
    }
}

TEST(compat, expected_results)
{
    const auto legacy = compat::describe_legacy();
    EXPECT_TRUE(legacy.at("plain").starts_with("countable=1 referenceable=1 namable=1 count=3 types=int,double,"));
    EXPECT_TRUE(legacy.at("plain").ends_with(" names=i,d,s,"));
    EXPECT_EQ(legacy.at("empty"), "countable=1 referenceable=1 namable=1 count=0 types= names=");
    EXPECT_EQ(legacy.at("std::monostate"), "countable=1 referenceable=1 namable=1 count=0 types= names=");
    EXPECT_EQ(legacy.at("int[3]"), "countable=0 referenceable=0 namable=0");
    EXPECT_EQ(legacy.at("std::array<int, 3>"), "countable=0 referenceable=0 namable=0");
    EXPECT_EQ(legacy.at("derived"), "countable=1 referenceable=0 namable=0 count=3");
    EXPECT_EQ(legacy.at("enum color"), "count=3 entries=red:1,green:2,blue:4,");
    EXPECT_EQ(legacy.at("enum plain_enum"), "count=2 entries=pe_a:0,pe_b:10,");

    if (compat::std_reflection_enabled())
    {
        const auto std = compat::describe_std();
        EXPECT_EQ(std.at("derived"), "countable=1 referenceable=1 namable=1 count=3 types=int,int,int, names=x,y,z,");
        EXPECT_EQ(std.at("derived_empty_base"), "countable=1 referenceable=1 namable=1 count=1 types=int, names=z,");
        EXPECT_EQ(std.at("with_rref"), "countable=1 referenceable=1 namable=1 count=2 types=int&&,double&, names=i,d,");
        EXPECT_EQ(std.at("enum plain_enum"), "count=3 entries=pe_a:0,pe_b:10,pe_c:200,");
    }
}
