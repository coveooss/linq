// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// Definition of exceptions used by LINQ operators.

#ifndef COVEO_LINQ_EXCEPTION_H
#define COVEO_LINQ_EXCEPTION_H

#include <stdexcept>

namespace coveo {
namespace linq {

// Exception thrown by LINQ operators that cannot be applied to an empty sequence.
class empty_sequence : public std::logic_error
{
public:
    empty_sequence() = delete;
    using std::logic_error::logic_error;
};

// Exception thrown by LINQ operators when going out of a sequence's range.
class out_of_range : public std::out_of_range
{
public:
    out_of_range() = delete;
    using std::out_of_range::out_of_range;
};

} // linq
} // coveo

#endif // COVEO_LINQ_EXCEPTION_H
