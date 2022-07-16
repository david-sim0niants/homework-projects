#ifndef __AUTOMATED_TESTING_H__
#define __AUTOMATED_TESTING_H__

#include <functional>
#include <vector>

enum class TestResult
{
    SUCCESS = 0, FAILURE, UNRESOLVED
};

void do_Tests(const std::vector<std::function<int()>> &tests,
              std::vector<TestResult> &results);

#endif
