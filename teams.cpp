#include <utility>
#include <deque>
#include <future>
#include <thread>
#include <iostream>
#include <semaphore>
#include "teams.hpp"
#include "contest.hpp"

namespace {
    uint64_t calcCollatzX(InfInt n, const std::shared_ptr<SharedResults> &shared) {
        assert(n > 0);
        uint64_t count = 0;
        uint64_t result = 0;
        InfInt n0 = n;

        while (n != 1) {
            uint64_t r;
            if (shared->get(n, r)) {
                result = count + r;
                break;
            }

            ++result;
            ++count;
            if (n % 2 == 1) {
                n *= 3;
                n += 1;
            } else {
                n /= 2;
            }
        }

        shared->put(n0, result);
        return result;
    }

    uint64_t calcCollatz(InfInt n, const std::shared_ptr<SharedResults> &shared) {
        return shared ? calcCollatzX(n, shared) : calcCollatz(n);
    }
}

ContestResult TeamNewThreads::runContestImpl(ContestInput const &contestInput) {
    ContestResult results(contestInput.size());

    std::mutex m{};
    std::counting_semaphore sem{this->getSize()};
    std::vector<std::thread> workers(this->getSize());

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
                    results[i] = calcCollatz(input, shared);
                    std::lock_guard lock{m};
                    unoccupied.push(id);
                    sem.release();
                });
        t.detach();
        workers[id] = std::move(t);
    }

    for (size_t j = 0; j < workers.size(); ++j) {
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
                        results[j] = calcCollatz(contestInput[j], shared);
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
            return calcCollatz(n, shared);
        }));
    }
    return cxxpool::get(res.begin(), res.end());
}

ContestResult TeamAsync::runContest(ContestInput const &contestInput) {
    ContestResult r;
    r.resize(contestInput.size());
    std::vector<std::future<void>> futures(contestInput.size());
    for (int i = 0; i < contestInput.size(); ++i) {
        futures[i] = std::async([&r, i, shared = this->getSharedResults(), &contestInput]() {
            r[i] = calcCollatz(contestInput[i], shared);
        });
    }

    for (int i = 0; i < contestInput.size(); ++i) {
        futures[i].get();
    }
    return r;
}