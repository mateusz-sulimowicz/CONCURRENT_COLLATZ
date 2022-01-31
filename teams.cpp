#include <utility>
#include <deque>
#include <future>
#include <thread>
#include <semaphore>
#include "teams.hpp"
#include "contest.hpp"

namespace {
    uint64_t calcCollatzX(InfInt n, const std::shared_ptr<SharedResults> &shared) {
        uint64_t count = 0;
        assert(n > 0);

        while (n != 1) {
            uint64_t r;
            if (shared->get(n, r)) {
                shared->put(n, count + r);
                return count + r;
            }

            ++count;
            if (n % 2 == 1) {
                n *= 3;
                n += 1;
            } else {
                n /= 2;
            }
        }

        shared->put(n, count);
        return count;
    }
}


ContestResult TeamNewThreads::runContestImpl(ContestInput const &contestInput) {
    ContestResult results;
    results.resize(contestInput.size());

    std::mutex m{};
    std::counting_semaphore sem{this->getSize()};
    std::vector<std::thread> workers;
    workers.resize(this->getSize());

    std::queue<int> unoccupied{};
    for (int i = 0; i < this->getSize(); ++i) {
        unoccupied.push(i);
    }

    for (int i = 0; i < contestInput.size(); ++i) {
        sem.acquire();
        std::lock_guard lock{m};
        int id = unoccupied.front();
        unoccupied.pop();

        InfInt input = contestInput[i];
        std::thread t = this->createThread(
                [shared = this->getSharedResults(), input, i, &results, &sem, &m, &unoccupied, id]() {
                    results[i] = shared
                                 ? calcCollatzX(input, shared)
                                 : calcCollatz(input);
                    std::lock_guard lock{m};
                    unoccupied.push(id);
                    sem.release();
                });
        t.detach();
        workers.push_back(std::move(t));
    }

    for (size_t j = 0; j < this->getSize(); ++j) {
        sem.acquire();
    }
    return results;
}

ContestResult TeamConstThreads::runContestImpl(ContestInput const &contestInput) {
    ContestResult results;
    results.resize(contestInput.size());
    std::vector<std::thread> workers{};
    size_t workload = contestInput.size() / this->getSize() + 1;

    for (size_t i = 0; i < this->getSize(); ++i) {
        std::thread t = this->createThread(
                [shared = this->getSharedResults(), &results, contestInput, workload, beg = workload * i] {
                    for (size_t j = beg; j < contestInput.size() && j < beg + workload; ++j) {
                        results[j] = shared
                                     ? calcCollatzX(contestInput[j], shared)
                                     : calcCollatz(contestInput[j]);
                    }
                });
        workers.push_back(std::move(t));
    }

    for (std::thread &t: workers) {
        t.join();
    }
    return results;
}

ContestResult TeamPool::runContest(ContestInput const &contestInput) {
    std::vector<std::future<uint64_t>> res{};
    for (size_t i = 0; i < contestInput.size(); ++i) {
        res.push_back(this->pool.push([shared = this->getSharedResults(), n = contestInput[i]] {
            return shared
                   ? calcCollatzX(n, shared)
                   : calcCollatz(n);
        }));
    }
    return cxxpool::get(res.begin(), res.end());
}
/*

ContestResult TeamNewProcesses::runContest(ContestInput const &contestInput) {
    ContestResult r;
    //TODO
    return r;
}

ContestResult TeamConstProcesses::runContest(ContestInput const &contestInput) {
    ContestResult r;
    //TODO
    return r;
}

ContestResult TeamAsync::runContest(ContestInput const &contestInput) {
    ContestResult r;
    //TODO
    return r;
}
*/
