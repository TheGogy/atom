
#include "thread.h"
#include "search.h"
#include "types.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

namespace Atom {

Thread::~Thread() {
    shouldExit = true;
    search();
    thread.join();
}

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


void ThreadPool::startSearch() {
    for (std::unique_ptr<Thread>& thread : threads) {
        if (thread != threads.front()) thread->search();
    }
}


void ThreadPool::waitForFinish() {
    for (std::unique_ptr<Thread>& thread : threads) {
        if (thread != threads.front()) thread->waitForFinish();
    }
}


void ThreadPool::clearThreads() {
    if (threads.size() == 0) return;

    for (std::unique_ptr<Thread>& thread: threads) {
        thread->clear();
        thread->waitForFinish();
    }
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

    for (size_t i = 0; i < nbThreads; ++i) {
        threads.emplace_back(std::make_unique<Thread>(i));
    }
}


Thread* ThreadPool::bestThread() const {

    Thread* bestThread = firstThread();

    for (const std::unique_ptr<Thread>& newThread : threads) {
        const Value maxScore = bestThread->worker->getRootMove(0).score;
        const Depth maxDepth = bestThread->worker->getRootMove(0).selDepth;

        const Value newScore = newThread->worker->getRootMove(0).score;
        const Depth newDepth = newThread->worker->getRootMove(0).selDepth;

        // If thread has an equal depth and greater score (or possible mate) it is better.
        if ((newScore > maxScore)
        &&  (newDepth == maxDepth || newScore > VALUE_MATE_IN_MAX_PLY)) {
            bestThread = newThread.get();
        }

        // If thread has a greater depth + score (without replacing a closer mate) it is better.
        if ((newDepth > maxDepth)
        && (newScore > maxScore || maxScore < VALUE_MATE_IN_MAX_PLY)) {
            bestThread = newThread.get();
        }

    }

    return bestThread;
}


uint64_t ThreadPool::totalNodesSearched() const {
    uint64_t sum = 0;
    for (const std::unique_ptr<Thread>& thread : threads) {
        sum += thread->worker->getNodes();
    }
    return sum;
}


uint64_t ThreadPool::totalTbHits() const {
    uint64_t sum = 0;
    for (const std::unique_ptr<Thread>& thread : threads) {
        sum += thread->worker->getTbHits();
    }
    return sum;
}

} // namespace Atom
