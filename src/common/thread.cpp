#include "thread.h"

#include <assert.h>
#include <new>
#include <unistd.h>

DECLARE_HB_NAMESPACE(common)

thread::thread()
{
    _detached = false;
    _param = NULL;
    _is_running = false;
    _thread = 0;
    _status = DF_TS_READY;
    _ready = false;
    pthread_mutexattr_t mattr;
    if (pthread_mutexattr_init(&mattr) == 0)
    {
#ifdef __x86_64__
        if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL) == 0)
        {
#endif
            if (0 == pthread_mutex_init(&_mutex, &mattr))
            {
                pthread_cond_init(&_cond, NULL);
                _ready = true;
            }
#ifdef __x86_64__
        }
#endif
        pthread_mutexattr_destroy(&mattr);
    }
}

thread::~thread()
{
    if (_ready)
    {
        pthread_cond_destroy(&_cond);
        pthread_mutex_destroy(&_mutex);
        _ready = false;
    }
}

void * thread::_thread_launcher(void * param)
{
    thread * ptr = NULL;
    void * ret = NULL;

    ptr = (thread *)param;

    if (NULL != ptr && ptr->_ready)
    {
        if (ptr->_detached)
        {
            sleep(1);//�����Ա�֤pthread_create��֮ǰ���� 
            if (0 == pthread_detach(pthread_self()))
            {
                // invoke real thread entry
                ret = ptr->_thread_main(ptr->_param);
            }
            else
            {
                // Unable to detach thread
                ret = (void *) E_DETACH_FAILED;
            }
        }
        else
        {
            // invoke real thread entry
            ret = ptr->_thread_main(ptr->_param);
        }

        pthread_mutex_lock(&(ptr->_mutex));
        ptr->_status = DF_TS_FINISHED;
        ptr->_is_running = false;
        pthread_cond_broadcast( &(ptr->_cond) );
        pthread_mutex_unlock(&(ptr->_mutex));
    }

    return ret;
}

int thread::start(bool detached, void * param)
{
    int ret = 0;

    if (_ready)
    {
        // Lock
        pthread_mutex_lock(&_mutex);
 
        if (DF_TS_READY != _status)
        {
            ret = E_ALREADY_RUNNING;    
        }
        else
        {
            _detached = detached;
            _param = param;
            _status = DF_TS_RUNNING;
            _is_running = true;
 
            if (0 == pthread_create(&_thread, NULL,
                    _thread_launcher, (void *)this))
            {
                ret = E_NO_ERROR;
            }
            else
            {
                _status = DF_TS_READY;
                _is_running = false;
                ret = E_FAIL_CREATE;
            }
        }

        // Unlock
        pthread_mutex_unlock(&_mutex);
    }
    else
    {
        ret = E_NOT_READY;
    }

    return ret;
}

int thread::join()
{
    int ret = E_NO_ERROR;

    if (_ready)
    {
        if (DF_TS_READY == _status)
        {
            ret = E_NOT_RUNNING;
        }
        else
        {
            _is_running = false;

            if (!_detached)
            {
                if (0 == pthread_join(_thread, NULL))
                {
                    ret = E_NO_ERROR;
                }
                else
                {
                    ret = E_FAIL_JOIN;
                }
            }
            else
            {
                ret = E_NO_ERROR;
            }

            // Lock
            pthread_mutex_lock(&_mutex);

            while (DF_TS_RUNNING == _status)
            {
                pthread_cond_wait(&_cond, &_mutex);
            }

            // Unlock
            pthread_mutex_unlock(&_mutex);
            _status = DF_TS_READY;
        }
    }
    else
    {
        ret = E_NOT_READY;
    }

    return ret;
}

END_DECLARE_HB_NAMESPACE(common)
