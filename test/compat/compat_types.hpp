// Types and helpers shared by the translation units that compare the C++20 and C++26 backends.
// This header is included by compat_legacy.cpp (C++20 backend) and compat_std.cpp (C++26 backend if available).
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include "field_reflection.hpp"

namespace compat
{
    struct plain
    {
        int i;
        double d;
        std::string s;
    };

    struct nested
    {
        plain p;
        std::optional<int> o;
    };

    struct with_lref
    {
        int& i;
        double& d;
    };

    struct with_cref
    {
        const int& i;
        const double& d;
    };

    struct with_rref
    {
        int&& i;
        double& d;
    };

    struct with_const
    {
        const int c;
        std::array<int, 2> a;
    };

    struct empty
    {
    };

    struct base
    {
        int x;
        int y;
    };

    struct derived : base
    {
        int z;
    };

    struct derived_empty_base : empty
    {
        int z;
    };

    struct derived_no_member : base
    {
    };

    enum class color
    {
        red = 1,
        green = 2,
        blue = 4,
        alias = 1,
    };

    enum plain_enum : std::uint8_t
    {
        pe_a,
        pe_b = 10,
        pe_c = 200,
    };

#if defined(__GNUC__) || defined(__clang__)
    struct packed
    {
        std::uint8_t a;
        std::uint64_t b;
    } __attribute__((__packed__));
#endif

    using result_map = std::map<std::string, std::string>;
}  // namespace compat

namespace
{
    template <typename T>
    std::string describe_type()
    {
        using namespace field_reflection;
        std::string r;
        r += "countable=" + std::to_string(field_countable<T>);
        r += " referenceable=" + std::to_string(field_referenceable<T>);
        r += " namable=" + std::to_string(field_namable<T>);
        if constexpr (field_countable<T>)
        {
            r += " count=" + std::to_string(field_count<T>);
        }
        if constexpr (field_referenceable<T>)
        {
            r += " types=";
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                ((r += std::string(type_name<field_type<T, Is>>) + ","), ...);
            }(std::make_index_sequence<field_count<T>>());
        }
        if constexpr (field_namable<T>)
        {
            r += " names=";
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                ((r += std::string(field_name<T, Is>) + ","), ...);
            }(std::make_index_sequence<field_count<T>>());
        }
        return r;
    }

    template <typename E>
    std::string describe_enum()
    {
        using namespace field_reflection;
        std::string r = "count=" + std::to_string(enum_count<E>) + " entries=";
        for (std::size_t i = 0; i < enum_count<E>; ++i)
        {
            r += std::string(enum_names<E>[i]) + ":" +
                 std::to_string(static_cast<long long>(static_cast<std::underlying_type_t<E>>(enum_values<E>[i]))) +
                 ",";
        }
        return r;
    }

    compat::result_map describe_all()
    {
        using namespace compat;
        result_map m;
#define COMPAT_TYPE(...) m[#__VA_ARGS__] = describe_type<__VA_ARGS__>()
        COMPAT_TYPE(plain);
        COMPAT_TYPE(nested);
        COMPAT_TYPE(with_lref);
        COMPAT_TYPE(with_cref);
        COMPAT_TYPE(with_rref);
        COMPAT_TYPE(with_const);
        COMPAT_TYPE(empty);
        COMPAT_TYPE(base);
        COMPAT_TYPE(derived);
        COMPAT_TYPE(derived_empty_base);
        COMPAT_TYPE(derived_no_member);
        COMPAT_TYPE(std::monostate);
        COMPAT_TYPE(std::array<int, 3>);
        COMPAT_TYPE(std::pair<int, int>);
        COMPAT_TYPE(std::string);
        COMPAT_TYPE(int[3]);
        COMPAT_TYPE(int);
#if defined(__GNUC__) || defined(__clang__)
        COMPAT_TYPE(packed);
#endif
#undef COMPAT_TYPE
        m["enum color"] = describe_enum<color>();
        m["enum plain_enum"] = describe_enum<plain_enum>();
        m["type_name"] = std::string(field_reflection::type_name<plain>) + "," +
                         std::string(field_reflection::type_name<std::pair<int, double>>);
        return m;
    }
}  // namespace
