// Copyright (c) 2015-2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// A relatively barebones test framework.

#ifndef COVEO_TEST_FRAMEWORK_H
#define COVEO_TEST_FRAMEWORK_H

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace coveo_tests {

// Exception thrown when an assertion fails.
class assert_exception : public std::logic_error
{
public:
    using std::logic_error::logic_error;
};

// Function called when an assertion fails.
template<typename = void>
void assertion_failed(const char* condition, const char* source_file, int line) {
    std::ostringstream oss;
    oss << "Assertion failed: " << condition << " (" << source_file << ", line " << line << ")";
    throw assert_exception(oss.str());
}

// Macros to validate test conditions.
#define COVEO_ASSERT(COND) \
    { \
        if (!(COND)) { \
            coveo_tests::assertion_failed(#COND, __FILE__, __LINE__); \
        } \
    }
#define COVEO_ASSERT_FALSE() \
    { \
        coveo_tests::assertion_failed("Default assertion failed", __FILE__, __LINE__); \
    }
#define COVEO_ASSERT_THROW(CODE) \
    { \
        bool _a_caught = false; \
        try { \
            CODE; \
        } catch (...) { \
            _a_caught = true; \
        } \
        if (!_a_caught) { \
            coveo_tests::assertion_failed(#CODE, __FILE__, __LINE__); \
        } \
    }

// Function that runs a series of tests and reports failed assertions.
// Returns a value that can be returned from main() to report errors.
template<typename F, typename... Args>
int run_tests(F&& test_run, Args&&... args)
{
    int ret = 0;

    try {
        test_run(std::forward<Args>(args)...);
    } catch (const assert_exception& ae) {
        std::cerr << std::endl << ae.what() << std::endl;
        ret = 1;
    } catch (const std::exception& e) {
        std::cerr << std::endl
                  << "Unexpected exception caught: " << e.what() << std::endl;
        ret = 2;
    } catch (...) {
        std::cerr << std::endl
                  << "Unexpected unknown exception caught" << std::endl;
        ret = 3;
    }

    return ret;
}

} // namespace coveo_tests

#endif // COVEO_TEST_FRAMEWORK_H
