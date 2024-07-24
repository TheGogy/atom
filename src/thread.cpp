
#include "thread.h"
#include <memory>

namespace Atom {

void Thread::search() {
    worker->startSearch();
}

void Thread::clear() {
    worker->clear();
}

void ThreadPool::clearThreads() {
    if (threads.size() == 0) return;

    for (std::unique_ptr<Thread>& thread: threads) {
        thread->clear();
        thread->lock();
    }

}

} // namespace Atom
