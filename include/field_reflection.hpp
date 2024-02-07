/*===================================================*
|  field-reflection version v0.1.0                   |
|  https://github.com/yosh-matsuda/field-reflection  |
|                                                    |
|  Copyright (c) 2024 Yoshiki Matsuda @yosh-matsuda  |
|                                                    |
|  This software is released under the MIT License.  |
|  https://opensource.org/license/mit/               |
====================================================*/

#pragma once

#include <climits>  // CHAR_BIT
#include <limits>
#include <source_location>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace field_reflection
{
    namespace detail
    {
        template <typename T, std::size_t = 0>
        struct any_lref
        {
            template <typename U>
            requires (!std::same_as<U, T>)
            constexpr operator U&() const&& noexcept;  // NOLINT
            template <typename U>
            requires (!std::same_as<U, T>)
            constexpr operator U&() const& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_rref
        {
            template <typename U>
            requires (!std::same_as<U, T>)
            constexpr operator U() const&& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_lref_no_base
        {
            template <typename U>
            requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
            constexpr operator U&() const&& noexcept;  // NOLINT
            template <typename U>
            requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
            constexpr operator U&() const& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_rref_no_base
        {
            template <typename U>
            requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
            constexpr operator U() const&& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_lref_base
        {
            template <typename U>
            requires std::is_base_of_v<U, T>
            constexpr operator U&() const&& noexcept;  // NOLINT
            template <typename U>
            requires std::is_base_of_v<U, T>
            constexpr operator U&() const& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_rref_base
        {
            template <typename U>
            requires std::is_base_of_v<U, T>
            constexpr operator U() const&& noexcept;  // NOLINT
        };

        template <typename T, std::size_t ArgNum>
        concept constructible = []() {
            if constexpr (ArgNum == 0)
            {
                return requires { T{}; };
            }
            else if constexpr (std::is_copy_constructible_v<T>)
            {
                return []<std::size_t I0, std::size_t... Is>(std::index_sequence<I0, Is...>) {
                    return requires { T{std::declval<any_lref_no_base<T, I0>>(), std::declval<any_lref<T, Is>>()...}; };
                }(std::make_index_sequence<ArgNum>());
            }
            else
            {
                return []<std::size_t I0, std::size_t... Is>(std::index_sequence<I0, Is...>) {
                    return requires { T{std::declval<any_rref_no_base<T, I0>>(), std::declval<any_rref<T, Is>>()...}; };
                }(std::make_index_sequence<ArgNum>());
            }
        }();

        template <typename T>
        concept has_base = []() {
            if constexpr (std::is_copy_constructible_v<T>)
            {
                return requires { T{std::declval<any_lref_base<T>>()}; };
            }
            else
            {
                return requires { T{std::declval<any_rref_base<T>>()}; };
            }
        }();

        constexpr std::size_t macro_max_fields_count = 100;

        template <typename T, std::size_t N>
        requires std::is_aggregate_v<T>
        constexpr std::size_t field_count_impl = []() {
            // num macro vs. theoretical max field num with bit fields
            constexpr auto max_field_count = std::min(std::size_t{macro_max_fields_count}, sizeof(T) * CHAR_BIT);

            if constexpr (N >= max_field_count)
            {
                return std::numeric_limits<std::size_t>::max();
            }
            else
            {
                if constexpr (constructible<T, N> && !constructible<T, N + 1>)
                {
                    return N;
                }
                else
                {
                    return field_count_impl<T, N + 1>;
                }
            }
        }();

        template <typename T>
        requires std::is_aggregate_v<T>
        constexpr std::size_t field_count_value = field_count_impl<T, 0>;

        template <typename T>
        concept field_countable =
            std::is_aggregate_v<T> && requires { requires field_count_value<T> <= macro_max_fields_count; };

        template <field_countable T>
        constexpr std::size_t field_count = field_count_value<T>;

        template <typename T>
        concept field_referenceable = field_countable<T> && (!has_base<T>);

        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires (field_count<U> == 0)
        constexpr auto to_ref_tuple(T&&)
        {
            return std::tie();
        }
        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires (field_count<U> == 0)
        constexpr auto to_tuple(T&&)
        {
            return std::tie();
        }

#pragma region TO_TUPLE_TEMPLATE_MACRO
// map macro: https://github.com/swansontec/map-macro
#define FIELD_RFL_EVAL0(...) __VA_ARGS__
#define FIELD_RFL_EVAL1(...) FIELD_RFL_EVAL0(FIELD_RFL_EVAL0(FIELD_RFL_EVAL0(__VA_ARGS__)))
#define FIELD_RFL_EVAL2(...) FIELD_RFL_EVAL1(FIELD_RFL_EVAL1(FIELD_RFL_EVAL1(__VA_ARGS__)))
#define FIELD_RFL_EVAL3(...) FIELD_RFL_EVAL2(FIELD_RFL_EVAL2(FIELD_RFL_EVAL2(__VA_ARGS__)))
#define FIELD_RFL_EVAL4(...) FIELD_RFL_EVAL3(FIELD_RFL_EVAL3(FIELD_RFL_EVAL3(__VA_ARGS__)))
#define FIELD_RFL_EVAL(...) FIELD_RFL_EVAL4(FIELD_RFL_EVAL4(FIELD_RFL_EVAL4(__VA_ARGS__)))

#define FIELD_RFL_MAP_END(...)
#define FIELD_RFL_MAP_OUT
#define FIELD_RFL_MAP_COMMA ,

#define FIELD_RFL_MAP_GET_END2() 0, FIELD_RFL_MAP_END
#define FIELD_RFL_MAP_GET_END1(...) FIELD_RFL_MAP_GET_END2
#define FIELD_RFL_MAP_GET_END(...) FIELD_RFL_MAP_GET_END1
#define FIELD_RFL_MAP_NEXT0(test, next, ...) next FIELD_RFL_MAP_OUT
#define FIELD_RFL_MAP_NEXT1(test, next) FIELD_RFL_MAP_NEXT0(test, next, 0)
#define FIELD_RFL_MAP_NEXT(test, next) FIELD_RFL_MAP_NEXT1(FIELD_RFL_MAP_GET_END test, next)

#define FIELD_RFL_MAP0(f, x, peek, ...) f(x) FIELD_RFL_MAP_NEXT(peek, FIELD_RFL_MAP1)(f, peek, __VA_ARGS__)
#define FIELD_RFL_MAP1(f, x, peek, ...) f(x) FIELD_RFL_MAP_NEXT(peek, FIELD_RFL_MAP0)(f, peek, __VA_ARGS__)

#define FIELD_RFL_MAP_LIST_NEXT1(test, next) FIELD_RFL_MAP_NEXT0(test, FIELD_RFL_MAP_COMMA next, 0)
#define FIELD_RFL_MAP_LIST_NEXT(test, next) FIELD_RFL_MAP_LIST_NEXT1(FIELD_RFL_MAP_GET_END test, next)

#define FIELD_RFL_MAP_LIST0(f, x, peek, ...) \
    f(x) FIELD_RFL_MAP_LIST_NEXT(peek, FIELD_RFL_MAP_LIST1)(f, peek, __VA_ARGS__)
#define FIELD_RFL_MAP_LIST1(f, x, peek, ...) \
    f(x) FIELD_RFL_MAP_LIST_NEXT(peek, FIELD_RFL_MAP_LIST0)(f, peek, __VA_ARGS__)
#define FIELD_RFL_MAP(f, ...) FIELD_RFL_EVAL(FIELD_RFL_MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FIELD_RFL_MAP_LIST(f, ...) FIELD_RFL_EVAL(FIELD_RFL_MAP_LIST1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define FIELD_RFL_DECLTYPE(x) decltype(x)
#define FIELD_FORWARD(x) std::forward<decltype(x)>(x)

#define TO_TUPLE_TEMPLATE(NUM, ...)                                             \
    template <typename T, field_referenceable U = std::remove_cvref_t<T>>       \
    requires (field_count<U> == NUM)                                            \
    constexpr auto to_ref_tuple(T&& t)                                          \
    {                                                                           \
        auto& [__VA_ARGS__] = t;                                                \
        return std::tie(__VA_ARGS__);                                           \
    }                                                                           \
    template <typename T, field_referenceable U = std::remove_cvref_t<T>>       \
    requires (field_count<U> == NUM)                                            \
    constexpr auto to_tuple(T&& t)                                              \
    {                                                                           \
        auto [__VA_ARGS__] = std::forward<T>(t);                                \
        return std::tuple<FIELD_RFL_MAP_LIST(FIELD_RFL_DECLTYPE, __VA_ARGS__)>( \
            FIELD_RFL_MAP_LIST(FIELD_FORWARD, __VA_ARGS__));                    \
    }

        TO_TUPLE_TEMPLATE(1, p0)
        TO_TUPLE_TEMPLATE(2, p0, p1)
        TO_TUPLE_TEMPLATE(3, p0, p1, p2)
        TO_TUPLE_TEMPLATE(4, p0, p1, p2, p3)
        TO_TUPLE_TEMPLATE(5, p0, p1, p2, p3, p4)
        TO_TUPLE_TEMPLATE(6, p0, p1, p2, p3, p4, p5)
        TO_TUPLE_TEMPLATE(7, p0, p1, p2, p3, p4, p5, p6)
        TO_TUPLE_TEMPLATE(8, p0, p1, p2, p3, p4, p5, p6, p7)
        TO_TUPLE_TEMPLATE(9, p0, p1, p2, p3, p4, p5, p6, p7, p8)
        TO_TUPLE_TEMPLATE(10, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
        TO_TUPLE_TEMPLATE(11, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
        TO_TUPLE_TEMPLATE(12, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
        TO_TUPLE_TEMPLATE(13, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
        TO_TUPLE_TEMPLATE(14, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)
        TO_TUPLE_TEMPLATE(15, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)
        TO_TUPLE_TEMPLATE(16, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15)
        TO_TUPLE_TEMPLATE(17, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16)
        TO_TUPLE_TEMPLATE(18, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17)
        TO_TUPLE_TEMPLATE(19, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18)
        TO_TUPLE_TEMPLATE(20, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19)
        TO_TUPLE_TEMPLATE(21, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20)
        TO_TUPLE_TEMPLATE(22, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21)
        TO_TUPLE_TEMPLATE(23, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22)
        TO_TUPLE_TEMPLATE(24, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23)
        TO_TUPLE_TEMPLATE(25, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24)
        TO_TUPLE_TEMPLATE(26, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25)
        TO_TUPLE_TEMPLATE(27, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26)
        TO_TUPLE_TEMPLATE(28, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27)
        TO_TUPLE_TEMPLATE(29, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28)
        TO_TUPLE_TEMPLATE(30, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29)
        TO_TUPLE_TEMPLATE(31, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30)
        TO_TUPLE_TEMPLATE(32, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31)
        TO_TUPLE_TEMPLATE(33, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32)
        TO_TUPLE_TEMPLATE(34, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33)
        TO_TUPLE_TEMPLATE(35, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34)
        TO_TUPLE_TEMPLATE(36, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35)
        TO_TUPLE_TEMPLATE(37, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36)
        TO_TUPLE_TEMPLATE(38, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37)
        TO_TUPLE_TEMPLATE(39, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38)
        TO_TUPLE_TEMPLATE(40, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39)
        TO_TUPLE_TEMPLATE(41, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40)
        TO_TUPLE_TEMPLATE(42, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41)
        TO_TUPLE_TEMPLATE(43, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42)
        TO_TUPLE_TEMPLATE(44, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43)
        TO_TUPLE_TEMPLATE(45, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44)
        TO_TUPLE_TEMPLATE(46, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45)
        TO_TUPLE_TEMPLATE(47, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46)
        TO_TUPLE_TEMPLATE(48, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47)
        TO_TUPLE_TEMPLATE(49, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48)
        TO_TUPLE_TEMPLATE(50, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49)
        TO_TUPLE_TEMPLATE(51, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50)
        TO_TUPLE_TEMPLATE(52, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51)
        TO_TUPLE_TEMPLATE(53, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52)
        TO_TUPLE_TEMPLATE(54, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53)
        TO_TUPLE_TEMPLATE(55, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54)
        TO_TUPLE_TEMPLATE(56, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55)
        TO_TUPLE_TEMPLATE(57, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56)
        TO_TUPLE_TEMPLATE(58, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57)
        TO_TUPLE_TEMPLATE(59, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58)
        TO_TUPLE_TEMPLATE(60, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59)
        TO_TUPLE_TEMPLATE(61, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60)
        TO_TUPLE_TEMPLATE(62, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61)
        TO_TUPLE_TEMPLATE(63, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62)
        TO_TUPLE_TEMPLATE(64, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63)
        TO_TUPLE_TEMPLATE(65, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64)
        TO_TUPLE_TEMPLATE(66, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65)
        TO_TUPLE_TEMPLATE(67, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66)
        TO_TUPLE_TEMPLATE(68, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67)
        TO_TUPLE_TEMPLATE(69, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68)
        TO_TUPLE_TEMPLATE(70, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69)
        TO_TUPLE_TEMPLATE(71, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70)
        TO_TUPLE_TEMPLATE(72, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71)
        TO_TUPLE_TEMPLATE(73, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72)
        TO_TUPLE_TEMPLATE(74, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73)
        TO_TUPLE_TEMPLATE(75, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74)
        TO_TUPLE_TEMPLATE(76, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75)
        TO_TUPLE_TEMPLATE(77, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76)
        TO_TUPLE_TEMPLATE(78, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77)
        TO_TUPLE_TEMPLATE(79, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78)
        TO_TUPLE_TEMPLATE(80, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79)
        TO_TUPLE_TEMPLATE(81, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80)
        TO_TUPLE_TEMPLATE(82, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81)
        TO_TUPLE_TEMPLATE(83, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82)
        TO_TUPLE_TEMPLATE(84, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83)
        TO_TUPLE_TEMPLATE(85, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84)
        TO_TUPLE_TEMPLATE(86, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85)
        TO_TUPLE_TEMPLATE(87, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86)
        TO_TUPLE_TEMPLATE(88, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87)
        TO_TUPLE_TEMPLATE(89, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88)
        TO_TUPLE_TEMPLATE(90, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89)
        TO_TUPLE_TEMPLATE(91, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90)
        TO_TUPLE_TEMPLATE(92, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91)
        TO_TUPLE_TEMPLATE(93, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92)
        TO_TUPLE_TEMPLATE(94, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93)
        TO_TUPLE_TEMPLATE(95, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94)
        TO_TUPLE_TEMPLATE(96, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95)
        TO_TUPLE_TEMPLATE(97, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96)
        TO_TUPLE_TEMPLATE(98, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96, p97)
        TO_TUPLE_TEMPLATE(99, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96, p97, p98)
        TO_TUPLE_TEMPLATE(100, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96, p97, p98, p99)
#undef FIELD_RFL_EVAL0
#undef FIELD_RFL_EVAL1
#undef FIELD_RFL_EVAL2
#undef FIELD_RFL_EVAL3
#undef FIELD_RFL_EVAL4
#undef FIELD_RFL_EVAL
#undef FIELD_RFL_MAP_END
#undef FIELD_RFL_MAP_OUT
#undef FIELD_RFL_MAP_COMMA
#undef FIELD_RFL_MAP_GET_END2
#undef FIELD_RFL_MAP_GET_END1
#undef FIELD_RFL_MAP_GET_END
#undef FIELD_RFL_MAP_NEXT0
#undef FIELD_RFL_MAP_NEXT1
#undef FIELD_RFL_MAP_NEXT
#undef FIELD_RFL_MAP0
#undef FIELD_RFL_MAP1
#undef FIELD_RFL_MAP_LIST_NEXT1
#undef FIELD_RFL_MAP_LIST_NEXT
#undef FIELD_RFL_MAP_LIST0
#undef FIELD_RFL_MAP_LIST1
#undef FIELD_RFL_MAP
#undef FIELD_RFL_MAP_LIST
#undef FIELD_RFL_DECLTYPE
#undef FIELD_RFL_MOVE
#undef FIELD_RFL_TO_TUPLE_TEMPLATE
#pragma endregion TO_TUPLE_TEMPLATE_MACRO

        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        constexpr auto to_ref_tuple(T&&)
        {
            static_assert([] { return false; }(), "The supported maximum number of fields in struct must be <= 100.");
        }

        template <auto Ptr>
        [[nodiscard]] consteval std::string_view mangled_name()
        {
#if defined(__clang__) && defined(_WIN32)
            // clang-cl returns function_name() as __FUNCTION__ instead of __PRETTY_FUNCTION__
            return __PRETTY_FUNCTION__;
#else
            return std::source_location::current().function_name();
#endif
        }

        template <typename T>
        struct pointer_wrapper final
        {
            T* ptr;
        };

        template <size_t N, typename T>
        consteval auto cptr(T&& t) noexcept
        {
            decltype(auto) p = std::get<N>(to_ref_tuple(t));
            return pointer_wrapper<const std::decay_t<decltype(p)>>{&p};
        }

        template <auto Ptr>
        using nontype_template_parameter_helper = void;

        template <typename T>
        extern const T external;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif __GNUC__

#else

#endif

        template <typename T>  // clang-format off
        concept field_namable = field_countable<T> && (field_count<T> > 0 && !has_base<T>) && requires {
            typename nontype_template_parameter_helper<cptr<0>(external<T>)>;
        };  // clang-format on

        struct field_name_detector
        {
            void* dummy;
        };

        template <field_namable T, std::size_t N>
        consteval auto get_field_name() noexcept
        {
            constexpr auto mangled_dummy = mangled_name<cptr<0>(external<field_name_detector>)>();
            constexpr auto dummy_begin = mangled_dummy.rfind(std::string_view("dummy"));
            constexpr auto suffix = mangled_dummy.substr(dummy_begin + std::string_view("dummy").size());
            constexpr auto begin_sentinel = mangled_dummy[dummy_begin - 1];

            constexpr auto mangled_member_name = mangled_name<cptr<N>(external<T>)>();
            constexpr auto last = mangled_member_name.rfind(suffix);
            constexpr auto pos_sentinel = mangled_member_name.rfind(begin_sentinel, last);
            constexpr auto pos_colon = mangled_member_name.rfind(std::string_view(":"), last);  // for derived class
            constexpr auto begin = std::max(pos_sentinel + 1, pos_colon == std::string_view::npos ? 0 : pos_colon + 1);

            static_assert(begin < mangled_member_name.size());
            static_assert(last <= mangled_member_name.size());
            static_assert(begin < last);
            return mangled_member_name.substr(begin, last - begin);
        }

#if defined(__clang__)
#pragma clang diagnostic pop
#elif __GNUC__

#else

#endif
        template <typename T>
        using remove_rvalue_reference_t =
            std::conditional_t<std::is_rvalue_reference_v<T>, std::remove_reference_t<T>, T>;

        template <field_namable T, std::size_t N>
        constexpr std::string_view field_name = get_field_name<T, N>();

        template <field_referenceable T, std::size_t N>
        using field_type = remove_rvalue_reference_t<decltype(std::get<N>(to_tuple(std::declval<T&>())))>;

        template <std::size_t N, typename T, field_referenceable U = std::remove_cvref_t<T>>
        constexpr decltype(auto) get_field(T& t) noexcept
        {
            return std::get<N>(to_ref_tuple(t));
        }

        template <std::size_t N, typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires std::is_rvalue_reference_v<T&&>
        constexpr auto get_field(T&& t) noexcept
        {
            return std::get<N>(to_tuple(std::forward<T>(t)));
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is, field_namable U = std::remove_cvref_t<T1>>
        requires requires(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>) {
            (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)), ...);
        }
        void for_each_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)), ...);
        }

        template <typename T, typename Func, std::size_t... Is, field_namable U = std::remove_cvref_t<T>>
        requires requires(T&& t, Func&& func, std::index_sequence<Is...>) {
            (func(field_name<U, Is>, get_field<Is>(t)), ...);
        }
        void for_each_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            (func(field_name<U, Is>, get_field<Is>(t)), ...);
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is,
                  field_referenceable U = std::remove_cvref_t<T1>>
        requires requires(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>) {
            (func(get_field<Is>(t1), get_field<Is>(t2)), ...);
        }
        void for_each_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            (func(get_field<Is>(t1), get_field<Is>(t2)), ...);
        }

        template <typename T, typename Func, std::size_t... Is, field_referenceable U = std::remove_cvref_t<T>>
        requires requires(T&& t, Func&& func, std::index_sequence<Is...>) { (func(get_field<Is>(t)), ...); }
        void for_each_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            (func(get_field<Is>(t)), ...);
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is, field_namable U = std::remove_cvref_t<T1>>
        requires requires(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>) {
            (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) && ...);
        }
        bool all_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            return (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) && ...);
        }

        template <typename T, typename Func, std::size_t... Is, field_namable U = std::remove_cvref_t<T>>
        requires requires(T&& t, Func&& func, std::index_sequence<Is...>) {
            (func(field_name<U, Is>, get_field<Is>(t)) && ...);
        }
        bool all_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            return (func(field_name<U, Is>, get_field<Is>(t)) && ...);
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is,
                  field_referenceable U = std::remove_cvref_t<T1>>
        requires requires(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>) {
            (func(get_field<Is>(t1), get_field<Is>(t2)) && ...);
        }
        bool all_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            return (func(get_field<Is>(t1), get_field<Is>(t2)) && ...);
        }

        template <typename T, typename Func, std::size_t... Is, field_referenceable U = std::remove_cvref_t<T>>
        requires requires(T&& t, Func&& func, std::index_sequence<Is...>) { (func(get_field<Is>(t)) && ...); }
        bool all_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            return (func(get_field<Is>(t)) && ...);
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is, field_namable U = std::remove_cvref_t<T1>>
        requires requires(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>) {
            (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) || ...);
        }
        bool any_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            return (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) || ...);
        }

        template <typename T, typename Func, std::size_t... Is, field_namable U = std::remove_cvref_t<T>>
        requires requires(T&& t, Func&& func, std::index_sequence<Is...>) {
            (func(field_name<U, Is>, get_field<Is>(t)) || ...);
        }
        bool any_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            return (func(field_name<U, Is>, get_field<Is>(t)) || ...);
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is,
                  field_referenceable U = std::remove_cvref_t<T1>>
        requires requires(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>) {
            (func(get_field<Is>(t1), get_field<Is>(t2)) || ...);
        }
        bool any_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            return (func(get_field<Is>(t1), get_field<Is>(t2)) || ...);
        }

        template <typename T, typename Func, std::size_t... Is, field_referenceable U = std::remove_cvref_t<T>>
        requires requires(T&& t, Func&& func, std::index_sequence<Is...>) { (func(get_field<Is>(t)) || ...); }
        bool any_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            return (func(get_field<Is>(t)) || ...);
        }
    }  // namespace detail

    using detail::field_count;
    using detail::field_countable;
    using detail::field_namable;
    using detail::field_name;
    using detail::field_referenceable;
    using detail::field_type;
    using detail::get_field;
    using detail::to_tuple;

    template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
              field_referenceable U2 = std::remove_cvref_t<T2>>
    requires std::is_same_v<U1, U2>
    void for_each_field(T1&& t1, T2&& t2, Func&& func)
    {
        detail::for_each_field_impl(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                    std::make_index_sequence<field_count<U1>>());
    }

    template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
    void for_each_field(T&& t, Func&& func)
    {
        detail::for_each_field_impl(std::forward<T>(t), std::forward<Func>(func),
                                    std::make_index_sequence<field_count<U>>());
    }

    template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
              field_referenceable U2 = std::remove_cvref_t<T2>>
    requires std::is_same_v<U1, U2>
    bool all_of_field(T1&& t1, T2&& t2, Func&& func)
    {
        return detail::all_of_field_impl(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U1>>());
    }

    template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
    bool all_of_field(T&& t, Func&& func)
    {
        return detail::all_of_field_impl(std::forward<T>(t), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U>>());
    }

    template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
              field_referenceable U2 = std::remove_cvref_t<T2>>
    requires std::is_same_v<U1, U2>
    bool any_of_field(T1&& t1, T2&& t2, Func&& func)
    {
        return detail::any_of_field_impl(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U1>>());
    }

    template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
    bool any_of_field(T&& t, Func&& func)
    {
        return detail::any_of_field_impl(std::forward<T>(t), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U>>());
    }
}  // namespace field_reflection
