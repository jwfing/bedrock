#ifndef _HYPERBIRDER_COMMON_HEAP_H_
#define _HYPERBIRDER_COMMON_HEAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "comm_def.h"

DECLARE_HB_NAMESPACE(common)

template <typename T> class heap {
public:
	static void make(T* begin, T* end);

	static void pop(T* begin, T* end);

	static void push(T* begin, T* end);

	static void sort(T* begin, T* end);

	static void adjust(T* first, int root_index, int heap_len);

private:
	static inline void _swap(T& a, T& b)
	{
		T tmp = a;
		a = b;
		b = tmp;
	}
};


template <typename T> void heap<T>::make(T* begin, T* end)
{
	assert(begin != NULL && end != NULL);
	if (NULL != begin && NULL != end)
	{
	    int heap_len = end - begin;
	    if (heap_len >= 1)
	    {
	        for (int i = heap_len/2; i >= 1; i--)
	        {
	            adjust(begin, i, heap_len);
	        }
	    }
	}
}

template <typename T> void heap<T>::adjust(T* first, int root_index,
		int heap_len)
{
	int rt = root_index;
	int left = -1;
	int right = -1;
	int smallest = -1;

	while (rt * 2 <= heap_len)
	{
		left = (rt << 1);
		right = (rt << 1) + 1;
		smallest = rt;
		if (*(first + left - 1) < *(first + smallest - 1))
		{
			smallest = left;
		}
		if (right <= heap_len && *(first + right - 1) < *(first + smallest - 1))
		{
			smallest = right;
		}
		if (smallest != rt)
		{
			_swap(*(first + smallest - 1), *(first + rt - 1));
			rt = smallest;
		}
		else
		{
			break;
		}
	}
}

template <typename T> void heap<T>::sort(T* begin, T* end)
{
	assert(begin != NULL && end != NULL);
	if (NULL != begin && NULL != end)
	{
	    int len = end - begin;
	    if (begin < end - 1)
	    {
	        make(begin, end);
	    }
	    while (begin < end - 1)
	    {
	        pop(begin, end);
	        --end;
	    }

	    for (int i = 0; i < len/2; ++i)
	    {
	        _swap(*(begin+i), *(begin + len - 1 - i));
	    }
	}
}

template <typename T> void heap<T>::push(T* begin, T* end)
{
	assert(begin != NULL && end != NULL);
	if (NULL != begin && NULL != end)
	{
    	int len = end - begin;
    	int now = len;
    	int parent = -1;
    	while (now > 1)
    	{
    		parent = (now >> 1);
    		if (*(begin + now - 1) < *(begin + parent - 1))
    		{
    			_swap(*(begin + now - 1), *(begin + parent - 1));
    			now = parent;
    		}
    		else
    		{
    			break;
    		}
    	}
	}
}

template <typename T> void heap<T>::pop(T* begin, T* end)
{
	assert(begin != NULL && end != NULL);
	if (NULL != begin && NULL != end)
	{
    	_swap(*begin, *(end - 1));
    	adjust(begin, 1, end - begin - 1);
	}
}

END_DECLARE_HB_NAMESPACE(common)

#endif /*_HYPERBIRDER_COMMON_HEAP_H_*/
