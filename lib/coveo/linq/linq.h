// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// C++ implementation of .NET's LINQ operators.

#ifndef COVEO_LINQ_H
#define COVEO_LINQ_H

#include "coveo/linq/exception.h"

#include <coveo/enumerable.h>
#include <coveo/linq/detail/linq_detail.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>

namespace coveo {
namespace linq {

// Entry point for the LINQ library. Use like this:
//
//   using namespace coveo::linq;
//   auto result = from(some_sequence)
//              >> linq_operator(...)
//              >> ...;
//
template<typename Seq>
decltype(auto) from(Seq&& seq_) {
    return std::forward<Seq>(seq_);
}

// Entry point for the LINQ library for a range
// delimited by two iterators. Use like this:
//
//   using namespace coveo::linq;
//   auto result = from_range(something.begin(), something.end())
//              >> linq_operator(...)
//              >> ...;
//
template<typename ItBeg, typename ItEnd>
auto from_range(ItBeg&& ibeg, ItEnd&& iend) {
    return enumerate_range(std::forward<ItBeg>(ibeg), std::forward<ItEnd>(iend));
}

// Entry point for the LINQ library for a range
// of integer values. Use like this:
//
//   using namespace coveo::linq;
//   auto result = from_int_range(1, 10)
//              >> linq_operator(...)
//              >> ...;
//
template<typename IntT>
auto from_int_range(IntT first, std::size_t count) {
    std::vector<std::decay_t<IntT>> vvalues;
    vvalues.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        vvalues.push_back(first++);
    }
    return coveo::enumerate_container(std::move(vvalues));
}

// Entry point for the LINQ library for a range
// of repeated values. Use like this:
//
//   using namespace coveo::linq;
//   auto result = from_repeated("Life", 7)
//              >> linq_operator(...)
//              >> ...;
//
template<typename T>
auto from_repeated(const T& value, std::size_t count) {
    std::vector<std::decay_t<T>> vvalues;
    vvalues.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        vvalues.push_back(value);
    }
    return coveo::enumerate_container(std::move(vvalues));
}

// Applies a LINQ operator to a sequence.
// Designed to allow chaining of LINQ operators as well as
// easy development of custom LINQ operators. Use like this:
//
//   using namespace coveo::linq;
//   auto result = from(some_sequence)
//              >> linq_op_1(...)
//              >> linq_op_2(...);
//
template<typename Seq, typename Op>
auto operator>>(Seq&& seq, Op&& op) -> decltype(op(std::forward<Seq>(seq))) {
    return op(std::forward<Seq>(seq));
}

// C++ LINQ operator: aggregate
// .NET equivalent: Aggregate

// Operator that aggregates all elements in a sequence using an aggregation function.
// Function receives previous aggregate and new element, must return new aggregate.
// Does not work on empty sequences.
template<typename F>
auto aggregate(const F& agg_f) {
    return [&agg_f](auto&& seq) {
        auto it = std::begin(seq);
        auto end = std::end(seq);
        if (it == end) {
            detail::throw_linq_empty_sequence();
        }
        auto aggregate(*it);
        for (++it; it != end; ++it) {
            aggregate = agg_f(aggregate, *it);
        }
        return aggregate;
    };
}

// Same thing with an initial value for the aggregate.
// Works with empty sequences because of this.
template<typename Acc, typename F>
auto aggregate(const Acc& seed, const F& agg_f) {
    return [&seed, &agg_f](auto&& seq) {
        Acc aggregate(seed);
        for (auto&& element : seq) {
            aggregate = agg_f(aggregate, element);
        }
        return aggregate;
    };
}

// Same thing with a result selector function that
// converts the final aggregate.
template<typename Acc, typename F, typename RF>
auto aggregate(const Acc& seed, const F& agg_f, const RF& result_f) {
    return [&seed, &agg_f, &result_f](auto&& seq) {
        return result_f(aggregate(seed, agg_f)(seq));
    };
}

// C++ LINQ operator: all
// .NET equivalent: All

// Operator that checks if all elements in a sequence satisfy a given predicate.
// Works on empty sequences (returns true in such a case).
template<typename Pred>
auto all(const Pred& pred) {
    return [&pred](auto&& seq) {
        return std::all_of(std::begin(seq), std::end(seq), pred);
    };
}

// C++ LINQ operator: any
// .NET equivalent: Any

// Operator that checks if a sequence has elements.
template<typename = void>
auto any() {
    return [](auto&& seq) {
        return std::begin(seq) != std::end(seq);
    };
}

// C++ LINQ operator: average
// .NET equivalent: Average

// Operator that computes the average of all elements in a sequence
// using a function to get a numerical value for each.
// Does not work on empty sequences.
template<typename F>
auto average(const F& num_f) {
    return [&num_f](auto&& seq) {
        auto it = std::begin(seq);
        auto end = std::end(seq);
        if (it == end) {
            detail::throw_linq_empty_sequence();
        }
        auto total = num_f(*it);
        decltype(total) count = 1;
        for (++it; it != end; ++it) {
            total += num_f(*it);
            ++count;
        }
        return total / count;
    };
}

// C++ LINQ operator: concat
// .NET equivalent: Concat

// Operator that concatenates the elements of two sequences.
// Sequences must have compatible elements for this to work.
template<typename Seq2>
auto concat(Seq2&& seq2) {
    return detail::concat_impl<Seq2>(std::forward<Seq2>(seq2));
}

// C++ LINQ operator: contains
// .NET equivalent: Contains

// Operator that determines if a sequence contains a specific element.
// Uses operator== to compare the elements.
template<typename T>
auto contains(const T& obj) {
    return [&obj](auto&& seq) {
        bool found = false;
        for (auto&& element : seq) {
            if (element == obj) {
                found = true;
                break;
            }
        }
        return found;
    };
}

// Same thing with a predicate to compare the elements.
// The predicate always receives obj as its second argument.
template<typename T, typename Pred>
auto contains(const T& obj, const Pred& pred) {
    return [&obj, &pred](auto&& seq) {
        bool found = false;
        for (auto&& element : seq) {
            if (pred(element, obj)) {
                found = true;
                break;
            }
        }
        return found;
    };
}

// C++ LINQ operator: count
// .NET equivalent: Count

// Operator that returns the number of elements in a sequence.
template<typename = void>
auto count() {
    return [](auto&& seq) {
        return static_cast<std::size_t>(std::distance(std::begin(seq), std::end(seq)));
    };
}

// Operator that returns the number of elements in a sequence
// that satisfy a given predicate.
template<typename Pred>
auto count(const Pred& pred) {
    return [&pred](auto&& seq) {
        return static_cast<std::size_t>(std::count_if(std::begin(seq), std::end(seq), pred));
    };
}

// C++ LINQ operator: default_if_empty
// .NET equivalent: DefaultIfEmpty

// Operator that returns a sequence or, if it's empty, a sequence
// containing a single, default element.
template<typename = void>
auto default_if_empty() {
    return [](auto&& seq) {
        coveo::enumerable<typename detail::seq_traits<decltype(seq)>::raw_value_type> e;
        if (from(seq) >> any()) {
            e = coveo::enumerate_container(seq);
        } else {
            e = coveo::enumerate_one(typename detail::seq_traits<decltype(seq)>::raw_value_type());
        }
        return e;
    };
}

// Same thing but with a specific default value if sequence is empty.
template<typename T>
auto default_if_empty(const T& obj) {
    return [&obj](auto&& seq) {
        coveo::enumerable<typename detail::seq_traits<decltype(seq)>::raw_value_type> e;
        if (from(seq) >> any()) {
            e = coveo::enumerate_container(seq);
        } else {
            e = coveo::enumerate_one(typename detail::seq_traits<decltype(seq)>::raw_value_type(obj));
        }
        return e;
    };
}

// C++ LINQ operator: distinct
// .NET equivalent: Distinct

// Operator that filters out duplicate elements in a sequence.
// Otherwise, returns elements in the same order.
template<typename = void>
auto distinct() {
    return detail::distinct_impl<std::less<>>(std::less<>());
}

// Same thing but with a predicate to compare the elements.
// The predicate must provide a strict ordering of elements, like std::less.
template<typename Pred>
auto distinct(Pred&& pred) {
    return detail::distinct_impl<Pred>(std::forward<Pred>(pred));
}

// C++ LINQ operator: element_at
// .NET equivalent: ElementAt

// Operator that returns the nth element in a sequence.
// Throws if sequence does not have enough elements.
template<typename = void>
auto element_at(std::size_t n) {
    return [n](auto&& seq) -> decltype(auto) {
        // #clp TODO optimize for bidirectional/random-access iterators
        auto icur = std::begin(seq);
        auto iend = std::end(seq);
        for (std::size_t i = 0; i < n && icur != iend; ++i, ++icur) {
        }
        if (icur == iend) {
            detail::throw_linq_out_of_range();
        }
        return *icur;
    };
}

// C++ LINQ operator: element_at_or_default
// .NET equivalent: ElementAtOrDefault

// Operator that returns the nth element in a sequence or,
// if the sequence does not have enough elements, a
// default-initialized value.
template<typename = void>
auto element_at_or_default(std::size_t n) {
    return [n](auto&& seq) {
        // #clp TODO optimize for bidirectional/random-access iterators
        auto icur = std::begin(seq);
        auto iend = std::end(seq);
        for (std::size_t i = 0; i < n && icur != iend; ++i, ++icur) {
        }
        return icur != iend ? *icur
            : typename detail::seq_traits<decltype(seq)>::raw_value_type();
    };
}

// C++ LINQ operator: except
// .NET equivalent: Except

// Operator that returns all elements that are in the first sequence
// but not in the second sequence (essentially a set difference).
template<typename Seq2>
auto except(Seq2&& seq2) {
    return detail::except_impl<Seq2, std::less<>>(std::forward<Seq2>(seq2), std::less<>());
}

// Same thing but with a predicate to compare the elements.
// The predicate must provide a strict ordering of the elements, like std::less.
template<typename Seq2, typename Pred>
auto except(Seq2&& seq2, Pred&& pred) {
    return detail::except_impl<Seq2, Pred>(std::forward<Seq2>(seq2), std::forward<Pred>(pred));
}

// C++ LINQ operqator: first
// .NET equivalent: First

// Operator that returns the first element in a sequence.
// Does not work on empty sequences.
template<typename = void>
auto first() {
    return [](auto&& seq) -> decltype(auto) {
        auto icur = std::begin(seq);
        if (icur == std::end(seq)) {
            detail::throw_linq_empty_sequence();
        }
        return *icur;
    };
}

// Operator that returns the first element in a sequence
// that satisfies a predicate. Does not work on empty sequences.
template<typename Pred>
auto first(const Pred& pred) {
    return [&pred](auto&& seq) -> decltype(auto) {
        auto icur = std::begin(seq);
        auto iend = std::end(seq);
        if (icur == iend) {
            detail::throw_linq_empty_sequence();
        }
        // #clp TODO what if sequence is sorted? Can we optimize?
        auto ifound = std::find_if(icur, iend, pred);
        if (ifound == iend) {
            detail::throw_linq_out_of_range();
        }
        return *ifound;
    };
}

// C++ LINQ operator: first_or_default
// .NET equivalent: FirstOrDefault

// Operator that returns the first element in a sequence
// or a default-initialized value if it's empty.
template<typename = void>
auto first_or_default() {
    return [](auto&& seq) {
        auto icur = std::begin(seq);
        return icur != std::end(seq) ? *icur
            : typename detail::seq_traits<decltype(seq)>::raw_value_type();
    };
}

// Operator that returns the first element in a sequence
// that satistifies a predicate or, if none are found,
// a default-initialized value.
template<typename Pred>
auto first_or_default(const Pred& pred) {
    return [&pred](auto&& seq) {
        auto iend = std::end(seq);
        auto ifound = std::find_if(std::begin(seq), iend, pred);
        return ifound != iend ? *ifound
            : typename detail::seq_traits<decltype(seq)>::raw_value_type();
    };
}

// C++ LINQ operators: group_by, group_values_by, group_by_and_fold, group_values_by_and_fold
// .NET equivalent: GroupBy

// Operator that groups elements in a sequence according
// to their keys, as returned by a key selector.
template<typename KeySelector>
auto group_by(KeySelector&& key_sel) {
    return detail::group_by_impl<KeySelector,
                                 detail::identity<>,
                                 detail::pair_of<>,
                                 std::less<>>(std::forward<KeySelector>(key_sel),
                                              detail::identity<>(),
                                              detail::pair_of<>(),
                                              std::less<>());
}

// Same thing but with a predicate used to compare keys.
template<typename KeySelector,
         typename Pred>
auto group_by(KeySelector&& key_sel,
              Pred&& pred)
{
    return detail::group_by_impl<KeySelector,
                                 detail::identity<>,
                                 detail::pair_of<>,
                                 Pred>(std::forward<KeySelector>(key_sel),
                                       detail::identity<>(),
                                       detail::pair_of<>(),
                                       std::forward<Pred>(pred));
}

// Operator that groups values according to keys, with both keys
// and values extracted from a sequence's elements using selectors.
template<typename KeySelector,
         typename ValueSelector>
auto group_values_by(KeySelector&& key_sel,
                     ValueSelector&& value_sel)
{
    return detail::group_by_impl<KeySelector,
                                 ValueSelector,
                                 detail::pair_of<>,
                                 std::less<>>(std::forward<KeySelector>(key_sel),
                                              std::forward<ValueSelector>(value_sel),
                                              detail::pair_of<>(),
                                              std::less<>());
}

// Same thing but with a predicate used to compare keys.
template<typename KeySelector,
         typename ValueSelector,
         typename Pred>
auto group_values_by(KeySelector&& key_sel,
                     ValueSelector&& value_sel,
                     Pred&& pred)
{
    return detail::group_by_impl<KeySelector,
                                 ValueSelector,
                                 detail::pair_of<>,
                                 Pred>(std::forward<KeySelector>(key_sel),
                                       std::forward<ValueSelector>(value_sel),
                                       detail::pair_of<>(),
                                       std::forward<Pred>(pred));
}

// Operator that groups a sequence's elements by keys as returned by
// a key selector, then folds each group into a final result using
// a result selector.
template<typename KeySelector,
         typename ResultSelector>
auto group_by_and_fold(KeySelector&& key_sel,
                       ResultSelector&& result_sel)
{
    return detail::group_by_impl<KeySelector,
                                 detail::identity<>,
                                 ResultSelector,
                                 std::less<>>(std::forward<KeySelector>(key_sel),
                                              detail::identity<>(),
                                              std::forward<ResultSelector>(result_sel),
                                              std::less<>());
}

// Same thing with a predicate to compare the keys.
template<typename KeySelector,
         typename ResultSelector,
         typename Pred>
auto group_by_and_fold(KeySelector&& key_sel,
                       ResultSelector&& result_sel,
                       Pred&& pred)
{
    return detail::group_by_impl<KeySelector,
                                 detail::identity<>,
                                 ResultSelector,
                                 Pred>(std::forward<KeySelector>(key_sel),
                                       detail::identity<>(),
                                       std::forward<ResultSelector>(result_sel),
                                       std::forward<Pred>(pred));
}

// Operator that creates groups of keys and values from a sequence's
// elements using selectors, then uses a result selector on each group
// to create the final results.
template<typename KeySelector,
         typename ValueSelector,
         typename ResultSelector>
auto group_values_by_and_fold(KeySelector&& key_sel,
                              ValueSelector&& value_sel,
                              ResultSelector&& result_sel)
{
    return detail::group_by_impl<KeySelector,
                                 ValueSelector,
                                 ResultSelector,
                                 std::less<>>(std::forward<KeySelector>(key_sel),
                                              std::forward<ValueSelector>(value_sel),
                                              std::forward<ResultSelector>(result_sel),
                                              std::less<>());
}

// Same thing with a predicate to compare the keys.
template<typename KeySelector,
         typename ValueSelector,
         typename ResultSelector,
         typename Pred>
auto group_values_by_and_fold(KeySelector&& key_sel,
                              ValueSelector&& value_sel,
                              ResultSelector&& result_sel,
                              Pred&& pred)
{
    return detail::group_by_impl<KeySelector,
                                 ValueSelector,
                                 ResultSelector,
                                 Pred>(std::forward<KeySelector>(key_sel),
                                       std::forward<ValueSelector>(value_sel),
                                       std::forward<ResultSelector>(result_sel),
                                       std::forward<Pred>(pred));
}

// C++ LINQ operator: group_join
// .NET equivalent: GroupJoin

// Operator that extracts keys from an outer and inner sequences using key selectors,
// then creates groups of elements from inner sequence matching keys of elements in
// outer sequence. A result selector is then used to convert each group into a final result.
template<typename InnerSeq,
         typename OuterKeySelector,
         typename InnerKeySelector,
         typename ResultSelector>
auto group_join(InnerSeq&& inner_seq,
                OuterKeySelector&& outer_key_sel,
                InnerKeySelector&& inner_key_sel,
                ResultSelector&& result_sel)
{
    return detail::group_join_impl<InnerSeq,
                                   OuterKeySelector,
                                   InnerKeySelector,
                                   ResultSelector,
                                   std::less<>>(std::forward<InnerSeq>(inner_seq),
                                                std::forward<OuterKeySelector>(outer_key_sel),
                                                std::forward<InnerKeySelector>(inner_key_sel),
                                                std::forward<ResultSelector>(result_sel),
                                                std::less<>());
}

// Same as above, with a predicate to compare the keys in both sequences.
template<typename InnerSeq,
         typename OuterKeySelector,
         typename InnerKeySelector,
         typename ResultSelector,
         typename Pred>
auto group_join(InnerSeq&& inner_seq,
                OuterKeySelector&& outer_key_sel,
                InnerKeySelector&& inner_key_sel,
                ResultSelector&& result_sel,
                Pred&& pred)
{
    return detail::group_join_impl<InnerSeq,
                                   OuterKeySelector,
                                   InnerKeySelector,
                                   ResultSelector,
                                   Pred>(std::forward<InnerSeq>(inner_seq),
                                         std::forward<OuterKeySelector>(outer_key_sel),
                                         std::forward<InnerKeySelector>(inner_key_sel),
                                         std::forward<ResultSelector>(result_sel),
                                         std::forward<Pred>(pred));
}

// C++ LINQ operator: intersect
// .NET equivalent: Intersect

// Operator that returns all elements that are found in two sequences.
// Essentially a set intersection.
template<typename Seq2>
auto intersect(Seq2&& seq2) {
    return detail::intersect_impl<Seq2, std::less<>>(std::forward<Seq2>(seq2),
                                                     std::less<>());
}

// Same as above, with a predicate to compare the elements.
// The predicate must provide a strict ordering of the elements, like std::less.
template<typename Seq2, typename Pred>
auto intersect(Seq2&& seq2, Pred&& pred) {
    return detail::intersect_impl<Seq2, Pred>(std::forward<Seq2>(seq2),
                                              std::forward<Pred>(pred));
}

// C++ LINQ operator: join
// .NET equivalent: Join

// Operator that extracts keys from elements in two sequences and joins
// elements with matching keys using a result selector, like a database JOIN.
template<typename InnerSeq,
         typename OuterKeySelector,
         typename InnerKeySelector,
         typename ResultSelector>
auto join(InnerSeq&& inner_seq,
          OuterKeySelector&& outer_key_sel,
          InnerKeySelector&& inner_key_sel,
          ResultSelector&& result_sel)
{
    return detail::join_impl<InnerSeq,
                             OuterKeySelector,
                             InnerKeySelector,
                             ResultSelector,
                             std::less<>>(std::forward<InnerSeq>(inner_seq),
                                          std::forward<OuterKeySelector>(outer_key_sel),
                                          std::forward<InnerKeySelector>(inner_key_sel),
                                          std::forward<ResultSelector>(result_sel),
                                          std::less<>());
}

// Same as above, with a predicate to compare the keys.
template<typename InnerSeq,
         typename OuterKeySelector,
         typename InnerKeySelector,
         typename ResultSelector,
         typename Pred>
auto join(InnerSeq&& inner_seq,
          OuterKeySelector&& outer_key_sel,
          InnerKeySelector&& inner_key_sel,
          ResultSelector&& result_sel,
          Pred&& pred)
{
    return detail::join_impl<InnerSeq,
                             OuterKeySelector,
                             InnerKeySelector,
                             ResultSelector,
                             Pred>(std::forward<InnerSeq>(inner_seq),
                                   std::forward<OuterKeySelector>(outer_key_sel),
                                   std::forward<InnerKeySelector>(inner_key_sel),
                                   std::forward<ResultSelector>(result_sel),
                                   std::forward<Pred>(pred));
}

// C++ LINQ operqator: last
// .NET equivalent: Last

// Operator that returns the last element in a sequence.
// Does not work on empty sequences.
template<typename = void>
auto last() {
    return detail::last_impl_0();
}

// Operator that returns the last element in a sequence
// that satisfies a predicate. Does not work on empty sequences.
template<typename Pred>
auto last(const Pred& pred) {
    return detail::last_impl_1<Pred>(pred);
}

// C++ LINQ operator: last_or_default
// .NET equivalent: LastOrDefault

// Operator that returns the last element in a sequence
// or a default-initialized value if it's empty.
template<typename = void>
auto last_or_default() {
    return detail::last_or_default_impl_0();
}

// Operator that returns the last element in a sequence
// that satistifies a predicate or, if none are found,
// a default-initialized value.
template<typename Pred>
auto last_or_default(const Pred& pred) {
    return detail::last_or_default_impl_1<Pred>(pred);
}

// C++ LINQ operator: max
// .NET equivalent: Max

// Operator that returns the maximum value in a sequence.
// Does not work on empty sequences.
template<typename = void>
auto max() {
    return [](auto&& seq) -> decltype(auto) {
        auto iend = std::end(seq);
        auto imax = std::max_element(std::begin(seq), iend);
        if (imax == iend) {
            detail::throw_linq_empty_sequence();
        }
        return *imax;
    };
}

// Operator that returns the maximum value in a sequence by
// projecting each element in the sequence into a different
// value using a selector. Does not work on empty sequences.
template<typename Selector>
auto max(const Selector& sel) {
    return [&sel](auto&& seq) {
        auto icur = std::begin(seq);
        auto iend = std::end(seq);
        if (icur == iend) {
            detail::throw_linq_empty_sequence();
        }
        auto max_val = sel(*icur);
        while (++icur != iend) {
            max_val = std::max(max_val, sel(*icur));
        }
        return max_val;
    };
}

// C++ LINQ operator: min
// .NET equivalent: Min

// Operator that returns the minimum value in a sequence.
// Does not work on empty sequences.
template<typename = void>
auto min() {
    return [](auto&& seq) -> decltype(auto) {
        auto iend = std::end(seq);
        auto imin = std::min_element(std::begin(seq), iend);
        if (imin == iend) {
            detail::throw_linq_empty_sequence();
        }
        return *imin;
    };
}

// Operator that returns the minimum value in a sequence by
// projecting each element in the sequence into a different
// value using a selector. Does not work on empty sequences.
template<typename Selector>
auto min(const Selector& sel) {
    return [&sel](auto&& seq) {
        auto icur = std::begin(seq);
        auto iend = std::end(seq);
        if (icur == iend) {
            detail::throw_linq_empty_sequence();
        }
        auto min_val = sel(*icur);
        while (++icur != iend) {
            min_val = std::min(min_val, sel(*icur));
        }
        return min_val;
    };
}

// C++ LINQ operators: order_by/order_by_descending/then_by/then_by_descending
// .NET equivalents: OrderBy/OrderByDesdending/ThenBy/ThenByDescending

// Operator that orders a sequence by fetching a key for each element
// using a key selector and then sorting elements by their keys using
// operator< to compare the keys.
template<typename KeySelector>
auto order_by(KeySelector&& key_sel) {
    typedef detail::order_by_comparator<KeySelector, std::less<>, false> comparator;
    return detail::order_by_impl<comparator>(std::make_unique<comparator>(std::forward<KeySelector>(key_sel), std::less<>()));
}

// As above, but uses the given predicate to compare the keys.
template<typename KeySelector, typename Pred>
auto order_by(KeySelector&& key_sel, Pred&& pred) {
    typedef detail::order_by_comparator<KeySelector, Pred, false> comparator;
    return detail::order_by_impl<comparator>(std::make_unique<comparator>(std::forward<KeySelector>(key_sel), std::forward<Pred>(pred)));
}

// As the first implementation above, but sorts keys descending,
// as if using operator>.
template<typename KeySelector>
auto order_by_descending(KeySelector&& key_sel) {
    typedef detail::order_by_comparator<KeySelector, std::less<>, true> comparator;
    return detail::order_by_impl<comparator>(std::make_unique<comparator>(std::forward<KeySelector>(key_sel), std::less<>()));
}

// As above, but uses the given predicate to compare the keys.
template<typename KeySelector, typename Pred>
auto order_by_descending(KeySelector&& key_sel, Pred&& pred) {
    typedef detail::order_by_comparator<KeySelector, Pred, true> comparator;
    return detail::order_by_impl<comparator>(std::make_unique<comparator>(std::forward<KeySelector>(key_sel), std::forward<Pred>(pred)));
}

// Operator that further orders a sequence previously ordered via order_by
// using a different key selector.
template<typename KeySelector>
auto then_by(KeySelector&& key_sel) {
    return order_by(std::forward<KeySelector>(key_sel));
}

// As above, but uses the given predicate to compare the new keys.
template<typename KeySelector, typename Pred>
auto then_by(KeySelector&& key_sel, Pred&& pred) {
    return order_by(std::forward<KeySelector>(key_sel), std::forward<Pred>(pred));
}

// Operator that further orders a sequence previously ordered via order_by
// using a different key selector, but in descending order
template<typename KeySelector>
auto then_by_descending(KeySelector&& key_sel) {
    return order_by_descending(std::forward<KeySelector>(key_sel));
}

// As above, but uses the given predicate to compare the new keys.
template<typename KeySelector, typename Pred>
auto then_by_descending(KeySelector&& key_sel, Pred&& pred) {
    return order_by_descending(std::forward<KeySelector>(key_sel), std::forward<Pred>(pred));
}

// C++ LINQ operator: reverse
// .NET equivalent: Reverse

// Operator that reverses the elements of a sequence.
template<typename = void>
auto reverse() {
    return detail::reverse_impl();
}

// C++ LINQ operators: select/select_with_index/select_many/select_many_with_index
// .NET equivalents: Select/SelectMany

// Operator that projects each element in a sequence into another form
// using a selector function, a little like std::transform.
template<typename Selector>
auto select(Selector&& sel) {
    return detail::select_impl<detail::indexless_selector_proxy<Selector>>(
        detail::indexless_selector_proxy<Selector>(std::forward<Selector>(sel)));
}

// As above, but selector also receives the index of each element in the sequence.
template<typename Selector>
auto select_with_index(Selector&& sel) {
    return detail::select_impl<Selector>(std::forward<Selector>(sel));
}

// Operator that projects each element in a sequence into a sequence of
// elements in another form using a selector function then flattens all
// those sequences into one.
template<typename Selector>
auto select_many(Selector&& sel) {
    return detail::select_many_impl<detail::indexless_selector_proxy<Selector>>(
        detail::indexless_selector_proxy<Selector>(std::forward<Selector>(sel)));
}

// As above, but selector also receives the index of each element in the sequence.
template<typename Selector>
auto select_many_with_index(Selector&& sel) {
    return detail::select_many_impl<Selector>(std::forward<Selector>(sel));
}

// C++ LINQ operator: sequence_equal
// .NET equivalent: SequenceEqual

// Operator that verifies if two sequences contain the same elements.
// Uses operator== to compare the elements.
template<typename Seq2>
auto sequence_equal(const Seq2& seq2) {
    return [&seq2](auto&& seq) {
        return std::equal(std::begin(seq), std::end(seq), std::begin(seq2), std::end(seq2));
    };
}

// As above, but uses the given predicate to compare the elements.
template<typename Seq2, typename Pred>
auto sequence_equal(const Seq2& seq2, const Pred& pred) {
    return [&seq2, &pred](auto&& seq) {
        return std::equal(std::begin(seq), std::end(seq), std::begin(seq2), std::end(seq2), pred);
    };
}

// C++ LINQ operator: single
// .NET equivalent: Single

// Operator that returns the single value in the given sequence.
// Throws an exception if the sequence is empty or if it has
// more than one value.
template<typename = void>
auto single() {
    return [](auto&& seq) -> decltype(auto) {
        auto ifirst = std::begin(seq);
        auto iend = std::end(seq);
        if (ifirst == iend) {
            detail::throw_linq_empty_sequence();
        }
        auto inext = ifirst;
        ++inext;
        if (inext != iend) {
            detail::throw_linq_out_of_range();
        }
        return *ifirst;
    };
}

// Operator that returns the single value in the given sequence
// that satisfies the given predicate. Throws an exception if the
// sequence contains no such element or more than one such element.
template<typename Pred>
auto single(const Pred& pred) {
    return [&pred](auto&& seq) -> decltype(auto) {
        auto ibeg = std::begin(seq);
        auto iend = std::end(seq);
        if (ibeg == iend) {
            detail::throw_linq_empty_sequence();
        }
        auto ifound = std::find_if(ibeg, iend, pred);
        if (ifound == iend) {
            detail::throw_linq_out_of_range();
        }
        auto inext = ifound;
        ++inext;
        auto ifoundagain = std::find_if(inext, iend, pred);
        if (ifoundagain != iend) {
            detail::throw_linq_out_of_range();
        }
        return *ifound;
    };
}

// C++ LINQ operator: single_or_default
// .NET equivalent: SingleOrDefault

// Operator that returns the single value in the given sequence,
// or a default-initialized value if the sequence is empty or if
// it has more than one element.
template<typename = void>
auto single_or_default() {
    return [](auto&& seq) {
        auto icur = std::begin(seq);
        auto iend = std::end(seq);
        if (icur != iend) {
            auto inext = icur;
            ++inext;
            if (inext != iend) {
                icur = iend;
            }
        }
        return icur != iend ? *icur
            : typename detail::seq_traits<decltype(seq)>::raw_value_type();
    };
}

// Operator that returns the single value in the given sequence
// that satisfies the given predicate, or a default-initialized value
// if the sequence contains no such element or more than one such element.
template<typename Pred>
auto single_or_default(const Pred& pred) {
    return [&pred](auto&& seq) {
        auto iend = std::end(seq);
        auto ifound = std::find_if(std::begin(seq), iend, pred);
        if (ifound != iend) {
            auto inext = ifound;
            ++inext;
            auto ifoundagain = std::find_if(inext, iend, pred);
            if (ifoundagain != iend) {
                ifound = iend;
            }
        }
        return ifound != iend ? *ifound
            : typename detail::seq_traits<decltype(seq)>::raw_value_type();
    };
}

// C++ LINQ operator: skip
// .NET equivalent: Skip

// Operator that skips the X first elements of a sequence.
// If the sequence contains less than X elements, returns
// an empty sequence.
template<typename = void>
auto skip(std::size_t n) {
    auto n_pred = [n](auto&&, std::size_t idx) {
        return idx < n;
    };
    return detail::skip_impl<decltype(n_pred)>(std::move(n_pred));
}

// C++ LINQ operators: skip_while, skip_while_with_index
// .NET equivalent: SkipWhile

// Operator that skips all first elements of a sequence
// that satisfy a given predicate. If the sequence contains
// only elements that satisfy the predicate, returns an
// empty sequence.
template<typename Pred>
auto skip_while(Pred&& pred) {
    return detail::skip_impl<detail::indexless_selector_proxy<Pred>>(
        detail::indexless_selector_proxy<Pred>(std::forward<Pred>(pred)));
}

// As above, but the predicate receives the index of the
// element in the sequence as second argument.
template<typename Pred>
auto skip_while_with_index(Pred&& pred) {
    return detail::skip_impl<Pred>(std::forward<Pred>(pred));
}

// C++ LINQ operator: sum
// .NET equivalent: Sum

// Operator that computes the sum of all elements in a sequence
// using a function to get a numerical value for each.
// Does not work on empty sequences.
template<typename F>
auto sum(const F& num_f) {
    return [&num_f](auto&& seq) {
        auto it = std::begin(seq);
        auto end = std::end(seq);
        if (it == end) {
            detail::throw_linq_empty_sequence();
        }
        auto total = num_f(*it);
        for (++it; it != end; ++it) {
            total += num_f(*it);
        }
        return total;
    };
}

// C++ LINQ operator: take
// .NET equivalent: Take

// Operator that returns the X first elements of a sequence.
// If the sequence contains less than X elements, returns
// as much as possible.
template<typename = void>
auto take(std::size_t n) {
    auto n_pred = [n](auto&&, std::size_t idx) {
        return idx < n;
    };
    return detail::take_impl<decltype(n_pred)>(std::move(n_pred));
}

// C++ LINQ operators: take_while, take_while_with_index
// .NET equivalent: TakeWhile

// Operator that returns all first elements of a sequence
// that satisfy a given predicate. If the sequence contains
// no elements that satisfy the predicate, returns an
// empty sequence.
template<typename Pred>
auto take_while(Pred&& pred) {
    return detail::take_impl<detail::indexless_selector_proxy<Pred>>(
        detail::indexless_selector_proxy<Pred>(std::forward<Pred>(pred)));
}

// As above, but the predicate receives the index of the
// element in the sequence as second argument.
template<typename Pred>
auto take_while_with_index(Pred&& pred) {
    return detail::take_impl<Pred>(std::forward<Pred>(pred));
}

// C++ LINQ operators: to, to_vector, to_associative, to_map
// .NET equivalents: ToArray, ToDictionary, ToList, ToLookup

// Operator that converts a sequence into a container of the given type.
template<typename Container>
auto to() {
    return [](auto&& seq) {
        return Container(std::begin(seq), std::end(seq));
    };
}

// Specialized version of the above for std::vector.
// Auto-detects the element type from the sequence.
template<typename = void>
auto to_vector() {
    return [](auto&& seq) {
        return std::vector<typename detail::seq_traits<decltype(seq)>::raw_value_type>(
            std::begin(seq), std::end(seq));
    };
}

// Operator that converts a sequence into a specific type
// of associative container (like std::map), using a key
// selector function to fetch a key for each element.
// Associative type must support the insert_or_assign function.
template<typename Container,
         typename KeySelector>
auto to_associative(const KeySelector& key_sel) {
    return [&key_sel](auto&& seq) {
        Container c;
        for (auto&& elem : seq) {
            // #clp TODO replace this with insert_or_assign once available
            auto key = key_sel(elem);
            auto insert_res = c.insert(std::make_pair(key, elem));
            if (!insert_res.second) {
                insert_res.first->second = elem;
            }
        }
        return c;
    };
}

// As above, but using an element selector to convert elements
// inserted as mapped values.
template<typename Container,
         typename KeySelector,
         typename ElementSelector>
auto to_associative(const KeySelector& key_sel,
                    const ElementSelector& elem_sel)
{
    return [&key_sel, &elem_sel](auto&& seq) {
        Container c;
        for (auto&& elem : seq) {
            // #clp TODO replace this with insert_or_assign once available
            auto key = key_sel(elem);
            auto mapped = elem_sel(elem);
            auto insert_res = c.insert(std::make_pair(key, mapped));
            if (!insert_res.second) {
                insert_res.first->second = mapped;
            }
        }
        return c;
    };
}

// Specialized version of the above that returns an std::map.
// Auto-detects the key and mapped types from the sequence and selector.
template<typename KeySelector>
auto to_map(const KeySelector& key_sel) {
    return [&key_sel](auto&& seq) {
        std::map<std::decay_t<decltype(key_sel(*std::begin(seq)))>,
                 typename detail::seq_traits<decltype(seq)>::raw_value_type> m;
        for (auto&& elem : seq) {
            // #clp TODO replace this with insert_or_assign once available
            auto key = key_sel(elem);
            auto insert_res = m.insert(std::make_pair(key, elem));
            if (!insert_res.second) {
                insert_res.first->second = elem;
            }
        }
        return m;
    };
}

// As above, but using an element selector to convert elements
// inserted as mapped values.
template<typename KeySelector,
         typename ElementSelector>
auto to_map(const KeySelector& key_sel,
            const ElementSelector& elem_sel)
{
    return [&key_sel, &elem_sel](auto&& seq) {
        std::map<std::decay_t<decltype(key_sel(*std::begin(seq)))>,
                 std::decay_t<decltype(elem_sel(*std::begin(seq)))>> m;
        for (auto&& elem : seq) {
            // #clp TODO replace this with insert_or_assign once available
            auto key = key_sel(elem);
            auto mapped = elem_sel(elem);
            auto insert_res = m.insert(std::make_pair(key, mapped));
            if (!insert_res.second) {
                insert_res.first->second = mapped;
            }
        }
        return m;
    };
}

// C++ LINQ operator: union_with
// .NET equivalent: Union

// Operator that returns all elements that are found in either of two sequences,
// excluding duplicates, Essentially a set union.
template<typename Seq2>
auto union_with(Seq2&& seq2) {
    return detail::union_impl<Seq2, std::less<>>(std::forward<Seq2>(seq2),
                                                 std::less<>());
}

// Same as above, with a predicate to compare the elements.
// The predicate must provide a strict ordering of the elements, like std::less.
template<typename Seq2, typename Pred>
auto union_with(Seq2&& seq2, Pred&& pred) {
    return detail::union_impl<Seq2, Pred>(std::forward<Seq2>(seq2),
                                          std::forward<Pred>(pred));
}

// C++ LINQ operators: where, where_with_index
// .NET equivalent: Where

// Operator that only returns elements of a sequence that
// satisfy a given predicate.
template<typename Pred>
auto where(Pred&& pred) {
    return detail::where_impl<detail::indexless_selector_proxy<Pred>>(
        detail::indexless_selector_proxy<Pred>(std::forward<Pred>(pred)));
}

// As above, but the predicate receives the index of the
// element as second argument.
template<typename Pred>
auto where_with_index(Pred&& pred) {
    return detail::where_impl<Pred>(std::forward<Pred>(pred));
}

// C++ LINQ operator: zip
// .NET equivalent: Zip

// Operator that iterates two sequences and passes elements with
// the same index to a result selector function, producing a sequence
// of the results. The resulting sequence will be as long as the
// shortest of the two input sequences.
template<typename Seq2,
         typename ResultSelector>
auto zip(Seq2&& seq2, ResultSelector&& result_sel) {
    return detail::zip_impl<Seq2, ResultSelector>(std::forward<Seq2>(seq2),
                                                  std::forward<ResultSelector>(result_sel));
}

} // linq
} // coveo

#endif // COVEO_LINQ_H
