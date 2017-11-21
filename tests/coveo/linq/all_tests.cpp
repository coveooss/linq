// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

#include "coveo/linq/all_tests.h"

#include <coveo/enumerable/enumerable_tests.h>
#include <coveo/linq/linq_tests.h>

namespace coveo_tests {
namespace linq {

// Runs all tests for coveo::enumerable and coveo::linq
void all_tests()
{
    // enumerable
    using namespace coveo_tests::enumerable;
    enumerable_tests();

    // linq
    linq_tests();
    chaining_tests();
    dangling_ref_tests();
}

// Runs all benchmarks for coveo::enumerable and coveo::linq
void all_benchmarks()
{
    // linq
    linq_benchmarks();
}

} // linq
} // coveo_tests
