// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

#include "coveo/linq/linq_tests.h"

#include <coveo/enumerable.h>
#include <coveo/linq.h>
#include <coveo/test_framework.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

namespace coveo_tests {
namespace linq {
namespace detail {

// Implementation of equal with four iterators like in C++14
template<typename It1, typename It2>
bool equal(It1 ibeg1, It1 iend1, It2 ibeg2, It2 iend2) {
    return std::distance(ibeg1, iend1) == std::distance(ibeg2, iend2) &&
           std::equal(ibeg1, iend1, ibeg2);
};

}

// Run all tests for coveo::linq operators
void linq_tests()
{
    // from
    {
        const auto v = { 42, 23, 66 };
        const std::vector<int> expected = { 42, 23, 66 };
        using namespace coveo::linq;
        auto seq = from(v);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // from_range
    {
        const auto v = { 42, 23, 66 };
        const std::vector<int> expected = { 42, 23, 66 };
        using namespace coveo::linq;
        auto seq = from_range(std::begin(v), std::end(v));
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // from_int_range
    {
        const std::vector<int> expected = { 42, 43, 44, 45, 46, 47, 48 };
        using namespace coveo::linq;
        auto seq = from_int_range(42, 7);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // from_repeated
    {
        const std::vector<int> expected = { 42, 42, 42, 42, 42, 42, 42 };
        using namespace coveo::linq;
        auto seq = from_repeated(42, 7);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // aggregate
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        auto agg = from(v)
                 | aggregate([](int a, int b) { return a + b; });
        COVEO_ASSERT(agg == 131);
    }
    {
        const std::vector<int> ev;
        using namespace coveo::linq;
        COVEO_ASSERT_THROW(from(ev)
                         | aggregate([](int a, int b) { return a + b; }));
    }
    {
        const char STR[] = "world!";
        using namespace coveo::linq;
        auto agg = from(coveo::enumerate_array(STR, 6))
                 | aggregate(std::string("Hello, "),
                             [](const std::string& agg, char c) { return agg + c; });
        COVEO_ASSERT(agg == "Hello, world!");
    }
    {
        const char NUMS[] = "31337";
        using namespace coveo::linq;
        auto agg = from(coveo::enumerate_array(NUMS, 5))
                 | aggregate(std::string(""),
                             [](const std::string& agg, char c) { return agg + c; },
                             [](const std::string& agg) {
                                 std::istringstream iss(agg);
                                 int n = 0;
                                 iss >> n;
                                 return n;
                             });
        COVEO_ASSERT(agg == 31337);
    }

    // all
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT(from(v) | all([](int i) { return i > 11; }));
        COVEO_ASSERT(!(from(v) | all([](int i) { return i % 2 == 0; })));
    }

    // any
    {
        std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT(from(v) | any());
        v.clear();
        COVEO_ASSERT(!(from(v) | any()));
    }

    // average
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        auto avg_int = from(v)
                     | average([](int i) { return i; });
        COVEO_ASSERT(avg_int == 43);
        auto avg_dbl = from(v)
                     | average([](int i) { return double(i); });
        COVEO_ASSERT(avg_dbl >= 43.66 && avg_dbl < 43.67);
    }
    {
        const std::vector<int> ev;
        using namespace coveo::linq;
        COVEO_ASSERT_THROW(from(ev)
                         | average([](int i) { return i; }));
    }

    // concat
    {
        const std::vector<int> a = { 42, 23 };
        const std::vector<int> b = { 66, 67 };
        const std::vector<int> ab = { 42, 23, 66, 67, 11, 7 };
        using namespace coveo::linq;
        auto all = from(a)
                 | concat(b)
                 | concat(std::vector<int>{ 11, 7 });
        COVEO_ASSERT(detail::equal(std::begin(all), std::end(all),
                                   std::begin(ab), std::end(ab)));
        COVEO_ASSERT(all.size() == ab.size());
    }

    // contains
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT(from(v) | contains(23));
        auto eq_str_int = [](int i, const std::string& s) {
            std::ostringstream oss;
            oss << i;
            return oss.str() == s;
        };
        COVEO_ASSERT(from(v) | contains("23", eq_str_int));
    }

    // count
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v) | count()) == 3);
        COVEO_ASSERT((from(v) | count([](int i) { return i % 2 == 0; })) == 2);
    }

    // default_if_empty
    {
        const std::vector<int> v;
        const std::vector<int> v_def = { 0 };
        const std::vector<int> not_v = { 42 };
        using namespace coveo::linq;
        auto def = from(v)
                 | default_if_empty();
        COVEO_ASSERT(detail::equal(std::begin(def), std::end(def),
                                   std::begin(v_def), std::end(v_def)));
        auto def_n = from(v)
                   | default_if_empty(42);
        COVEO_ASSERT(detail::equal(std::begin(def_n), std::end(def_n),
                                   std::begin(not_v), std::end(not_v)));
    }

    // distinct
    {
        const std::vector<int> v = { 42, 23, 66, 42, 67, 66, 23, 11 };
        const std::vector<int> v_no_dup = { 42, 23, 66, 67, 11 };
        using namespace coveo::linq;
        auto no_dup = from(v)
                    | distinct();
        COVEO_ASSERT(detail::equal(std::begin(no_dup), std::end(no_dup),
                                   std::begin(v_no_dup), std::end(v_no_dup)));
        auto no_dup_2 = from(v)
                      | distinct([](int i, int j) { return i > j; });
        COVEO_ASSERT(detail::equal(std::begin(no_dup_2), std::end(no_dup_2),
                                   std::begin(v_no_dup), std::end(v_no_dup)));
    }

    // element_at
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v) | element_at(1)) == 23);
        COVEO_ASSERT_THROW(from(v) | element_at(3));
    }

    // element_at_or_default
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v) | element_at_or_default(1)) == 23);
        COVEO_ASSERT((from(v) | element_at_or_default(3)) == 0);
    }

    // except
    {
        const std::vector<int> v = { 42, 23, 66, 42, 23, 67, 11, 66, 7 };
        const std::vector<int> not_v = { 66, 23, 11 };
        const std::vector<int> res = { 42, 42, 67, 7 };
        using namespace coveo::linq;
        auto lres = from(v)
                  | except(not_v);
        COVEO_ASSERT(detail::equal(std::begin(lres), std::end(lres),
                                   std::begin(res), std::end(res)));
    }

    // first
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v)
                    | first()) == 42);
        COVEO_ASSERT((from(v)
                    | first([](int i) { return i % 2 != 0; })) == 23);
    }

    // first_or_default
    {
        const std::vector<int> v = { 42, 23, 66 };
        const std::vector<int> e;
        using namespace coveo::linq;
        COVEO_ASSERT((from(v)
                    | first_or_default()) == 42);
        COVEO_ASSERT((from(e)
                    | first_or_default()) == 0);
        COVEO_ASSERT((from(v)
                    | first_or_default([](int i) { return i > 60; })) == 66);
        COVEO_ASSERT((from(v)
                    | first_or_default([](int i) { return i > 100; })) == 0);
    }

    // group_by, group_values_by, group_by_and_fold, group_values_by_and_fold
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> odd_group = { 23, 11, 7 };
        const std::vector<int> even_group = { 42, 66 };
        using namespace coveo::linq;
        auto groups = from(v)
                    | group_by([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; });
        auto icurg = std::begin(groups);
        auto iendg = std::end(groups);
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::odd);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(odd_group), std::end(odd_group)));
        ++icurg;
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::even);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(even_group), std::end(even_group)));
        ++icurg;
        COVEO_ASSERT(icurg == iendg);
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> odd_group = { 23, 11, 7 };
        const std::vector<int> even_group = { 42, 66 };
        using namespace coveo::linq;
        auto groups = from(v)
                    | group_by([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                               coveo::linq::detail::greater<>());
        auto icurg = std::begin(groups);
        auto iendg = std::end(groups);
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::even);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(even_group), std::end(even_group)));
        ++icurg;
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::odd);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(odd_group), std::end(odd_group)));
        ++icurg;
        COVEO_ASSERT(icurg == iendg);
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> odd_group = { 230, 110, 70 };
        const std::vector<int> even_group = { 420, 660 };
        using namespace coveo::linq;
        auto groups = from(v)
                    | group_values_by([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                                      [](int i) { return i * 10; });
        auto icurg = std::begin(groups);
        auto iendg = std::end(groups);
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::odd);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(odd_group), std::end(odd_group)));
        ++icurg;
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::even);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(even_group), std::end(even_group)));
        ++icurg;
        COVEO_ASSERT(icurg == iendg);
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> odd_group = { 230, 110, 70 };
        const std::vector<int> even_group = { 420, 660 };
        using namespace coveo::linq;
        auto groups = from(v)
                    | group_values_by([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                                      [](int i) { return i * 10; },
                                      coveo::linq::detail::greater<>());
        auto icurg = std::begin(groups);
        auto iendg = std::end(groups);
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::even);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(even_group), std::end(even_group)));
        ++icurg;
        COVEO_ASSERT(icurg != iendg);
        COVEO_ASSERT(std::get<0>(*icurg) == oddity::odd);
        COVEO_ASSERT(detail::equal(std::begin(std::get<1>(*icurg)), std::end(std::get<1>(*icurg)),
                                   std::begin(odd_group), std::end(odd_group)));
        ++icurg;
        COVEO_ASSERT(icurg == iendg);
    }
#if 0
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> size_by_oddity_v = { 3, 2 };
        using namespace coveo::linq;
        auto res = from(v)
                 | group_by_and_fold([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                                     [](oddity, auto nums) { return std::distance(std::begin(nums), std::end(nums)); });
        COVEO_ASSERT(std::equal(std::begin(res), std::end(res),
                                std::begin(size_by_oddity_v), std::end(size_by_oddity_v)));
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> size_by_oddity_v = { 2, 3 };
        using namespace coveo::linq;
        auto res = from(v)
                 | group_by_and_fold([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                                     [](oddity, auto nums) { return std::distance(std::begin(nums), std::end(nums)); },
                                     std::greater<>());
        COVEO_ASSERT(std::equal(std::begin(res), std::end(res),
                                std::begin(size_by_oddity_v), std::end(size_by_oddity_v)));
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> somewhat_size_by_oddity_v = { 233, 422 };
        using namespace coveo::linq;
        auto res = from(v)
                 | group_values_by_and_fold([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                                            [](int i) { return i * 10; },
                                            [](oddity, auto nums) {
                                                return std::distance(std::begin(nums), std::end(nums)) + *std::begin(nums);
                                            });
        COVEO_ASSERT(std::equal(std::begin(res), std::end(res),
                                std::begin(somewhat_size_by_oddity_v), std::end(somewhat_size_by_oddity_v)));
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> v = { 42, 23, 66, 11, 7 };
        const std::vector<int> somewhat_size_by_oddity_v = { 422, 233 };
        using namespace coveo::linq;
        auto res = from(v)
                 | group_values_by_and_fold([](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; },
                                            [](int i) { return i * 10; },
                                            [](oddity, auto nums) {
                                                return std::distance(std::begin(nums), std::end(nums)) + *std::begin(nums);
                                            },
                                            std::greater<>());
        COVEO_ASSERT(std::equal(std::begin(res), std::end(res),
                                std::begin(somewhat_size_by_oddity_v), std::end(somewhat_size_by_oddity_v)));
    }
#endif

    // group_join
#if 0
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> out_v = { 42, 23, 66 };
        const std::vector<int> in_v = { 11, 7, 6, 66, 9, 22 };
        const std::vector<int> in_odd_v = { 11, 7, 9 };
        const std::vector<int> in_even_v = { 6, 66, 22 };
        const std::vector<std::tuple<int, const std::vector<int>&>> expected = {
            std::make_tuple(42, std::ref(in_even_v)),
            std::make_tuple(23, std::ref(in_odd_v)),
            std::make_tuple(66, std::ref(in_even_v)),
        };
        auto get_oddity = [](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; };
        using namespace coveo::linq;
        auto res = from(out_v)
                 | group_join(in_v, get_oddity, get_oddity,
                              [](int o, auto&& i_s) {
                                  return std::make_tuple(o, std::vector<int>(std::begin(i_s), std::end(i_s)));
                              });
        auto icurex = expected.cbegin();
        for (auto&& r : res) {
            COVEO_ASSERT(icurex != expected.cend());
            COVEO_ASSERT(std::get<0>(r) == std::get<0>(*icurex));
            auto&& exp_seq = std::get<1>(*icurex);
            auto&& act_seq = std::get<1>(r);
            COVEO_ASSERT(std::equal(std::begin(act_seq), std::end(act_seq),
                                    std::begin(exp_seq), std::end(exp_seq)));
            ++icurex;
        }
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> out_v = { 42, 23, 66 };
        const std::vector<int> in_v = { 11, 7, 6, 66, 9, 22 };
        const std::vector<int> in_odd_v = { 11, 7, 9 };
        const std::vector<int> in_even_v = { 6, 66, 22 };
        const std::vector<std::tuple<int, const std::vector<int>&>> expected = {
            std::make_tuple(42, std::ref(in_even_v)),
            std::make_tuple(23, std::ref(in_odd_v)),
            std::make_tuple(66, std::ref(in_even_v)),
        };
        auto get_oddity = [](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; };
        using namespace coveo::linq;
        auto res = from(out_v)
                 | group_join(in_v, get_oddity, get_oddity,
                              [](int o, auto&& i_s) {
                                  return std::make_tuple(o, std::vector<int>(std::begin(i_s), std::end(i_s)));
                              },
                              std::greater<>());
        auto icurex = expected.cbegin();
        for (auto&& r : res) {
            COVEO_ASSERT(icurex != expected.cend());
            COVEO_ASSERT(std::get<0>(r) == std::get<0>(*icurex));
            auto&& exp_seq = std::get<1>(*icurex);
            auto&& act_seq = std::get<1>(r);
            COVEO_ASSERT(std::equal(std::begin(act_seq), std::end(act_seq),
                                    std::begin(exp_seq), std::end(exp_seq)));
            ++icurex;
        }
    }
#endif

    // intersect
    {
        const std::vector<int> v1 = { 42, 23, 66, 11 };
        const std::vector<int> v2 = { 11, 7, 67, 42, 22 };
        const std::vector<int> expected = { 42, 11 };
        using namespace coveo::linq;
        auto intersection = from(v1)
                          | intersect(v2);
        COVEO_ASSERT(detail::equal(std::begin(intersection), std::end(intersection),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v1 = { 42, 23, 66, 11 };
        const std::vector<int> v2 = { 11, 7, 67, 42, 22 };
        const std::vector<int> expected = { 42, 11 };
        using namespace coveo::linq;
        auto intersection = from(v1)
                          | intersect(v2, coveo::linq::detail::greater<>());
        COVEO_ASSERT(detail::equal(std::begin(intersection), std::end(intersection),
                                   std::begin(expected), std::end(expected)));
    }

    // join
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> out_v = { 42, 23, 66 };
        const std::vector<int> in_v = { 11, 7, 6, 66, 9, 22 };
        const std::vector<std::tuple<int, int>> expected = {
            std::make_tuple(42, 6), std::make_tuple(42, 66), std::make_tuple(42, 22),
            std::make_tuple(23, 11), std::make_tuple(23, 7), std::make_tuple(23, 9),
            std::make_tuple(66, 6), std::make_tuple(66, 66), std::make_tuple(66, 22),
        };
        auto get_oddity = [](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; };
        using namespace coveo::linq;
        auto res = from(out_v)
                 | join(in_v, get_oddity, get_oddity,
                        [](int o, int i) { return std::make_tuple(o, i); });
        auto icurex = expected.cbegin();
        for (auto&& r : res) {
            COVEO_ASSERT(icurex != expected.cend());
            COVEO_ASSERT(r == *icurex);
            ++icurex;
        }
    }
    {
        enum class oddity { odd = 1, even = 2 };
        const std::vector<int> out_v = { 42, 23, 66 };
        const std::vector<int> in_v = { 11, 7, 6, 66, 9, 22 };
        const std::vector<std::tuple<int, int>> expected = {
            std::make_tuple(42, 6), std::make_tuple(42, 66), std::make_tuple(42, 22),
            std::make_tuple(23, 11), std::make_tuple(23, 7), std::make_tuple(23, 9),
            std::make_tuple(66, 6), std::make_tuple(66, 66), std::make_tuple(66, 22),
        };
        auto get_oddity = [](int i) { return i % 2 == 0 ? oddity::even : oddity::odd; };
        using namespace coveo::linq;
        auto res = from(out_v)
                 | join(in_v, get_oddity, get_oddity,
                        [](int o, int i) { return std::make_tuple(o, i); },
                        coveo::linq::detail::greater<>());
        auto icurex = expected.cbegin();
        for (auto&& r : res) {
            COVEO_ASSERT(icurex != expected.cend());
            COVEO_ASSERT(r == *icurex);
            ++icurex;
        }
    }

    // last
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v)
                    | last()) == 24);
        COVEO_ASSERT((from(v)
                    | last([](int i) { return i % 2 != 0; })) == 11);
    }
    {
        const std::forward_list<int> v = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v)
                    | last()) == 24);
        COVEO_ASSERT((from(v)
                    | last([](int i) { return i % 2 != 0; })) == 11);
    }

    // last_or_default
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> e;
        using namespace coveo::linq;
        COVEO_ASSERT((from(v)
                    | last_or_default()) == 24);
        COVEO_ASSERT((from(e)
                    | last_or_default()) == 0);
        COVEO_ASSERT((from(v)
                    | last_or_default([](int i) { return i > 30; })) == 66);
        COVEO_ASSERT((from(v)
                    | last_or_default([](int i) { return i > 100; })) == 0);
    }
    {
        const std::forward_list<int> v = { 42, 23, 66, 11, 24 };
        const std::forward_list<int> e;
        using namespace coveo::linq;
        COVEO_ASSERT((from(v)
                    | last_or_default()) == 24);
        COVEO_ASSERT((from(e)
                    | last_or_default()) == 0);
        COVEO_ASSERT((from(v)
                    | last_or_default([](int i) { return i > 30; })) == 66);
        COVEO_ASSERT((from(v)
                    | last_or_default([](int i) { return i > 100; })) == 0);
    }

    // max
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v) | max()) == 66);
        COVEO_ASSERT((from(v) | max([](int i) { return -i; })) == -11);
    }

    // min
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(v) | min()) == 11);
        COVEO_ASSERT((from(v) | min([](int i) { return -i; })) == -66);
    }

    // order_by/order_by_descending/then_by/then_by_descending
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> expected = { 11, 23, 24, 42, 66 };
        using namespace coveo::linq;
        auto seq = from(v)
                 | order_by([](int i) { return i; });
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> expected = { 66, 42, 24, 23, 11 };
        using namespace coveo::linq;
        auto seq = from(v)
                 | order_by([](int i) { return i; }, std::greater<int>());
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> expected = { 66, 42, 24, 23, 11 };
        using namespace coveo::linq;
        auto seq = from(v)
                 | order_by_descending([](int i) { return i; });
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> expected = { 11, 23, 24, 42, 66 };
        using namespace coveo::linq;
        auto seq = from(v)
                 | order_by_descending([](int i) { return i; }, std::greater<int>());
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<std::string> v = { "grape", "passionfruit", "banana", "mango",
                                             "orange", "raspberry", "apple", "blueberry" };
        const std::vector<std::string> expected = { "apple", "grape", "mango", "banana", "orange",
                                                    "blueberry", "raspberry", "passionfruit" };
        using namespace coveo::linq;
        auto seq = from(v)
                 | order_by([](const std::string& a) { return a.size(); })
                 | then_by([](const std::string& a) { return a; });
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<std::string> v = { "grape", "passionfruit", "banana", "mango",
                                             "orange", "raspberry", "apple", "blueberry" };
        const std::vector<std::string> expected = { "passionfruit", "raspberry", "blueberry",
                                                    "orange", "banana", "mango", "grape", "apple" };
        using namespace coveo::linq;
        auto seq = from(v)
                 | order_by_descending([](const std::string& a) { return a.size(); })
                 | then_by_descending([](const std::string& a) { return a; });
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // reverse
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> expected = { 24, 11, 66, 23, 42 };
        using namespace coveo::linq;
        auto seq = from(v)
                 | reverse();
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::forward_list<int> v = { 42, 23, 66, 11, 24 };
        const std::forward_list<int> expected = { 24, 11, 66, 23, 42 };
        using namespace coveo::linq;
        auto seq = from(v)
                 | reverse();
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // select/select_with_index/select_many/select_many_with_index
    {
        const std::vector<int> v = { 42, 23, 66 };
        const std::vector<std::string> expected = { "4242", "2323", "6666" };
        auto our_itoa = [](int i) -> std::string {
            std::ostringstream oss;
            oss << i;
            return oss.str();
        };
        auto our_dblstr = [](const std::string& s) -> std::string {
            return s + s;
        };
        using namespace coveo::linq;
        auto seq = from(v)
                 | select(our_itoa)
                 | select(our_dblstr);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v = { 42, 23, 66 };
        const std::vector<std::string> expected = { "43", "2525", "696969" };
        auto our_itoa = [](int i, std::size_t idx) -> std::string {
            std::ostringstream oss;
            oss << static_cast<std::size_t>(i) + idx + 1;
            return oss.str();
        };
        auto our_mulstr = [](const std::string& s, std::size_t idx) -> std::string {
            std::string r;
            for (std::size_t i = 0; i <= idx; ++i) {
                r += s;
            }
            return r;
        };
        using namespace coveo::linq;
        auto seq = from(v)
                 | select_with_index(our_itoa)
                 | select_with_index(our_mulstr);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v = { 42, 23, 66 };
        const std::vector<std::string> expected = { "42", "24", "23", "32", "66", "66" };
        auto our_itoa = [](int i) {
            std::vector<std::string> ov;
            std::ostringstream oss;
            oss << i;
            ov.push_back(oss.str());
            ov.push_back(oss.str());
            std::reverse(ov.back().begin(), ov.back().end());
            return coveo::enumerate_container(std::move(ov));
        };
        using namespace coveo::linq;
        auto seq = from(v)
                 | select_many(our_itoa);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }
    {
        const std::vector<int> v = { 42, 23, 66 };
        const std::vector<std::string> expected = { "43", "34", "25", "52", "69", "96" };
        auto our_itoa = [](int i, std::size_t idx) {
            std::vector<std::string> ov;
            std::ostringstream oss;
            oss << static_cast<std::size_t>(i) + idx + 1;
            ov.push_back(oss.str());
            ov.push_back(oss.str());
            std::reverse(ov.back().begin(), ov.back().end());
            return coveo::enumerate_container(std::move(ov));
        };
        using namespace coveo::linq;
        auto seq = from(v)
                 | select_many_with_index(our_itoa);
        COVEO_ASSERT(detail::equal(std::begin(seq), std::end(seq),
                                   std::begin(expected), std::end(expected)));
    }

    // sequence_equal
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::forward_list<int> expected = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        COVEO_ASSERT(from(v) | sequence_equal(expected));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::forward_list<int> expected = { -42, 23, -66, -11, 24 };
        auto fuzzy_equal = [](int i, int j) {
            return std::abs(i) == std::abs(j);
        };
        using namespace coveo::linq;
        COVEO_ASSERT(from(v) | sequence_equal(expected, fuzzy_equal));
    }

    // single
    {
        const std::vector<int> zero_v;
        const std::vector<int> one_v = { 42 };
        const std::vector<int> two_v = { 42, 23 };
        using namespace coveo::linq;
        COVEO_ASSERT_THROW(from(zero_v) | single());
        COVEO_ASSERT((from(one_v) | single()) == 42);
        COVEO_ASSERT_THROW(from(two_v) | single());
    }
    {
        const std::vector<int> zero_v;
        const std::vector<int> no_42_v = { 23, 66, 11 };
        const std::vector<int> one_42_v = { 42, 23, 66, 11 };
        const std::vector<int> two_42_v = { 42, 23, 66, 42, 11 };
        auto equal_to_42 = std::bind(std::equal_to<int>(), std::placeholders::_1, 42);
        using namespace coveo::linq;
        COVEO_ASSERT_THROW(from(zero_v) | single(equal_to_42));
        COVEO_ASSERT_THROW(from(no_42_v) | single(equal_to_42));
        COVEO_ASSERT((from(one_42_v) | single(equal_to_42)) == 42);
        COVEO_ASSERT_THROW(from(two_42_v) | single(equal_to_42));
    }

    // single_or_default
    {
        const std::vector<int> zero_v;
        const std::vector<int> one_v = { 42 };
        const std::vector<int> two_v = { 42, 23 };
        using namespace coveo::linq;
        COVEO_ASSERT((from(zero_v) | single_or_default()) == 0);
        COVEO_ASSERT((from(one_v) | single_or_default()) == 42);
        COVEO_ASSERT((from(two_v) | single_or_default()) == 0);
    }
    {
        const std::vector<int> zero_v;
        const std::vector<int> no_42_v = { 23, 66, 11 };
        const std::vector<int> one_42_v = { 42, 23, 66, 11 };
        const std::vector<int> two_42_v = { 42, 23, 66, 42, 11 };
        auto equal_to_42 = std::bind(std::equal_to<int>(), std::placeholders::_1, 42);
        using namespace coveo::linq;
        COVEO_ASSERT((from(zero_v) | single_or_default(equal_to_42)) == 0);
        COVEO_ASSERT((from(no_42_v) | single_or_default(equal_to_42)) == 0);
        COVEO_ASSERT((from(one_42_v) | single_or_default(equal_to_42)) == 42);
        COVEO_ASSERT((from(two_42_v) | single_or_default(equal_to_42)) == 0);
    }

    // skip
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> last_two = { 11, 24 };
        const std::vector<int> none;
        using namespace coveo::linq;
        auto e_skip_3 = from(v)
                      | skip(3);
        auto e_skip_9 = from(v)
                      | skip(9);
        COVEO_ASSERT(detail::equal(std::begin(e_skip_3), std::end(e_skip_3),
                                   std::begin(last_two), std::end(last_two)));
        COVEO_ASSERT(detail::equal(std::begin(e_skip_9), std::end(e_skip_9),
                                   std::begin(none), std::end(none)));
    }

    // skip_while/skip_while_with_index
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> v_66_and_up = { 66, 11, 24 };
        const std::vector<int> none;
        using namespace coveo::linq;
        auto e_after_60 = from(v)
                        | skip_while([](int i) { return i < 60; });
        auto e_after_90 = from(v)
                        | skip_while([](int i) { return i < 90; });
        COVEO_ASSERT(detail::equal(std::begin(e_after_60), std::end(e_after_60),
                                   std::begin(v_66_and_up), std::end(v_66_and_up)));
        COVEO_ASSERT(detail::equal(std::begin(e_after_90), std::end(e_after_90),
                                   std::begin(none), std::end(none)));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> v_66_and_up = { 66, 11, 24 };
        const std::vector<int> v_24_and_up = { 24 };
        using namespace coveo::linq;
        auto e_after_60 = from(v)
                        | skip_while_with_index([](int i, std::size_t n) { return i < 60 && n < 4; });
        auto e_after_90 = from(v)
                        | skip_while_with_index([](int i, std::size_t n) { return i < 90 && n < 4; });
        COVEO_ASSERT(detail::equal(std::begin(e_after_60), std::end(e_after_60),
                                   std::begin(v_66_and_up), std::end(v_66_and_up)));
        COVEO_ASSERT(detail::equal(std::begin(e_after_90), std::end(e_after_90),
                                   std::begin(v_24_and_up), std::end(v_24_and_up)));
    }

    // sum
    {
        const std::vector<int> v = { 42, 23, 66 };
        using namespace coveo::linq;
        auto sum_int = from(v)
                     | sum([](int i) { return i; });
        COVEO_ASSERT(sum_int == 131);
        auto sum_dbl = from(v)
                     | sum([](int i) { return double(i); });
        COVEO_ASSERT(sum_dbl >= 131.0 && sum_dbl < 131.01);
    }
    {
        const std::vector<int> ev;
        using namespace coveo::linq;
        COVEO_ASSERT_THROW(from(ev)
                         | sum([](int i) { return i; }));
    }

    // take
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> first_three = { 42, 23, 66 };
        const std::vector<int> none;
        using namespace coveo::linq;
        auto e_take_3 = from(v)
                      | take(3);
        auto e_take_0 = from(v)
                      | take(0);
        COVEO_ASSERT(detail::equal(std::begin(e_take_3), std::end(e_take_3),
                                   std::begin(first_three), std::end(first_three)));
        COVEO_ASSERT(detail::equal(std::begin(e_take_0), std::end(e_take_0),
                                   std::begin(none), std::end(none)));
    }

    // take_while/take_while_with_index
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> v_before_66 = { 42, 23 };
        using namespace coveo::linq;
        auto e_before_60 = from(v)
                        | take_while([](int i) { return i < 60; });
        auto e_before_90 = from(v)
                        | take_while([](int i) { return i < 90; });
        COVEO_ASSERT(detail::equal(std::begin(e_before_60), std::end(e_before_60),
                                   std::begin(v_before_66), std::end(v_before_66)));
        COVEO_ASSERT(detail::equal(std::begin(e_before_90), std::end(e_before_90),
                                   std::begin(v), std::end(v)));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::vector<int> v_before_66 = { 42, 23 };
        const std::vector<int> v_before_5th = { 42, 23, 66, 11 };
        using namespace coveo::linq;
        auto e_before_60 = from(v)
                        | take_while_with_index([](int i, std::size_t n) { return i < 60 && n < 4; });
        auto e_before_90 = from(v)
                        | take_while_with_index([](int i, std::size_t n) { return i < 90 && n < 4; });
        COVEO_ASSERT(detail::equal(std::begin(e_before_60), std::end(e_before_60),
                                   std::begin(v_before_66), std::end(v_before_66)));
        COVEO_ASSERT(detail::equal(std::begin(e_before_90), std::end(e_before_90),
                                   std::begin(v_before_5th), std::end(v_before_5th)));
    }

    // to/to_vector/to_associative/to_map
    {
        const std::vector<int> v = { 42, 23, 66, 11, 24 };
        const std::forward_list<int> fl = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        auto linq_fl = from(v)
                     | to<std::forward_list<int>>();
        COVEO_ASSERT(linq_fl == fl);
    }
    {
        const std::forward_list<int> fl = { 42, 23, 66, 11, 24 };
        using namespace coveo::linq;
        auto linq_v = from(fl)
                    | to_vector();
        COVEO_ASSERT(detail::equal(std::begin(linq_v), std::end(linq_v),
                                   std::begin(fl), std::end(fl)));
    }
    {
        const std::vector<std::pair<int, std::string>> v = { { 42, "Life" }, { 23, "Hangar" } };
        auto pair_first = [](std::pair<int, std::string> p) { return p.first; };
        using namespace coveo::linq;
        auto linq_m = from(v)
                    | to_associative<std::map<int, std::pair<int, std::string>>>(pair_first);
        auto it = std::begin(linq_m);
        COVEO_ASSERT(it->first == 23);
        COVEO_ASSERT(it->second.first == 23);
        COVEO_ASSERT(it->second.second == "Hangar");
        ++it;
        COVEO_ASSERT(it->first == 42);
        COVEO_ASSERT(it->second.first == 42);
        COVEO_ASSERT(it->second.second == "Life");
        COVEO_ASSERT(++it == std::end(linq_m));
    }
    {
        const std::vector<std::pair<int, std::string>> v = { { 42, "Life" }, { 23, "Hangar" } };
        auto pair_first = [](std::pair<int, std::string> p) { return p.first; };
        auto pair_second = [](std::pair<int, std::string> p) { return p.second; };
        using namespace coveo::linq;
        auto linq_m = from(v)
                    | to_associative<std::map<int, std::string>>(pair_first, pair_second);
        auto it = std::begin(linq_m);
        COVEO_ASSERT(it->first == 23);
        COVEO_ASSERT(it->second == "Hangar");
        ++it;
        COVEO_ASSERT(it->first == 42);
        COVEO_ASSERT(it->second == "Life");
        COVEO_ASSERT(++it == std::end(linq_m));
    }
    {
        const std::vector<std::pair<int, std::string>> v = { { 42, "Life" }, { 23, "Hangar" } };
        auto pair_first = [](std::pair<int, std::string> p) { return p.first; };
        using namespace coveo::linq;
        auto linq_m = from(v)
                    | to_map(pair_first);
        auto it = std::begin(linq_m);
        COVEO_ASSERT(it->first == 23);
        COVEO_ASSERT(it->second.first == 23);
        COVEO_ASSERT(it->second.second == "Hangar");
        ++it;
        COVEO_ASSERT(it->first == 42);
        COVEO_ASSERT(it->second.first == 42);
        COVEO_ASSERT(it->second.second == "Life");
        COVEO_ASSERT(++it == std::end(linq_m));
    }
    {
        const std::vector<std::pair<int, std::string>> v = { { 42, "Life" }, { 23, "Hangar" } };
        auto pair_first = [](std::pair<int, std::string> p) { return p.first; };
        auto pair_second = [](std::pair<int, std::string> p) { return p.second; };
        using namespace coveo::linq;
        auto linq_m = from(v)
                    | to_map(pair_first, pair_second);
        auto it = std::begin(linq_m);
        COVEO_ASSERT(it->first == 23);
        COVEO_ASSERT(it->second == "Hangar");
        ++it;
        COVEO_ASSERT(it->first == 42);
        COVEO_ASSERT(it->second == "Life");
        COVEO_ASSERT(++it == std::end(linq_m));
    }

    // union_with
    {
        const std::vector<int> v1 = { 42, 23, 66, 42, 67, 66, 23, 11 };
        const std::vector<int> v2 = { 11, 7, 67, 24, 44, 42, 44 };
        const std::vector<int> v_union = { 42, 23, 66, 67, 11, 7, 24, 44 };
        using namespace coveo::linq;
        auto union1 = from(v1)
                    | union_with(v2);
        COVEO_ASSERT(detail::equal(std::begin(union1), std::end(union1),
                                   std::begin(v_union), std::end(v_union)));
        auto union2 = from(v1)
                    | union_with(v2, [](int i, int j) { return i > j; });
        COVEO_ASSERT(detail::equal(std::begin(union2), std::end(union2),
                                   std::begin(v_union), std::end(v_union)));
    }

    // where/where_with_index
    {
        const std::vector<int> v = { 42, 23, 66, 11, 7, 24 };
        const std::vector<int> expected_odd = { 23, 11, 7 };
        const std::vector<int> expected_div_3 = { 42, 66, 24 };
        auto is_odd = [](int i) { return i % 2 != 0; };
        auto is_div_3 = [](int i) { return i % 3 == 0; };
        using namespace coveo::linq;
        auto e1 = from(v)
                | where(is_odd);
        COVEO_ASSERT(detail::equal(std::begin(e1), std::end(e1),
                                   std::begin(expected_odd), std::end(expected_odd)));
        auto e2 = from(v)
                | where(is_div_3);
        COVEO_ASSERT(detail::equal(std::begin(e2), std::end(e2),
                                   std::begin(expected_div_3), std::end(expected_div_3)));
    }
    {
        const std::vector<int> v = { 42, 23, 66, 11, 7, 24 };
        const std::vector<int> expected_odd_idx = { 23, 11, 24 };
        auto is_odd_idx = [](int, std::size_t idx) { return idx % 2 != 0; };
        using namespace coveo::linq;
        auto e = from(v)
               | where_with_index(is_odd_idx);
        COVEO_ASSERT(detail::equal(std::begin(e), std::end(e),
                                   std::begin(expected_odd_idx), std::end(expected_odd_idx)));
    }

    // zip
    {
        const std::vector<int> v1 = { 42, 23, 66 };
        const std::vector<int> v2 = { 11, 7, 24, 67 };
        const std::vector<int> expected = { 53, 30, 90 };
        auto add = [](int i, int j) { return i + j; };
        using namespace coveo::linq;
        auto zipped = from(v1)
                    | zip(v2, add);
        COVEO_ASSERT(detail::equal(std::begin(zipped), std::end(zipped),
                                   std::begin(expected), std::end(expected)));
    }
}

// Run tests of chaining LINQ operators.
void chaining_tests()
{
    struct student {
        uint32_t id;
        bool male;
        std::string first_name;
        std::string last_name;
    };
    struct course {
        uint32_t id;
        std::string name;
    };
    struct registration {
        uint32_t student_id;
        uint32_t course_id;
    };
    const std::vector<student> v_students = {
        { 1000, true, "John", "Peterson" },
        { 1001, false, "Lynn", "Sinclair" },
        { 1002, true, "Paul", "Rickman" },
        { 1003, true, "Robert", "McFly" },
    };
    const std::vector<course> v_courses {
        { 1000, "Chemistry 1" },
        { 1001, "Mathematics 1" },
        { 1002, "Chemistry Adv. 1" },
        { 1003, "History 2" },
        { 1004, "Social Studies" },
    };
    const std::vector<registration> v_registrations = {
        { 1000, 1001 },
        { 1000, 1003 },
        { 1001, 1000 },
        { 1001, 1001 },
        { 1001, 1004 },
        { 1002, 1001 },
        { 1002, 1002 },
        { 1002, 1003 },
        { 1003, 1003 },
        { 1003, 1004 },
    };

    using namespace coveo::linq;

    // Display courses males are registered to
    {
        struct streg { student stu; registration reg; };
        struct stcourse { student stu; course c; };
        auto seq = from(v_students)
                 | where([](const student& stu) { return stu.male; })
                 | join(v_registrations,
                        [](const student& stu) { return stu.id; },
                        [](const registration& reg) { return reg.student_id; },
                        [](const student& stu, const registration& reg) {
                            return streg { stu, reg };
                        })
                 | join(v_courses,
                        [](const streg& st_reg) { return st_reg.reg.course_id; },
                        [](const course& c) { return c.id; },
                        [](const streg& st_reg, const course& c) {
                            return stcourse { st_reg.stu, c };
                        })
                 | order_by([](const stcourse& st_c) { return st_c.stu.last_name; })
                 | then_by_descending([](const stcourse& st_c) { return st_c.c.name; });

        std::cout << std::endl;
        for (auto&& st_c : seq) {
            std::cout << st_c.stu.last_name << ", "
                      << st_c.stu.first_name << "\t"
                      << st_c.c.name << std::endl;
        }
        std::cout << std::endl;
    }

#if 0
    // Display how many students are registered to each course
    {
        auto seq = from(v_registrations)
                 | join(v_courses,
                        [](auto&& reg) { return reg.course_id; },
                        [](auto&& c) { return c.id; },
                        [](const registration& reg, const course& c) {
                            struct regcourse { course c; uint32_t stu_id; };
                            return regcourse { c, reg.student_id };
                        })
                 | group_values_by([](auto&& c_stid) { return c_stid.c; },
                                   [](auto&& c_stid) { return c_stid.stu_id; },
                                   [](auto&& course1, auto&& course2) { return course1.id < course2.id; })
                 | select([](auto&& c_stids) {
                              auto num = from(std::get<1>(c_stids))
                                       | count();
                              struct coursenumst { course c; std::size_t num_st; };
                              return coursenumst { std::get<0>(c_stids), num };
                          })
                 | order_by([](auto&& c_numst) { return c_numst.c.name; });

        std::cout << std::endl;
        for (auto&& c_numst : seq) {
            std::cout << c_numst.c.name << "\t" << c_numst.num_st << std::endl;
        }
        std::cout << std::endl;
    }
#endif
}

} // linq
} // namespace coveo_tests
