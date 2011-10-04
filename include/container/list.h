#ifndef _HYPERBIRDER_COMMON_LIST_INCLUDE_H_
#define _HYPERBIRDER_COMMON_LIST_INCLUDE_H_

#include <new>
#include "comm_def.h"

DECLARE_HB_NAMESPACE(common)

template <typename T>
class list
{
public:
    typedef struct _list_node
    {
        _list_node* _next;
        T _data;
        _list_node() : _next(NULL)
        {
        }
    } list_node;

public:
    list()
    {
        _head = NULL;
        _tail = NULL;
        _count = 0;
    }
    virtual ~list()
    {
        list_node* tmp = NULL;
        while (NULL != _head)
        {
            tmp = _head;
            _head = _head->_next;
            delete tmp;
            tmp = NULL;
        }
        clear();
    }

public:
    static list_node* alloc_node()
    {
        list_node* node = new(std::nothrow) list_node();
        if (NULL != node)
        {
            node->_next = NULL;
        }
        return node;
    }

    static void free_node(list_node*& node)
    {
        if (NULL != node)
        {
            delete node;
            node = NULL;
        }
    }
    inline int64_t count(void)
    {
        return _count;
    }

    inline list_node* get_first(void)
    {
        return _head;
    }

    inline int64_t remove(list_node* node)
    {
        int64_t retval = 0;
        list_node* prev = _head;
        if (NULL != _head && NULL != node)
        {
            if (_head == node)
            {
                _head = _head->_next;
                if (NULL == _head)
                {
                    _tail = NULL;
                }
                _count--;
            }
            else
            {
                while (NULL != prev->_next && prev->_next != node)
                {
                    prev = prev->_next;
                }
                if (NULL != prev->_next)
                {
                    if (_tail == node)
                    {
                        _tail = prev;
                    }
                    prev->_next = node->_next;
                    _count--;
                }
                else
                {
                    retval = -1;
                }
            }
        }
        else
        {
            retval = -1;
        }
        return retval;
    }

    inline int64_t add(list_node* node)
    {
        int64_t retval = 0;
        list_node* prev = _head;
        if (NULL == node)
        {
            retval = -1;
        }
        else
        {
            while(NULL != prev && prev != node)
            {
                prev = prev->_next;
            }
            if (NULL != prev)
            {
                retval = -1;
            }
            else
            {
                if (0 == _count)
                {
                    _head = node;
                    _tail = node;
                }
                else
                {
                    _tail->_next = node;
                    _tail = node;
                }
                node->_next = NULL;
                _count++;
                retval = 0;
            }
        }
        return retval;
    }

    void clear()
    {
        _head = NULL;
        _tail = NULL;
        _count = 0;
    }

private:
    list_node* _head; // header
    list_node* _tail; // tailer
    int64_t _count;   // counter
};

END_DECLARE_HB_NAMESPACE(common)

#endif
