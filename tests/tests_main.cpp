// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

#include <coveo/linq/all_tests.h>
#include <coveo/test_framework.h>

#include <iostream>
#include <string>

// Test program entry point.
int main()
{
    int ret = 0;

#ifndef COVEO_LINQ_BENCHMARKS
    std::cout << "Running tests..." << std::endl;
    ret = coveo_tests::run_tests(&coveo_tests::linq::all_tests);
#else
    std::cout << "Running benchmarks..." << std::endl;
    ret = coveo_tests::run_tests(&coveo_tests::linq::all_benchmarks);
#endif
    std::cout << "Done." << std::endl;

    if (ret != 0) {
        std::cout << std::endl << "Press enter to continue ";
        std::string unused;
        std::getline(std::cin, unused);
    }
    
    return ret;
}
