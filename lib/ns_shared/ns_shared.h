/*  ns_shared.h — State shared between FreeRTOS tasks
 *
 *  Every access happens under the attached lock, so a task on the other
 *  core never sees a partly written value. Values are copied in and out;
 *  nothing hands out a reference to the guarded storage.
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_SHARED_H
#define NS_SHARED_H

struct NsLock {
    void (*acquire)(void *);
    void (*release)(void *);
    void *ctx;
};

class NsHold {
public:
    explicit NsHold(const NsLock &lock) : lock_(lock) {
        if (lock_.acquire) lock_.acquire(lock_.ctx);
    }
    ~NsHold() {
        if (lock_.release) lock_.release(lock_.ctx);
    }
    NsHold(const NsHold &) = delete;
    NsHold &operator=(const NsHold &) = delete;

private:
    const NsLock &lock_;
};

template <typename T>
class Guarded {
public:
    void attach(NsLock lock) { lock_ = lock; }

    T snapshot() const {
        NsHold hold(lock_);
        return value_;
    }

    void publish(const T &v) {
        NsHold hold(lock_);
        value_ = v;
    }

private:
    NsLock lock_{};
    T value_{};
};

/**
 * One request in flight from a client task to a server task, and its reply.
 * A new request is refused while one is pending or being served, but an
 * uncollected reply never blocks the next request.
 */
template <typename Req, typename Resp>
class Exchange {
public:
    void attach(NsLock lock) { lock_ = lock; }

    bool submit(const Req &r) {
        NsHold hold(lock_);
        if (state_ == PENDING || state_ == TAKEN)
            return false;
        req_   = r;
        state_ = PENDING;
        return true;
    }

    bool take(Req *out) {
        NsHold hold(lock_);
        if (state_ != PENDING)
            return false;
        *out   = req_;
        state_ = TAKEN;
        return true;
    }

    void reply(const Resp &r) {
        NsHold hold(lock_);
        if (state_ != TAKEN)
            return;
        resp_  = r;
        state_ = DONE;
    }

    bool collect(Resp *out) {
        NsHold hold(lock_);
        if (state_ != DONE)
            return false;
        *out   = resp_;
        state_ = IDLE;
        return true;
    }

private:
    enum State { IDLE, PENDING, TAKEN, DONE };
    NsLock lock_{};
    State  state_ = IDLE;
    Req    req_{};
    Resp   resp_{};
};

#endif
