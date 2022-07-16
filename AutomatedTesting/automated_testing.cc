#include "automated_testing.h"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>


// runs tests creating child processes
static void _run_Tests(const std::vector<std::function<int()>> &tests,
                         std::vector<pid_t> &child_procs)
{
    child_procs.reserve(child_procs.size() + tests.size());

    for (auto &&test : tests)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("do_Tests: fork");
            child_procs.push_back(pid);
        }
        else if (pid == 0)
        {
            exit(test());
        }
        else
        {
            child_procs.push_back(pid);
        }
    }
}

// waits for child process and gets the results
static void _get_Results(const std::vector<pid_t> &child_procs,
                           std::vector<TestResult> &results)
{
    results.reserve(results.size() + child_procs.size());

    for (auto &&child_proc : child_procs)
    {
        if (child_proc == -1)
        {
            results.push_back(TestResult::UNRESOLVED);
            continue;
        }

        siginfo_t siginfo {};
        int ret = waitid(P_PID, child_proc, &siginfo, WEXITED);

        if (ret == -1)
        {
            perror("waitid");
            results.push_back(TestResult::UNRESOLVED);
            continue;
        }

        if (siginfo.si_code == CLD_EXITED)
        {
            if (siginfo.si_status == 0)
                results.push_back(TestResult::SUCCESS);
            else
                results.push_back(TestResult::FAILURE);
        }
        else
        {
            results.push_back(TestResult::UNRESOLVED);
        }
    }
}

void do_Tests(const std::vector<std::function<int()>> &tests,
              std::vector<TestResult> &results)
{
    std::vector<pid_t> child_procs;
    _run_Tests(tests, child_procs);
    _get_Results(child_procs, results);
}
