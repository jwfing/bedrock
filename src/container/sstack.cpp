#include "sstack.h"
#include "atomic.h"

DECLARE_HB_NAMESPACE(common)

sstack::sstack()
{
    _top = 0;
    _stack_num = 0;
}

sstack::~sstack()
{
    // does nothing
}

int sstack::push(hbc_slink* node)
{
    int err = 0;
    hbc_slink* old_v = NULL;

    if (NULL == node)
    {
        DF_WRITE_LOG(UL_LOG_WARNING, "Invalid param, node=%p", node);
        err = -1;
    }
    else
    {
        do
        {
            old_v = _top;
            if (node == old_v)
            {
                DF_WRITE_LOG(UL_LOG_FATAL, "the node to be pushed is the same as stack top, "\
                    "node=%p, old_v=%p", node, old_v);
                err = -1;
                break;
            }
            node->_set_next(old_v);
        }
        while (atomic_compare_exchange_pointer((volatile pvoid*)&_top, node, old_v) != old_v);

        if (0 == err)
        {
            atomic_inc((volatile uint64_t*) &_stack_num);
        }
    }
    return err;
}

int sstack::pop(hbc_slink** node)
{
    int err = 0;
    hbc_slink* current = NULL;
    hbc_slink* head = NULL;
    bool exchange_success = false;
    bool has_elem = true;

    if (NULL == node)
    {
        DF_WRITE_LOG(UL_LOG_WARNING, "Invalid param, node=%p", node);
        err = -1;
    }
    else
    {
        *node = NULL;

        do
        {
            if (0 == _stack_num)
            {
                has_elem = false; // û��Ԫ��
                break;
            }
            current = _top;
        } while (NULL == current || atomic_compare_exchange_pointer((volatile pvoid*)&_top, NULL, current)
                                                                     != current);

        if (true == has_elem)
        {
            if (NULL == current)
            {
                DF_WRITE_LOG(UL_LOG_FATAL, "current should be non-null");
                err = -1;
            }
            else
            {
                atomic_dec(&_stack_num);
                *node = current;

                current = current->_get_next();

                if (NULL != *node) // always succeed
                {
                    (*node)->_set_next(NULL);
                }

                if (NULL != current)
                {
                    while (!exchange_success)
                    {
                        head = (hbc_slink*) atomic_compare_exchange_pointer(
                                        (volatile pvoid*) &_top, current, NULL);

                        if (NULL != head)
                        {
                            head = (hbc_slink*) atomic_exchange_pointer((volatile pvoid*) &_top, NULL);
                            current = _merge_lists(current, head);
                        }
                        else
                        {
                            exchange_success = true;
                        }
                    }
                }
            }
        }
    }
    return err;
}

hbc_slink* sstack::_merge_lists(hbc_slink* list1, hbc_slink* list2)
{
    hbc_slink* result = NULL;
    hbc_slink* p1 = NULL;
    hbc_slink* q1 = NULL;

    hbc_slink* p2 = NULL;
    hbc_slink* q2 = NULL;

    if (NULL != list1 || NULL != list2)
    {
        p1 = list1;
        p2 = list2;
        if (NULL == p1)
        {
            result = p2;
        }
        else if (NULL == p2)
        {
            result = p1;
        }
        else
        {
            while (NULL != p1 && NULL != p2)
            {
                q1 = p1;
                q2 = p2;
                p1 = p1->_get_next();
                p2 = p2->_get_next();
            }
            if (NULL == p1)
            {
                q1->_set_next(list2);
                result = list1;
            }
            else
            {
                q2->_set_next(list1);
                result = list2;
            }
        }
    }

    return result;
}

END_DECLARE_HB_NAMESPACE(common)
