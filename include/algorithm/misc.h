#ifndef _HYPERBIRDER_COMMON_MISC_INCLUDE_H_
#define _HYPERBIRDER_COMMON_MISC_INCLUDE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
//#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <new>

#include "comm_def.h"
#include "atomic.h"

DECLARE_HB_NAMESPACE(common)

#ifndef HBC_NOT_USE_MUTEX_TIMEDLOCK
#define HBC_NOT_USE_MUTEX_TIMEDLOCK
#endif

#define len_of_ary(ary) (sizeof(ary)/sizeof((ary)[0]))

inline timeval microseconds_to_tv(const int64_t microseconds)
{
    struct timeval tp;

    tp.tv_sec = microseconds / 1000000;
    tp.tv_usec = microseconds % 1000000;

    return tp;
}

inline timespec microseconds_to_ts(const int64_t microseconds)
{
    struct timespec ts;

    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;

    return ts;
}

inline int64_t tv_to_microseconds(const timeval & tp)
{
    return (((int64_t) tp.tv_sec) * 1000000 + (int64_t) tp.tv_usec);
}

inline int64_t ts_to_microseconds(const timespec & ts)
{
    return (((int64_t) ts.tv_sec) * 1000000 + (int64_t) ((ts.tv_nsec + 500) / 1000));
}

inline int64_t get_cur_microseconds_time(void)
{
    struct timeval tp;

    gettimeofday(&tp, NULL);

    return tv_to_microseconds(tp);
}

inline void microseconds_sleep(const int64_t microseconds)
{
    struct timespec ts = microseconds_to_ts(microseconds);

    nanosleep(&ts, NULL);
}

inline void make_timespec_with_interval(struct timespec& tsp, int64_t useconds)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    useconds += now.tv_usec;
    tsp.tv_sec = now.tv_sec;
    while (useconds >= 1000000)
    {
        tsp.tv_sec++;
        useconds -= 1000000;
    }
    tsp.tv_nsec = useconds * 1000;
}

# if defined __x86_64__

class initial
{
protected:
    enum cconst_private
    {
        STATE_INIT_SUCCEED      = 0     ,
        STATE_NOT_INIT_YET              ,
        STATE_INIT_FAIL                 ,
        STATE_DESTRUCTED                ,
    };
public:
    initial()
    {
        _err = 0;
        set_not_init_yet();
    };
    ~initial()
    {
        set_destructed();
    };
private:
    int _state;
    int _err;
public:
    bool is_not_init_yet(void) const
    {
        return (STATE_NOT_INIT_YET == _state);
    };
    bool is_init_fail(void) const
    {
        return (STATE_INIT_FAIL == _state);
    };
    bool is_init_succeed(void) const
    {
        return (STATE_INIT_SUCCEED == _state);
    };
    void set_not_init_yet(void) const
    {
        *((int *)&_state) = STATE_NOT_INIT_YET;
    };
    void set_init_result(const int err) const
    {
        if (is_not_init_yet())
        {
            *((int *)&_err) = err;
            *((int *)&_state) = (0 == err) ? STATE_INIT_SUCCEED : STATE_INIT_FAIL;
        }
    };
    int get_init_result(void) const
    {
        return _err;
    };
    void set_destructed(void) const
    {
        *((int *)&_state) = STATE_DESTRUCTED;
    };
};

// a lightweight lock
class spinlock
{
public:
    enum cconst_private
    {
        MICROSECONDS_PER_SLEEP  = 20    ,
    };
private:
    initial _init_state;
    pthread_spinlock_t _spinlock;
public:
    inline spinlock()
    {
        init();
    }
    inline ~ spinlock()
    {
        if (_init_state.is_init_succeed())
        {
            pthread_spin_destroy(&_spinlock);
            _init_state.set_destructed();
        }
    }
public:
    int init(void) const
    {
        int err = 0;

        if (_init_state.is_not_init_yet())
        {
            err = pthread_spin_init((pthread_spinlock_t *)&_spinlock, PTHREAD_PROCESS_PRIVATE);
            _init_state.set_init_result(err);
        }
        else
        {
            err = _init_state.get_init_result();
        }

        return err;
    };
protected:
    inline int _lock(const int64_t wait_microsecond = 0) const
    {
        int64_t wait_us = (wait_microsecond <= 0) ? (MAX_INT64/2) : wait_microsecond;
        int64_t time_us = 0;
        int err = 0;

        while (wait_us > 0)
        {
            time_us = get_cur_microseconds_time();
            if ((err = pthread_spin_trylock((pthread_spinlock_t *)&_spinlock)) == 0)
            {
                break;
            }
            else if (EBUSY == err)
            {
                microseconds_sleep(MICROSECONDS_PER_SLEEP);
                wait_us -= get_cur_microseconds_time() - time_us;
            }
            else
            {
                break;
            }
        }

        return err;
    }
    inline int _unlock(void) const
    {
        int err = 0;

        err = pthread_spin_unlock((pthread_spinlock_t *) &_spinlock);

        return err;
    }
public:
    int lock(const int64_t max_try_time) const
    {
        return _lock(max_try_time);
    };
    int unlock(void) const
    {
        return _unlock();
    };
};

class rwlocker
{
private:
    enum cconst_private
    {
        MICROSECONDS_PER_R_SLEEP  = 20    ,
        MICROSECONDS_PER_W_SLEEP  = 20    ,
    };
    static const uint64_t W_FLAG = ((uint64_t)0x1)<<62;
    static const uint64_t R_MASK = W_FLAG-1;
private:
    initial _init_state;
    pthread_rwlock_t _rwlock;
public:
    inline rwlocker()
    {
        init();
    }
    inline ~ rwlocker()
    {
        if (_init_state.is_init_succeed())
        {
            pthread_rwlock_destroy(&_rwlock);
            _init_state.set_destructed();
        }
    }
public:
    int init(void) const
    {
        int err = 0;

        if (_init_state.is_not_init_yet())
        {
            err = pthread_rwlock_init((pthread_rwlock_t *)&_rwlock, NULL);
            _init_state.set_init_result(err);
        }
        else
        {
            err = _init_state.get_init_result();
        }

        return err;
    };
protected:
    inline int _r_lock(const int64_t wait_microsecond = 0) const
    {
        int64_t wait_us = (wait_microsecond <= 0) ? (MAX_INT64/2) : wait_microsecond;
        struct timespec ts = microseconds_to_ts(wait_us);
        int err = 0;

        err = pthread_rwlock_timedrdlock((pthread_rwlock_t *)&_rwlock, &ts);

        return err;
    };
    inline int _w_lock(const int64_t wait_microsecond = 0) const
    {
        int64_t wait_us = (wait_microsecond <= 0) ? (MAX_INT64/2) : wait_microsecond;
        struct timespec ts = microseconds_to_ts(wait_us);
        int err = 0;

        err = pthread_rwlock_timedwrlock((pthread_rwlock_t *)&_rwlock, &ts);

        return err;
    };
    inline int _unlock(void) const
    {
        int err = 0;

        err = pthread_rwlock_unlock((pthread_rwlock_t *)&_rwlock);

        return err;
    };
public:
    int r_lock(const int64_t max_try_time) const
    {
        return _r_lock(max_try_time);
    };
   int r_unlock(void) const
    {
        return _unlock();
    };
    int w_lock(const int64_t max_try_time) const
    {
        return _w_lock(max_try_time);
    };
    int w_unlock(void) const
    {
        return _unlock();
    };
};

class mutex_locker;
class mutex_holder
{
    friend class mutex_locker;
public:
    mutex_holder()
    {
        _init(NULL);
    };
    ~mutex_holder();
private:
    uint64_t _lock_uniq_cnt;
    const mutex_locker * _plock;
private:
    void _init(mutex_locker * plock)
    {
        _set_lock_uniq_cnt(0);
        _set_lock_ptr(plock);
    };
    void _set_lock_uniq_cnt(const uint64_t lock_uniq_cnt)
    {
        _lock_uniq_cnt = lock_uniq_cnt;
        return;
    };
    uint64_t _get_lock_uniq_cnt(void) const
    {
        return _lock_uniq_cnt;
    };
    void _set_lock_ptr(const mutex_locker * plock)
    {
        _plock = plock;
    };
    const mutex_locker * _get_lock_ptr(void) const
    {
        return _plock;
    };
};

class mutex_locker
{
    friend class mutex_holder;
private:
protected:
    enum cconst_private
    {
        LOCK_UNIQ_CNT_ADD   = 2 ,
        ON_MUTATE_MARK      = 1 ,
    };
private:
    initial _init_state;
    uint64_t _lock_uniq_cnt;
	#ifdef HB_NOT_USE_MUTEX_TIMEDLOCK
	pthread_rwlock_t _rwlock;
	#else
    pthread_mutex_t _lock;
	#endif
public:
    inline mutex_locker()
    {
        int err = 0;

		#ifdef HB_NOT_USE_MUTEX_TIMEDLOCK
		if ((err = pthread_rwlock_init((pthread_rwlock_t *)&_rwlock, NULL)) != 0)
		#else
        if ((err = pthread_mutex_init(&_lock, NULL)) != 0)
		#endif
        {
        }

        _lock_uniq_cnt = LOCK_UNIQ_CNT_ADD;

        _init_state.set_init_result(err);
    }
    inline ~ mutex_locker()
    {
		#ifdef HB_NOT_USE_MUTEX_TIMEDLOCK
		pthread_rwlock_destroy(&_rwlock);
		#else
        pthread_mutex_destroy(&_lock);
        #endif
		_init_state.set_destructed();
    }
public:
    int init(void) const
    {
        return 0;
    };
protected:
    inline int _acquire_lock(
        mutex_holder & lock_hold,
        const int64_t wait_microsecond = 0) const;
    inline int _release_lock(mutex_holder & lock_hold) const;
    inline bool _verify_hold(const mutex_holder & lock_hold) const;
    inline int _acquire_hold(const mutex_holder & lock_hold) const;
    inline int _release_hold(const mutex_holder & lock_hold) const;
};

inline mutex_holder::~mutex_holder()
{
    if (NULL != _plock)
    {
        _plock->_release_lock(*this);
        _plock = NULL;
    }
};

inline int mutex_locker::_acquire_lock(
        mutex_holder & lock_hold,
        const int64_t wait_microsecond) const
{
    int err = 0;

    if (wait_microsecond <= 0 || wait_microsecond >=  MAX_INT64/4)
    {
		#ifdef HB_NOT_USE_MUTEX_TIMEDLOCK
		if ((err = pthread_rwlock_wrlock((pthread_rwlock_t *)&_rwlock)) != 0)
		#else
        if ((err = pthread_mutex_lock((pthread_mutex_t *)&_lock)) != 0)
		#endif
        {
        }
        else if ((_lock_uniq_cnt & ON_MUTATE_MARK) != 0)
        {
            err = EINTR;
        }
        else
        {
            lock_hold._set_lock_uniq_cnt(_lock_uniq_cnt);
            lock_hold._set_lock_ptr(this);
        }
    }
    else
    {
        const int64_t wait_us = wait_microsecond+get_cur_microseconds_time();
        struct timespec ts = microseconds_to_ts(wait_us);

		#ifdef HB_NOT_USE_MUTEX_TIMEDLOCK
		if ((err = pthread_rwlock_timedwrlock((pthread_rwlock_t *)&_rwlock, &ts)) != 0)
		#else
        if ((err = pthread_mutex_timedlock((pthread_mutex_t *)&_lock, &ts)) != 0)
		#endif
        {
        }
        else if ((_lock_uniq_cnt & ON_MUTATE_MARK) != 0)
        {
            err = EINTR;
        }
        else
        {
            lock_hold._set_lock_uniq_cnt(_lock_uniq_cnt);
            lock_hold._set_lock_ptr(this);
        }
    }

    return err;
};

inline int mutex_locker::_release_lock(mutex_holder & lock_hold) const
{
    int err = 0;
    if (atomic_compare_exchange(
                            (volatile uint64_t *)&_lock_uniq_cnt, 
                            lock_hold._get_lock_uniq_cnt()+LOCK_UNIQ_CNT_ADD, 
                            lock_hold._get_lock_uniq_cnt()) == lock_hold._get_lock_uniq_cnt())
    {
		#ifdef HB_NOT_USE_MUTEX_TIMEDLOCK
		if ((err = pthread_rwlock_unlock((pthread_rwlock_t *)&_rwlock)) != 0)
		#else
        if ((err = pthread_mutex_unlock((pthread_mutex_t *)&_lock)) != 0)
		#endif
        {
        }
        else
        {
            lock_hold._set_lock_uniq_cnt(0);
            lock_hold._set_lock_ptr(NULL);
        }
    }
    else
    {
        err = EPERM;
    }

    return err;
};

inline bool mutex_locker::_verify_hold(const mutex_holder & lock_hold) const
{
    return (lock_hold._get_lock_uniq_cnt() == _lock_uniq_cnt);
};

inline int mutex_locker::_acquire_hold(const mutex_holder & lock_hold) const
{
    int err = 0;

    if ((lock_hold._get_lock_uniq_cnt() & ON_MUTATE_MARK) != 0)
    {
        err = EINVAL;
    }
    else if (atomic_compare_exchange(
                            (uint64_t *)&_lock_uniq_cnt, 
                            _lock_uniq_cnt | ON_MUTATE_MARK,
                            lock_hold._get_lock_uniq_cnt()) 
                    != lock_hold._get_lock_uniq_cnt())
    {
        err = EPERM;
    }

    return err;
};

inline int mutex_locker::_release_hold(const mutex_holder & lock_hold) const
{
    int err = 0;

    if ((lock_hold._get_lock_uniq_cnt() & ON_MUTATE_MARK) != 0)
    {
        err = EINVAL;
    }
    else if (atomic_compare_exchange(
                            (uint64_t *)&_lock_uniq_cnt, 
                            _lock_uniq_cnt & ~((uint64_t)ON_MUTATE_MARK),
                            lock_hold._get_lock_uniq_cnt() | ON_MUTATE_MARK)
                    != (lock_hold._get_lock_uniq_cnt() | ON_MUTATE_MARK))
    {
        err = EPERM;
    }

    return err;
};

inline uint64_t atomic_inc_if_not_equal(volatile uint64_t * pv, const uint64_t cv)
{
    uint64_t pre_v = *pv;
    uint64_t org_v = pre_v;

    while (cv != org_v)
    {
        pre_v = atomic_compare_exchange(pv, org_v+1, org_v);
        if (pre_v == org_v)
        {
            ++org_v;
            break;
        }
        else
        {
            org_v = pre_v;
        }
    }
    
    return org_v;
};

inline uint32_t atomic_inc_if_not_equal(volatile uint32_t * pv, const uint32_t cv)
{
    uint32_t pre_v = *pv;
    uint32_t org_v = pre_v;

    while (cv != org_v)
    {
        pre_v = atomic_compare_exchange(pv, org_v+1, org_v);
        if (pre_v == org_v)
        {
            ++org_v;
            break;
        }
        else
        {
            org_v = pre_v;
        }
    }
    
    return org_v;
};

inline uint16_t atomic_inc_if_not_equal(volatile uint16_t * pv, const uint16_t cv)
{
    uint16_t pre_v = *pv;
    uint16_t org_v = pre_v;

    while (cv != org_v)
    {
        pre_v = atomic_compare_exchange(pv, org_v+1, org_v);
        if (pre_v == org_v)
        {
            ++org_v;
            break;
        }
        else
        {
            org_v = pre_v;
        }
    }
    
    return org_v;
};

inline uint8_t atomic_inc_if_not_equal(volatile uint8_t * pv, const uint8_t cv)
{
    uint8_t pre_v = *pv;
    uint8_t org_v = pre_v;

    while (cv != org_v)
    {
        pre_v = atomic_compare_exchange(pv, org_v+1, org_v);
        if (pre_v == org_v)
        {
            ++org_v;
            break;
        }
        else
        {
            org_v = pre_v;
        }
    }
    
    return org_v;
};

inline uint64_t atomic_exchange_to_larger(volatile uint64_t * pv, const uint64_t nv)
{
    uint64_t pre_v = *pv;
    uint64_t org_v = pre_v;

    while (nv > org_v)
    {
        pre_v = atomic_compare_exchange(pv, nv, org_v);
        if (pre_v == org_v)
        {
            org_v = nv;
            break;
        }
        else
        {
            org_v = pre_v;
        }
    }
    
    return org_v;
};

#endif

END_DECLARE_HB_NAMESPACE(common)

#endif

