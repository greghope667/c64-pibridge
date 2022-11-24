#pragma once

#include "pico/util/queue.h"

/* Simple C++ wrapper around provided thread-safe queue
 * Very thin, mostly for type-safety */

// front back empty size push emplace pop swap

template <typename T>
struct queue {
    using handle_t = queue_t;
    handle_t* handle;

    void init(int sz) {
        queue_init(handle, sizeof(T), sz);
    }

    void free() {
        queue_free(handle);
    }

    [[nodiscard]] int size() const {
        return queue_get_level(handle);
    }

    // Non-blocking
    [[nodiscard]] bool empty() const {
        return queue_is_empty(handle);
    }

    // Non-blocking
    [[nodiscard]] bool full() const {
        return queue_is_full(handle);
    }

    void push(const T& t) {
        queue_add_blocking(handle, &t);
    }

    void pop(T& t) {
        queue_remove_blocking(handle, &t);
    }

    // Non-blocking
    bool try_push(const T& t) {
        return queue_try_add(handle, &t);
    }

    // Non-blocking
    bool try_pop(T& t) {
        return queue_try_remove(handle, &t);
    }
};
