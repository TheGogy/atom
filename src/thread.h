#ifndef THREAD_H
#define THREAD_H

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "search.h"

namespace Atom {

constexpr size_t NB_THREADS_DEFAULT = 1;

class Thread {
public:
    Thread(size_t index) : idx(index), thread(&Thread::idle, this) {}

    virtual ~Thread();

    void search();
    void clear();
    void idle();

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
    using ThreadList = std::vector<std::unique_ptr<Thread>>;

    ThreadPool() {}                     // Constructor
    ~ThreadPool() { clearThreads(); }   // Destructor

    // ThreadPool cannot be copied
    ThreadPool(const ThreadPool &)            = delete;
    ThreadPool(ThreadPool &&)                 = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&)      = delete;

    void clearThreads();
    void setNbThreads(size_t nbThreads);
    Thread* firstThread() const { return threads.front().get(); }

    ThreadList::const_iterator cbegin() const noexcept { return threads.cbegin(); }
    ThreadList::const_iterator cend()   const noexcept { return threads.cend();   }
    ThreadList::iterator       begin()        noexcept { return threads.begin();  }
    ThreadList::iterator       end()          noexcept { return threads.end();    }
    ThreadList::size_type      size()   const noexcept { return threads.size();   }
    bool                       empty()  const noexcept { return threads.empty();  }

private:
    ThreadList threads;
};

} // namespace Atom

#endif // THREAD_H
