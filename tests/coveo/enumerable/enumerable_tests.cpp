// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

#include "coveo/enumerable/enumerable_tests.h"

#include <coveo/enumerable.h>
#include <coveo/test_framework.h>

#include <list>
#include <vector>

namespace coveo_tests {
namespace enumerable {
namespace detail {

// Compares enumerable sequence with content of container
template<typename T, typename C>
void validate_sequence(const coveo::enumerable<T>& seq, const C& expected, bool fast_size) {
    auto eit = std::begin(expected);
    auto eend = std::end(expected);
    for (auto&& obj : seq) {
        COVEO_ASSERT(eit != eend);
        COVEO_ASSERT(obj == *eit++);
    }
    COVEO_ASSERT(eit == eend);
    COVEO_ASSERT(seq.has_fast_size() == fast_size);
    COVEO_ASSERT(seq.size() == expected.size());
}

// Simple struct that cannot be copied.
struct no_copy {
    int i_;
    no_copy(int i)
        : i_(i) { }
    no_copy(const no_copy&) = delete;
    no_copy& operator=(const no_copy&) = delete;
    bool operator==(const no_copy& obj) const {
        return i_ == obj.i_;
    }
};

// Vector whose iterators don't return references
template<typename T>
class noref_vector {
    std::vector<T> v_;
public:
    class const_iterator
    {
        typename std::vector<T>::const_iterator it_;
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using pointer = const T*;
        using reference = T;
        using difference_type = std::ptrdiff_t;

        const_iterator()
            : it_() { }
        explicit const_iterator(const typename std::vector<T>::const_iterator& it)
            : it_(it) { }

        reference operator*() const {
            return *it_;
        }

        const_iterator& operator++() {
            ++it_;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator before(*this);
            ++*this;
            return before;
        }

        friend bool operator==(const const_iterator& left, const const_iterator& right) {
            return left.it_ == right.it_;
        }
        friend bool operator!=(const const_iterator& left, const const_iterator& right) {
            return left.it_ != right.it_;
        }
    };

public:
    noref_vector()
        : v_() { }
    noref_vector(std::initializer_list<T> ilist)
        : v_(ilist) { }

    void push_back(const T& obj) { v_.push_back(obj); }
    void push_back(T&& obj) { v_.push_back(std::move(obj)); }

    const_iterator begin() const { return const_iterator(v_.cbegin()); }
    const_iterator end() const { return const_iterator(v_.cend()); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    std::size_t size() const { return v_.size(); }
    bool empty() const { return v_.empty(); }
};

} // detail

// Runs all tests for coveo::enumerable
void enumerable_tests()
{
    // empty sequence
    {
        std::vector<int> vempty;
        auto empty_seq = coveo::enumerable<int>::empty();
        detail::validate_sequence(empty_seq, vempty, true);
        auto empty_cseq = empty_seq.as_const();
        detail::validate_sequence(empty_cseq, vempty, true);
    }
    {
        const std::vector<int> vempty;
        auto empty_seq = coveo::enumerable<const int>::empty();
        detail::validate_sequence(empty_seq, vempty, true);
    }

    // sequence defined by next delegate
    {
        std::vector<int> vi = { 42 };
        auto seq_i = coveo::enumerable<int>([](std::unique_ptr<int>& upi) -> int* {
            if (upi == nullptr) {
                upi.reset(new int(42));
                return upi.get();
            } else {
                return nullptr;
            }
        });
        detail::validate_sequence(seq_i, vi, false);
        auto seq_ci = seq_i.as_const();
        detail::validate_sequence(seq_ci, vi, false);
    }
    {
        const std::vector<int> vi = { 42 };
        auto seq_i = coveo::enumerable<const int>([](std::unique_ptr<int>& upi) -> const int* {
            if (upi == nullptr) {
                upi.reset(new int(42));
                return upi.get();
            } else {
                return nullptr;
            }
        });
        detail::validate_sequence(seq_i, vi, false);
    }

    // sequence of one element
    {
        std::vector<int> vone = { 42 };
        auto seq_one = coveo::enumerable<int>::for_one(42);
        detail::validate_sequence(seq_one, vone, true);
        auto seq_cone = seq_one.as_const();
        detail::validate_sequence(seq_cone, vone, true);
    }
    {
        std::vector<int> vone = { 42 };
        auto seq_one = coveo::enumerate_one(42);
        detail::validate_sequence(seq_one, vone, true);
    }
    {
        const std::vector<int> vone = { 42 };
        auto seq_one = coveo::enumerable<const int>::for_one(42);
        detail::validate_sequence(seq_one, vone, true);
    }
    {
        const int i = 42;
        const std::vector<int> vone = { 42 };
        auto seq_one = coveo::enumerate_one(i);
        detail::validate_sequence(seq_one, vone, true);
    }
    
    // sequence of one element held by ref
    {
        int hangar = 23;
        std::vector<int> vone = { 23 };
        auto seq_one_ref = coveo::enumerable<int>::for_one_ref(hangar);
        detail::validate_sequence(seq_one_ref, vone, true);
        auto seq_cone_ref = seq_one_ref.as_const();
        detail::validate_sequence(seq_cone_ref, vone, true);
    }
    {
        int hangar = 23;
        std::vector<int> vone = { 23 };
        auto seq_one_ref = coveo::enumerate_one_ref(hangar);
        detail::validate_sequence(seq_one_ref, vone, true);
    }
    {
        const int hangar = 23;
        const std::vector<int> vone = { 23 };
        auto seq_one_ref = coveo::enumerable<const int>::for_one_ref(hangar);
        detail::validate_sequence(seq_one_ref, vone, true);
    }
    {
        const int hangar = 23;
        const std::vector<int> vone = { 23 };
        auto seq_one_ref = coveo::enumerate_one_ref(hangar);
        detail::validate_sequence(seq_one_ref, vone, true);
    }

    // sequence bound by iterators
    {
        std::vector<int> vforseq = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_range = coveo::enumerable<int>::for_range(vforseq.begin(), vforseq.end());
        detail::validate_sequence(seq_range, vexpected, true);
        auto seq_crange = seq_range.as_const();
        detail::validate_sequence(seq_crange, vexpected, true);
    }
    {
        std::vector<int> vforseq = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_range = coveo::enumerate_range(vforseq.begin(), vforseq.end());
        detail::validate_sequence(seq_range, vexpected, true);
    }
    {
        const std::vector<int> vforseq = { 42, 23, 66 };
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_range = coveo::enumerable<const int>::for_range(vforseq.begin(), vforseq.end());
        detail::validate_sequence(seq_range, vexpected, true);
    }
    {
        const std::vector<int> vforseq = { 42, 23, 66 };
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_range = coveo::enumerate_range(vforseq.begin(), vforseq.end());
        detail::validate_sequence(seq_range, vexpected, true);
    }

    // sequence stored in container (externally)
    {
        std::vector<int> vcnt = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerable<int>(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
        auto seq_ccnt = seq_cnt.as_const();
        detail::validate_sequence(seq_ccnt, vexpected, true);
    }
    {
        std::vector<int> vcnt = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerable<int>::for_container(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
    }
    {
        std::vector<int> vcnt = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerate_container(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
    }
    {
        const std::vector<int> vcnt = { 42, 23, 66 };
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerable<const int>(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
    }
    {
        const std::vector<int> vcnt = { 42, 23, 66 };
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerable<const int>::for_container(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
    }
    {
        const std::vector<int> vcnt = { 42, 23, 66 };
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerate_container(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
    }

    // sequence stored in container (internally)
    {
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerable<int>(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
        auto seq_ccnt_mv = seq_cnt_mv.as_const();
        detail::validate_sequence(seq_ccnt_mv, vexpected, true);
    }
    {
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerable<int>::for_container(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
    }
    {
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerate_container(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
    }
    {
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerable<const int>(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
    }
    {
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerable<const int>::for_container(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
    }
    {
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerable<const int>::for_container(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
    }

    // sequence in array
    {
        int arr[] = { 42, 23, 66 };
        size_t arr_size = sizeof(arr) / sizeof(arr[0]);
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_arr = coveo::enumerable<int>::for_array(arr, arr_size);
        detail::validate_sequence(seq_arr, vexpected, true);
        auto seq_carr = seq_arr.as_const();
        detail::validate_sequence(seq_carr, vexpected, true);
    }
    {
        int arr[] = { 42, 23, 66 };
        size_t arr_size = sizeof(arr) / sizeof(arr[0]);
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_arr = coveo::enumerate_array(arr, arr_size);
        detail::validate_sequence(seq_arr, vexpected, true);
    }
    {
        const int arr[] = { 42, 23, 66 };
        size_t arr_size = sizeof(arr) / sizeof(arr[0]);
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_arr = coveo::enumerable<const int>::for_array(arr, arr_size);
        detail::validate_sequence(seq_arr, vexpected, true);
    }
    {
        const int arr[] = { 42, 23, 66 };
        size_t arr_size = sizeof(arr) / sizeof(arr[0]);
        const std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_arr = coveo::enumerate_array(arr, arr_size);
        detail::validate_sequence(seq_arr, vexpected, true);
    }

    // Objects that cannot be copied
    {
        detail::no_copy an_obj(42);
        bool avail = true;
        auto seq = coveo::enumerable<detail::no_copy>([&an_obj, avail](std::unique_ptr<detail::no_copy>&) mutable {
            detail::no_copy* pobj = nullptr;
            if (avail) {
                pobj = &an_obj;
                avail = false;
            }
            return pobj;
        });
        std::list<detail::no_copy> lexpected;
        lexpected.emplace_back(42);
        detail::validate_sequence(seq, lexpected, false);
        auto cseq = seq.as_const();
        detail::validate_sequence(cseq, lexpected, false);
    }
    {
        const detail::no_copy an_obj(42);
        bool avail = true;
        auto seq = coveo::enumerable<const detail::no_copy>([&an_obj, avail](std::unique_ptr<detail::no_copy>&) mutable {
            const detail::no_copy* pobj = nullptr;
            if (avail) {
                pobj = &an_obj;
                avail = false;
            }
            return pobj;
        });
        const std::list<detail::no_copy> lexpected = []() {
            std::list<detail::no_copy> l;
            l.emplace_back(42);
            return l;
        }();
        detail::validate_sequence(seq, lexpected, false);
    }

    // sequence with iterator returning non-reference
    {
        detail::noref_vector<int> vcnt = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerate_container(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
        auto seq_ccnt = seq_cnt.as_const();
        detail::validate_sequence(seq_ccnt, vexpected, true);
    }

    // non-const to const
    {
        auto validat = [](const coveo::enumerable<const int>& seq) {
            const std::vector<int> vexpected = { 42, 23, 66 };
            detail::validate_sequence(seq, vexpected, true);
        };
        std::vector<int> vforseq = { 42, 23, 66 };
        auto seq = coveo::enumerable<int>(vforseq);
        validat(seq);
        coveo::enumerable<const int> cseq(std::move(seq));
        validat(cseq);
        seq = coveo::enumerable<int>(vforseq);
        cseq = seq;
        validat(cseq);
        cseq = std::move(seq);
        validat(cseq);
    }
}

} // enumerable
} // coveo_tests
