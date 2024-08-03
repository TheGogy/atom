#ifndef THREAD_H
#define THREAD_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "search.h"

namespace Atom {

constexpr size_t NB_THREADS_DEFAULT = 1;

class Thread {
public:
    Thread(
        size_t index,
        Search::SearchWorkerShared& sharedState
    ) :
        idx(index),
        thread(&Thread::idle, this)
    {
        worker = std::make_unique<Search::SearchWorker>(sharedState, index);
    }

    virtual ~Thread();

    void search();
    void clear();
    void idle();

    void waitForFinish();
    bool isSearching() { return searching; }

    size_t id() const { return idx; }

    std::unique_ptr<Search::SearchWorker> worker;

    void setupWorker(
        const Position& rootPosition,
        Search::RootMoveList rootMoves,
        const Search::SearchLimits limits
    ) {
        worker->rootPosition = rootPosition;
        worker->rootMoves    = rootMoves;
        worker->limits       = limits;
    }

private:
    size_t idx;

    std::mutex              mutex;
    std::condition_variable cv;
    std::thread             thread;

    bool shouldExit = false;
    bool searching  = true;
};


class ThreadPool {
public:
    using ThreadList = std::vector<std::unique_ptr<Thread>>;

    // Constructor / destructor
    ThreadPool() {}
    ~ThreadPool() { clearThreads(); }

    // ThreadPool cannot be copied
    ThreadPool(const ThreadPool &)            = delete;
    ThreadPool(ThreadPool &&)                 = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&)      = delete;

    // UCI commands
    void clearThreads();
    void setNbThreads(size_t nbThreads, Search::SearchWorkerShared sharedState);

    // Start / stop searching
    void go(
        Position& pos,
        Search::SearchLimits limits
    );
    void startSearching();
    void waitForFinish();

    // Find specific threads / workers
    Thread* bestThread() const;
    Thread* firstThread() const { return threads.front().get(); }
    Search::SearchWorker* firstWorker() const { return threads.front()->worker.get(); }

    // Vector operations
    ThreadList::const_iterator cbegin() const noexcept { return threads.cbegin(); }
    ThreadList::const_iterator cend()   const noexcept { return threads.cend();   }
    ThreadList::iterator       begin()        noexcept { return threads.begin();  }
    ThreadList::iterator       end()          noexcept { return threads.end();    }
    ThreadList::size_type      size()   const noexcept { return threads.size();   }
    bool                       empty()  const noexcept { return threads.empty();  }

    // Get info from threads
    uint64_t totalNodesSearched() const;
    uint64_t totalTbHits() const;

    // Stop variable
    std::atomic_bool shouldStop;
    std::atomic_bool abortSearch;

private:
    ThreadList threads;
};

} // namespace Atom

#endif // THREAD_H
