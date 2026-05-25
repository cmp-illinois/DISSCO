/*
LASS (additive sound synthesis library)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

/**
 * @file Semaphore.h
 * @brief Counting semaphore shim over std::mutex / std::condition_variable.
 *
 * C++11 has no std::counting_semaphore (that arrived in C++20), so this
 * supplies the POSIX sem_init / wait / post semantics LASS uses to throttle
 * the per-sound rendering workers in @ref Score, without pulling in
 * pthreads. Non-copyable to prevent the implicit move that would invalidate
 * the embedded mutex.
 */

#ifndef __SEMAPHORE_SHIM_H
#define __SEMAPHORE_SHIM_H

#include <condition_variable>
#include <mutex>

/**
 * @brief Minimal counting semaphore with POSIX-style wait/post.
 *
 * The internal count is incremented by post() and decremented by wait();
 * wait() blocks on the condition variable until the count is positive.
 * Used by @ref Score to cap the number of Sounds that are being rendered
 * concurrently.
 */
class Semaphore {
public:
    explicit Semaphore(unsigned int initial = 0) : count_(initial) {}

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    void post() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return count_ > 0; });
        --count_;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    unsigned int count_;
};

#endif // __SEMAPHORE_SHIM_H
