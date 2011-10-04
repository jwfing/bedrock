#ifndef _HYPERBIRDER_COMMON_MEMORY_POOL_H
#define _HYPERBIRDER_COMMON_MEMORY_POOL_H

#include <pthread.h>
#include "comm_def.h"

DECLARE_HB_NAMESPACE(common)

using namespace std;

typedef size_t MP_ID;

#ifdef __64BIT__
const MP_ID MP_NULL = 0xFFFFFFFFFFFFFFFF;
const size_t MAXCHUNKNUM = 0x40000; // 2^18
const size_t MAXCHUNKSIZE = 0x00200000; // 2^21
const size_t OFFSETMASK = 0x001FFFFF;
const size_t IDXMASK = 0x3FFFF << 21;
#else
const MP_ID MP_NULL = 0xFFFFFFFF;
const size_t MAXCHUNKNUM = 0x400; // 2^10
const size_t MAXCHUNKSIZE = 0x00200000; // 2^21
const size_t OFFSETMASK = 0x001FFFFF;
const size_t IDXMASK = 0x3FF << 21;
#endif

struct element_node
{
    MP_ID next;
};

// queue structure: first in, first out
struct element_queue
{
    MP_ID head;
    MP_ID tail;
};

struct chunk_node
{
    element_node *chunk_begin;
};

class fixedlen_mempool
{
public:
    /*
     It intialize a memory pool with a specified element size.
     This function only specify the element size, but not malloc real memory.
    
     Input:
        element_size - size of element that will be put into this pool
    */
    fixedlen_mempool(size_t element_size) :
        _element_size(element_size), _chunk_number(0), _current_element_number(0)
    {
        _free_queue.head = _free_queue.tail = MP_NULL;
        _delete_queue.head = _delete_queue.tail = MP_NULL;
        _node_size = element_size + sizeof(element_node);
        _chunk_capacity = MAXCHUNKSIZE / _node_size;
        _chunk_size = _chunk_capacity * _node_size;
        for (size_t i = 0; i < MAXCHUNKNUM; i++)
        {
            _chunk_list[i].chunk_begin = NULL;
        }
        pthread_mutexattr_t attr;
        if (0 == pthread_mutexattr_init(&attr)) {
#ifdef __x86_64__
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#endif
            pthread_mutex_init(&_mutex, &attr);
            pthread_mutexattr_destroy(&attr);
        } else {
            pthread_mutex_init(&_mutex, NULL);
        }
    }

    /*
       memory_pool_t(int, size_t) is the overloaded constructor of class memory_pool_t.
       It intialize a memory pool with a specified element number and element size,
       then it try to malloc enough memory to hold these element. If there are
       not enough memory, it will use up all available.
    
      Input:
        element_num - number of elements that will be put into this pool
        element_size - size of element that will be put into this pool
    */
    fixedlen_mempool(size_t element_num, size_t element_size) :
        _element_size(element_size), _chunk_number(0), _current_element_number(0)
    {
        _free_queue.head = _free_queue.tail = MP_NULL;
        _delete_queue.head = _delete_queue.tail = MP_NULL;
        for (size_t i = 0; i < MAXCHUNKNUM; i++)
        {
            _chunk_list[i].chunk_begin = NULL;
        }
        pthread_mutexattr_t attr;
        if (0 == pthread_mutexattr_init(&attr)) {
#ifdef __x86_64__
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#endif
            pthread_mutex_init(&_mutex, &attr);
            pthread_mutexattr_destroy(&attr);
        } else {
            pthread_mutex_init(&_mutex, NULL);
        }
        reserve(element_num);
    }
    ~fixedlen_mempool()
    {
        for (size_t i = 0; i < _chunk_number; i++)
        {
            free(_chunk_list[i].chunk_begin);
        }
        pthread_mutex_destroy(&_mutex);
    }

    inline void reserve(size_t element_num)
    {
        pthread_mutex_lock(&_mutex);
        size_t chunk_needed = 0;
        // free the allocated memory
        for (size_t i = 0; i < _chunk_number; i++)
        {
            if (NULL != _chunk_list[i].chunk_begin)
            {
                free(_chunk_list[i].chunk_begin);
                _chunk_list[i].chunk_begin = NULL;
            }
        }
        _recycle_Nodes();
        _free_queue.head = _free_queue.tail = MP_NULL;

        // compute the size of each chunk and how many chunks needed
        _node_size = _element_size + sizeof(element_node);
        _chunk_capacity = MAXCHUNKSIZE / _node_size;
        _chunk_size = _chunk_capacity * _node_size;

        chunk_needed = element_num / _chunk_capacity + 1;
        _chunk_number = 0;

        // alloc _chunk_number chunks
        // if alloc failed, modify _chunk_number as real chunk number alloced
        for (size_t i = 0; i < chunk_needed && i < MAXCHUNKNUM; i++)
        {
            if (0 == _add_a_chunk())
            {
                break;
            }
        }
        pthread_mutex_unlock(&_mutex);
    }

    inline void *getStruct(size_t mp_id)
    {
        void* retval = NULL;
        if (mp_id != MP_NULL)
        {
        	if (NULL != _get_node_address(mp_id))
        	{
                retval = (void *)((char *) _get_node_address(mp_id) + sizeof(element_node));
        	}
        }
        return retval;
    }

    /*
       alloc() alloc a node from the memory pool. It first try to look for a
       node in free_queue. If free_queue is empty, then it calls _add_a_chunk()
       to malloc a block of memory from OS.
       If _add_a_chunk still failed, this means memory has run out,
       it returns MP_NULL.
    
      Return:
        MP_ID   -   the MP ID of the node that alloced
        MP_NULL when there a no node alloced because real memory has run out
    
    */
    inline MP_ID alloc()
    {
        pthread_mutex_lock(&_mutex);
        MP_ID ret;

        ret = _pop_out(_free_queue); // try to take of the first node from _free_queue

        // if _free_queue is empty, add a new chunk
        // this need a malloc() from real memory
        if (ret == MP_NULL)
        {
            if (0 == _add_a_chunk())
            {
                pthread_mutex_unlock(&_mutex);
                return MP_NULL;
            }
            ret = _pop_out(_free_queue);
        }
        if (ret != MP_NULL)
        {
            _current_element_number++;
        }
        pthread_mutex_unlock(&_mutex);
        return ret;
    }

    /*
       deleteNode(MP_ID) put a node to _delete_queue.
       Nodes in _delete_queue can not be alloced until _delete_queue is
       merged into _free_queue.
    
       Input:
         dlt_node    -   the MP ID of the node that is to be deleted
    */
    inline void deleteNode(MP_ID dlt_node)
    {
        pthread_mutex_lock(&_mutex);
        _push_back(_delete_queue, dlt_node);
        _current_element_number--;
        pthread_mutex_unlock(&_mutex);
    }

    /*
       freeNode(MP_ID) put a node to _free_queue.
        Nodes in _free_queue can be alloced immediately.
    
       Input:
         freeNode   -   the MP ID of the node that is to be freed
     */
    inline void freeNode(MP_ID freeNode)
    {
        pthread_mutex_lock(&_mutex);
        _push_back(_free_queue, freeNode);
        _current_element_number--;
        pthread_mutex_unlock(&_mutex);
    }

    /*
     *  recycleNodes() merge _delete_queue into _free_queue
     */
    inline void recycleNodes()
    {
        pthread_mutex_lock(&_mutex);
        _recycle_Nodes();
        pthread_mutex_unlock(&_mutex);
    }

    inline size_t getChunkNumber()
    {
        return _chunk_number;
    }

    inline size_t getChunkCapacity()
    {
        return _chunk_capacity;
    }

    inline size_t getCapacity()
    {
        return _chunk_number * _chunk_capacity;
    }

    inline size_t getElementNumber()
    {
        return _current_element_number;
    }

private:
    ////==========================_push_back()=============================== **
    //   _push_back(element_queue&, MP_ID) push a node to the end of a queue
    //
    //   Input:
    //     queue    -   the queue that the node will be added to
    //     mpid -   the MP ID of the node
    //
    ////=================================================================== **
    inline void _push_back(element_queue & queue, MP_ID mpid)
    {
        element_node *end_node;
        if (mpid == MP_NULL)
        { // invalid input
            return;
        }
        if (queue.head == MP_NULL)
        { // as the first node
            queue.head = mpid;
            queue.tail = mpid;
        }
        else
        { // add to the tail of the queue
            end_node = _get_node_address(queue.tail);
            end_node->next = mpid;
            queue.tail = mpid;
        }

        // this is important!!!
        // let the tail node's next point to MP_NULL
        end_node = _get_node_address(queue.tail);
        end_node->next = MP_NULL;
    }

    ////==========================_pop_out()================================== **
    //   _pop_out(element_queue&) : take a node out from a queue
    //
    //   Input:
    //     queue    -   the queue that the node will be taken out
    //
    ////===================================================================== **
    inline MP_ID _pop_out(element_queue & queue)
    {
        MP_ID ret;
        ret = queue.head;
        element_node *hdnode;
        hdnode = (element_node *) _get_node_address(queue.head); // if the queue is empty, hdnode would be NULL
        if (NULL != hdnode)
        { // if the queue is not empty
            queue.head = hdnode->next;
            hdnode->next = MP_NULL;
            if (queue.head == MP_NULL)
            { // if the last node has been taken off
                queue.tail = MP_NULL;
            }
        }

        return ret;
    }

    ////==========================_add_a_chunk()============================= **
    //  _add_a_chunk() calls malloc() to add a memory chunk to _chunk_list.
    //     It failed either memory has run out or memory pool is full.
    //
    //   Return:
    //     0    -   failed
    //     1    -   succeed
    //
    ////==================================================================== **
    inline int _add_a_chunk()
    {
        // has reached the top limit of chunk number
        if (_chunk_number == MAXCHUNKNUM)
        {
            return 0;
        }

        size_t chunk_idx = _chunk_number;
        _chunk_list[chunk_idx].chunk_begin = (element_node *) malloc(_chunk_size);
        if (NULL == _chunk_list[chunk_idx].chunk_begin)
        {
            // this can only happen when system overload is too heavy
            // memory run out, malloc return NULL
            return 0;
        }

        // format this chunk as a queue
        size_t offset = 0;
        for (size_t i = 0; i < _chunk_capacity; i++)
        {
            size_t ndid = (chunk_idx << 21) | (offset & OFFSETMASK);
            offset += _node_size;
            _push_back(_free_queue, ndid);
        }
        _chunk_number++;

        return 1;
    }

    ////==========================_free_a_queue()============================== **
    //   mp_free_queue(element_queue&) merge a queue into free_queue
    //
    //   Input:
    //     queue    -   the head node of the queue that is to be merged
    //
    ////===================================================================== **
    inline void _free_a_queue(element_queue & queue)
    {
        if (queue.head == MP_NULL)
        {
            return;
        }

        if (_free_queue.head == MP_NULL)
        {
            _free_queue.head = queue.head;
            _free_queue.tail = queue.tail;
        }
        else
        {
            element_node *node = (element_node *) _get_node_address(_free_queue.tail);
            node->next = queue.head;
            _free_queue.tail = queue.tail;
        }
        queue.head = MP_NULL;
        queue.tail = MP_NULL;
    }

    inline element_node *_get_node_address(size_t mp_id)
    { //���mp��ַ���ʵ���ڴ��ַ
        element_node* retval = NULL;
        if (mp_id == MP_NULL)
        {
            retval = NULL;
        }
        else
        {
            if ((mp_id >> 21) > _chunk_number)
            {
                retval = NULL;
            }
            else
            {
                retval = (element_node *) ((char *) _chunk_list[mp_id >> 21].
                chunk_begin + (mp_id & OFFSETMASK));
            }
        }
        return retval;
    }

    inline void _recycle_Nodes()
    {
        _free_a_queue(_delete_queue);
    }

private:
    chunk_node _chunk_list[MAXCHUNKNUM];
    size_t _chunk_size;
    size_t _chunk_capacity;
    size_t _element_size;
    size_t _node_size;
    size_t _chunk_number;
    size_t _current_element_number;
    element_queue _free_queue;
    element_queue _delete_queue;
    pthread_mutex_t _mutex;
};

END_DECLARE_HB_NAMESPACE(common)

#endif

