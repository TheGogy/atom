
#include "thread.h"
#include <memory>
#include <mutex>

namespace Atom {

void Thread::search() {
    worker->startSearch();
}


void Thread::clear() {
    worker->clear();
}


void Thread::waitForFinish() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]{ return !searching; });
}


void ThreadPool::clearThreads() {
    if (threads.size() == 0) return;

    for (std::unique_ptr<Thread>& thread: threads) {
        thread->clear();
        thread->waitForFinish();
    }

}

} // namespace Atom
