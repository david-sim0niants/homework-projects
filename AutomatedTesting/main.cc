#include "automated_testing.h"
#include <cstddef>
#include <iostream>


int success_func()
{
    return 0;
}

int fail_func()
{
    return 1;
}

int crash_func()
{
    throw "CRASH";
}

int exit_func(int x)
{
    return x;
}

int cond_crash_func(bool crash)
{
    if (crash)
    {
        int *ptr = NULL;
        *ptr = 42;
    }

    return 0;
}


int main()
{
    std::vector<std::function<int ()>> tests = 
    {
        success_func, fail_func, crash_func,
        std::bind(exit_func, 0), std::bind(exit_func, -1),
        std::bind(cond_crash_func, true), std::bind(cond_crash_func, false)
    };

    std::vector<TestResult> results;
    do_Tests(tests, results);

    size_t test_i = 0;
    for (auto &&result : results)
    {
        switch (result)
        {
        case TestResult::SUCCESS:
            std::cerr << "Test " << test_i << ": Passed." << std::endl;
            break;
        case TestResult::FAILURE:
            std::cerr << "Test " << test_i << ": Failed." << std::endl;
            break;
        case TestResult::UNRESOLVED:
            std::cerr << "Test " << test_i
                      << ": Unexpected error. Test unresolved." << std::endl;
            break;
        default:
            std::cerr << "Really unknown error occurred." << std::endl;
            break;
        }
        ++test_i;
    }

    return 0;
}
