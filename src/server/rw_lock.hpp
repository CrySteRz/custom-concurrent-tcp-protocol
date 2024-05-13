#pragma once

#include <mutex>
#include <condition_variable>

class ReadWriteLock
{
private:
    std::mutex              mtx;
    std::condition_variable cv;
    bool                    is_locked        = false;
    bool                    is_writer_active = false;

public:
    void read_lock()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]()
        {
            return !is_locked && !is_writer_active;
        });
        is_locked = true;
    }

    void read_unlock()
    {
        std::lock_guard<std::mutex> lock(mtx);
        is_locked = false;
        cv.notify_one();
    }

    void write_lock()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]()
        {
            return !is_locked && !is_writer_active;
        });
        is_writer_active = true;
        is_locked        = true;
    }

    void write_unlock()
    {
        std::lock_guard<std::mutex> lock(mtx);
        is_writer_active = false;
        is_locked        = false;
        cv.notify_all();
    }

    class ReadLockGuard
    {
    private:
        ReadWriteLock& rw_lock;
    public:
        ReadLockGuard(ReadWriteLock& lock)
            : rw_lock(lock)
        {
            rw_lock.read_lock();
        }
        ~ReadLockGuard()
        {
            rw_lock.read_unlock();
        }
    };

    class WriteLockGuard
    {
    private:
        ReadWriteLock& rw_lock;
    public:
        WriteLockGuard(ReadWriteLock& lock)
            : rw_lock(lock)
        {
            rw_lock.write_lock();
        }
        ~WriteLockGuard()
        {
            rw_lock.write_unlock();
        }
    };
};
