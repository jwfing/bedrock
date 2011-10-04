#ifndef _HYPERBIRDER_COMMON_VECTOR_H_
#define _HYPERBIRDER_COMMON_VECTOR_H_

#include <stdlib.h>
#include <new>
#include <assert.h>

#include "comm_def.h"

END_DECLARE_HB_NAMESPACE(common)

template <typename T>
class vector
{
public:
    vector(uint32_t incsize, uint32_t initnum = 0);
    virtual ~vector();

public:
    inline T* get(uint32_t idx) const;
    inline uint32_t put(const T& t);
    inline T* begin(void) const
    {
        return _chk;
    }
    inline T* end(void) const
    {
        return _chk + _size;
    }
    inline void reset(void)
    {
        _size = 0;
    }
    inline int reserve(uint32_t capacity)
    {
        int ret = -1;
        if (capacity <= 0)
        {
            DF_WRITE_LOG(UL_LOG_WARNING, "param is invalid(capacity:%u)", capacity);
        }
        else
        {
            reset();
            _capacity = capacity;
            if (_chk)
            {
                delete[] _chk;
            }
            _chk = new(std::nothrow) T[capacity];
            if (NULL != _chk)
            {
                ret = 0;
            }
        }
        return ret;
    }
    inline int shrink(uint32_t new_size)
    {
        int ret = -1;
        if (new_size > _size)
        {
            DF_WRITE_LOG(UL_LOG_WARNING, "param is invalid(original_size:%u, new_size:%u)",
                    _size, new_size);
        }
        else
        {
            _size = new_size;
            ret = 0;
        }
        return ret;
    }
    inline uint32_t size(void) const
    {
        return _size;
    }
    inline uint32_t incsize(void) const
    {
        return _incsize;
    }
    inline uint32_t capacity(void) const
    {
        return _capacity;
    }

private:
    vector(const vector<T> & vec);
    vector<T> operator=(vector<T> & vec);

private:
    uint32_t _incsize;
    uint32_t _size;
    uint32_t _capacity;
    T* _chk;
};

template <typename T> vector<T>::vector(uint32_t incsize, uint32_t initnum)
    : _incsize(incsize), _size(0)
{
    assert(incsize != 0);

    if (0 == _incsize)
    {
        _incsize = 1;
    }
    _capacity = initnum;
    _chk = new(std::nothrow) T[initnum];
}

template <typename T> vector<T>::~vector()
{
    if (NULL != _chk)
    {
        delete[] _chk;
        _chk = NULL;
    }
}

template <typename T> uint32_t vector<T>::put(const T& t)
{
    T* new_chk = NULL;
    if (_size >= capacity())
    {
        DF_WRITE_LOG_DEBUG("resize the vector.");
        _capacity += _incsize;
        new_chk = new(std::nothrow) T[_capacity];
        for (uint32_t i = 0; i < _size; ++i)
        {
            new_chk[i] = _chk[i];
        }
        if (NULL != _chk)
        {
            delete[] _chk;
        }
        _chk = new_chk;
    }

    _chk[_size++] = t;
    return _size - 1;
}

template <typename T> T* vector<T>::get(uint32_t idx) const
{
    return (idx < _size) ? (_chk+idx) : NULL;
}

END_DECLARE_HB_NAMESPACE(common)

#endif /*DF_VECTOR_H_*/
