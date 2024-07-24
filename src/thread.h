#ifndef THREAD_H
#define THREAD_H

#include "search.h"
#include <memory>
#include <mutex>
#include <thread>
#include <vector>


namespace Atom {

class Thread {
public:
    Thread(Search::SearchWorkerShared&);
    virtual ~Thread();

    void search();
    void clear();
    void lock();

    size_t id() const { return idx; }

    std::unique_ptr<Search::SearchWorker> worker;

private:
    size_t idx;

    std::mutex mutex;
    bool shouldExit;
    std::thread thread;
};


class ThreadPool {
public:
    ThreadPool() {}                     // Constructor
    ~ThreadPool() { clearThreads(); }   // Destructor

    // ThreadPool cannot be copied
    ThreadPool(const ThreadPool &)            = delete;
    ThreadPool(ThreadPool &&)                 = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&)      = delete;

    void clearThreads();

  private:
    std::vector<std::unique_ptr<Thread>> threads;
};

} // namespace Atom

#endif // THREAD_H
