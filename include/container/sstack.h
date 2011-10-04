#ifndef _HYPERBIRDER_COMMON_SSTACK_H
#define _HYPERBIRDER_COMMON_SSTACK_H

#include "comm_def.h"

END_DECLARE_HB_NAMESPACE(common)

class slink
{
    friend class sstack;

public:
    slink() : _next(NULL)
    {
        // empty
    }
    slink(slink* next) : _next(next)
    {
        // empty
    }
    ~slink()
    {
        // empty
    }

protected:
    // ����nextָ��
    inline void _set_next(slink* next)
    {
        _next = next;
    }
    // ��ȡnextָ��
    inline slink* _get_next(void) const
    {
        return _next;
    }

private:
    slink* _next;
};

class sstack
{
public:
    sstack();
    ~sstack();

public:
    int push(slink* node);
    int pop(slink** node);

    inline bool is_empty(void) const
    {
        return 0 == _stack_num;
    }

    inline uint64_t get_stack_num(void) const
    {
        return _stack_num;
    }

private:
    slink* _merge_lists(slink* list1, slink* list2);

private:
    slink* _top;
    volatile uint64_t _stack_num;
};

END_DECLARE_HB_NAMESPACE(common)

#endif
