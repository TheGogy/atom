#ifndef THREAD_H
#define THREAD_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "search.h"

namespace Atom {

class Thread {
public:
    Thread(Search::SearchWorkerShared&);
    virtual ~Thread();

    void search();
    void clear();

    void waitForFinish();
    bool isSearching() { return searching; }

    size_t id() const { return idx; }

    std::unique_ptr<Search::SearchWorker> worker;

private:
    size_t idx;

    std::mutex              mutex;
    std::condition_variable cv;
    std::thread             thread;

    bool shouldExit;
    bool searching = true;
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
    Thread* firstThread() const { return threads.front().get(); }

  private:
    std::vector<std::unique_ptr<Thread>> threads;
};

} // namespace Atom

#endif // THREAD_H
