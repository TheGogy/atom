
#include "thread.h"
#include "search.h"
#include <memory>
#include <mutex>

namespace Atom {

void Thread::search() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return !searching; });
    searching = true;

    worker->startSearch();

    cv.notify_one();
}


void Thread::clear() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return !searching; });
    searching = true;

    worker->clear();

    cv.notify_one();
}


void Thread::idle() {
    while (true) {
        searching = false;
        std::unique_lock<std::mutex> lock(mutex);
        cv.notify_one();
        // Wait until some other thread starts searching
        cv.wait(lock, [&] { return searching; });

        if (shouldExit) return;

        lock.unlock();
    }
}


void Thread::waitForFinish() {
    std::unique_lock<std::mutex> lock(mutex);
    // Wait until other threads are no longer searching
    cv.wait(lock, [&]{ return !searching; });
}


void ThreadPool::setNbThreads(size_t nbThreads) {
    // Wait for existing threads to finish
    if (!threads.empty()) {
        firstThread()->waitForFinish();
        threads.clear();
    }

    if (nbThreads <= 0) {
        return;
    }

    // First thread is the main thread
    threads.emplace_back(std::make_unique<Thread>(0));
}

void ThreadPool::clearThreads() {
    if (threads.size() == 0) return;

    for (std::unique_ptr<Thread>& thread: threads) {
        thread->clear();
        thread->waitForFinish();
    }
}

} // namespace Atom
