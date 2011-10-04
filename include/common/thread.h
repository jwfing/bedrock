#ifndef _HYPERBIRDER_COMMON_THREAD_H_
#define _HYPERBIRDER_COMMON_THREAD_H_

#include "comm_def.h"
#include <pthread.h>

DECLARE_HB_NAMESPACE(common)

class thread
{
public:
    enum
    {
        E_NO_ERROR = 0,
        E_NOT_READY = -1,
        E_FAIL_CREATE = -2,
        E_FAIL_JOIN = -3,
        E_NOT_RUNNING = -4,
        E_DETACH_FAILED = -5,
        E_ALREADY_RUNNING = -6,
        E_INTERNAL = -7,
    };

public:
    // ���캯��
    // @param(in) max_threads - �߳������ޣ�Ĭ��Ϊ10��
    thread();

    // ��������
    virtual ~thread();

    // ����һ���߳�
    // @param(in/out) thread_id - �߳�id
    // @param(in) detached - �߳��Ƿ�����־
    // @param(in) param - �߳���������
    // @return
    //      0 - succ
    //      other - failed.
    int start(bool detached = false, void * param = NULL);

    // ֹͣһ���߳�
    // @return
    //      0 - succ
    //      other - failed.
    int join();

protected:
    // �ж��߳��Ƿ�������
    // @param(in) thread_id - �߳�id
    // @return
    //    true  - �߳���������
    //    false - �߳�δ����
    inline bool is_running() const
    {
        return _is_running;
    }

    // �߳�������
    virtual void * _thread_main(void * param) = 0;
    // sample implementation
    //    {
    //        while(is_running())
    //        {
    //            // do something
    //        }
    //
    //        return 0;
    //    }

private:
    static void * _thread_launcher(void * param);

private:
    typedef enum _thread_status_t {
        DF_TS_READY = 0,
        DF_TS_RUNNING = 1,
        DF_TS_FINISHED = 2,
    } thread_status_t;

private:
    bool _detached;
    void *_param;
    volatile bool _is_running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t  _cond;
    thread_status_t _status;
    bool _ready;
};

END_DECLARE_HB_NAMESPACE(common)

#endif 
