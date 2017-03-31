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
    no_copy(int i) : i_(i) { }
    no_copy(const no_copy&) = delete;
    no_copy& operator=(const no_copy&) = delete;
    bool operator==(const no_copy& obj) const {
        return i_ == obj.i_;
    }
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
    }

    // sequence defined by next delegate
    {
        std::vector<int> vi = { 42 };
        auto seq_i = coveo::enumerable<int>([](std::unique_ptr<int>& upi) -> const int* {
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
    }
    {
        std::vector<int> vone = { 42 };
        auto seq_one = coveo::enumerate_one(42);
        detail::validate_sequence(seq_one, vone, true);
    }
    
    // sequence of one element held by ref
    {
        const int hangar = 23;
        std::vector<int> vone = { 23 };
        auto seq_one_ref = coveo::enumerable<int>::for_one_ref(hangar);
        detail::validate_sequence(seq_one_ref, vone, true);
    }
    {
        const int hangar = 23;
        std::vector<int> vone = { 23 };
        auto seq_one_ref = coveo::enumerate_one_ref(hangar);
        detail::validate_sequence(seq_one_ref, vone, true);
    }

    // sequence bound by iterators
    {
        std::vector<int> vforseq = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_range = coveo::enumerable<int>::for_range(vforseq.cbegin(), vforseq.cend());
        detail::validate_sequence(seq_range, vexpected, true);
    }
    {
        std::vector<int> vforseq = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_range = coveo::enumerate_range(vforseq.cbegin(), vforseq.cend());
        detail::validate_sequence(seq_range, vexpected, true);
    }

    // sequence stored in container (externally)
    {
        std::vector<int> vcnt = { 42, 23, 66 };
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt = coveo::enumerable<int>(vcnt);
        detail::validate_sequence(seq_cnt, vexpected, true);
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

    // sequence stored in container (internally)
    {
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_cnt_mv = coveo::enumerable<int>(std::vector<int> { 42, 23, 66 });
        detail::validate_sequence(seq_cnt_mv, vexpected, true);
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

    // sequence in array
    {
        const int arr[] = { 42, 23, 66 };
        const size_t arr_size = sizeof(arr) / sizeof(arr[0]);
        std::vector<int> vexpected = { 42, 23, 66 };
        auto seq_arr = coveo::enumerable<int>::for_array(arr, arr_size);
        detail::validate_sequence(seq_arr, vexpected, true);
    }
    {
        const int arr[] = { 42, 23, 66 };
        const size_t arr_size = sizeof(arr) / sizeof(arr[0]);
        std::vector<int> vexpected = { 42, 23, 66 };
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
    }
}

} // enumerable
} // coveo_tests
