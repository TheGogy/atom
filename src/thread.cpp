#include <cstdint>
#include <memory>
#include <mutex>

#include "thread.h"
#include "movegen.h"
#include "search.h"
#include "types.h"

namespace Atom {

Thread::~Thread() {
    shouldExit = true;
    search();
    thread.join();
}

void Thread::search() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return !searching; });
    jobFunction = std::move([this]() { worker->startSearch(); });
    searching = true;

    cv.notify_one();
}


void Thread::clear() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return !searching; });
    jobFunction = std::move([this]() { worker->clear(); });
    searching = true;

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

        std::function<void()> job = std::move(jobFunction);
        jobFunction = nullptr;

        lock.unlock();

        if (job) {
            job();
        }
    }
}


void Thread::waitForFinish() {
    std::unique_lock<std::mutex> lock(mutex);
    // Wait until other threads are no longer searching
    cv.wait(lock, [&]{ return !searching; });
}


// Main go command. This will be run whenever the engine needs to start thinking
void ThreadPool::go(
    Position& pos,
    Search::SearchLimits limits
) {

    firstThread()->waitForFinish();

    shouldStop = abortSearch = false;

    Search::RootMoveList rootMoves;

    Movegen::enumerateLegalMoves(pos, [&](Move m) {
        rootMoves.push_back(Search::RootMove(m));
        return true;
    });

    for (std::unique_ptr<Thread>& thread : threads) {
        thread->waitForFinish();
        thread->setupWorker(pos, rootMoves, limits);
    }

    // Start first thread searching, this will notify the others.
    firstThread()->search();
}


// Start all threads searching.
// This should only be invoked once, by the main thread.
void ThreadPool::startSearching() {

    for (std::unique_ptr<Thread>& thread : threads) {
        // First thread has called all the other threads: it is
        // already searching. Call all other threads.
        if (!(thread->id() == 0)) {
            thread->search();
        }
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


void ThreadPool::setNbThreads(size_t nbThreads, Search::SearchWorkerShared sharedState) {
    // Wait for existing threads to finish
    if (!threads.empty()) {
        firstThread()->waitForFinish();
        threads.clear();
    }

    if (nbThreads <= 0) {
        return;
    }

    for (size_t i = 0; i < nbThreads; ++i) {
        threads.emplace_back(std::make_unique<Thread>(i, sharedState));
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
