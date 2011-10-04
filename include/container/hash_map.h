#ifndef _HYPERBIRD_COMMON_COMMON_HASH_TEMPLATE_H
#define _HYPERBIRD_COMMON_COMMON_HASH_TEMPLATE_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <utility>
#include <iterator>
#include <new>

#include "common_def.h"

DECLARE_HB_NAMESPACE(common)

using namespace std;
typedef signed char char8;
typedef unsigned char u_char8;

#define BEFORE_STARTING_POS ((void*)-1L)

//! Template hash table( key-value pair )
/*
 Requirements for TElemnet and TKey:
 They can be any fix-size type or class which has overloaded '=' opeartor
 eg. int, WString, struct, union, and any other l-value type.
 NOTE: Neither pointer nor array are valid type.
 eg. PSTR, char[24], float[10]
 void* can be used as valid type. But with care!
 They must have a default constructor or a user-define constructor without parameters.
 ownership policy:
 TKey and TElement are copyed using '=' operator.
 They are owned by HashTemplate.
 Becareful when a narrow-copy/share-copy '=' operator is defined.

 HashTemplate quick reference:
 Insert/Retrieve:	insert / []
 Search entry:		find / Lookup
 Remove entry:		Remove
 Iteration:			FindFirst/FindNext
 Clear the table:	clear

 Reserve memory(not necessary): resize/reserve
 Empty  array(not necessary): RemoveAll
 */

static const uint32_t internal_prime_list[] =
{ 13ul, 53ul, 97ul, 193ul, 389ul, 769ul, 1543ul, 3079ul, 6151ul, 12289ul, 24593ul, 49157ul,
        98317ul, 196613ul, 393241ul, 786433ul, 1572869ul, 2500009, 3145739ul, 6291469ul,
        12582917ul, 25165843ul, 50331653ul, 100663319ul, 201326611ul, 402653189ul, 805306457ul,
                1610612741ul, 3221225473ul, 4294967291ul };

template<class T1, class T2> inline void obj_construct(T1 * p, const T2 & value)
{
    new(p) T1(value); // placement new; invoke ctor T1(value);
}

template<class T1> inline void obj_construct(T1 * p)
{
    new(p) T1(); // placement new; invoke ctor T1();
}

/*!
 Note: in the above obj_construct(T1* p), placement-new does nothing for internal
 data types, i.e. it doesn't invoke T1().  We have to define specialized ones.

 verified using gcc-2.96
 */

inline void obj_construct(char *p)
{
    *p = 0;
}
inline void obj_construct(short *p)
{
    *p = 0;
}
inline void obj_construct(int *p)
{
    *p = 0;
}
inline void obj_construct(long *p)
{
    *p = 0;
}
inline void obj_construct(float *p)
{
    *p = 0.0;
}
inline void obj_construct(double *p)
{
    *p = 0.0;
}
inline void obj_construct(unsigned char *p)
{
    *p = 0;
}
inline void obj_construct(unsigned short *p)
{
    *p = 0;
}
inline void obj_construct(unsigned int *p)
{
    *p = 0;
}
inline void obj_construct(unsigned long *p)
{
    *p = 0;
}

template<class T> inline void obj_destroy(T * pointer)
{
    pointer -> ~T();
}

template<class TKey, class TElement>
class HashTemplate
{
public:

    //! structure for interface compatible with STL's map template
    class iterator :
        public std::iterator < std::forward_iterator_tag, pair < const TKey, TElement> >
    {
public:
        // in gcc 2.96, it seems that value_type can't be inherited from std::iterator
        typedef pair < const TKey, TElement> value_type;
        typedef TKey key_type;

        iterator()
        {
            ptr = NULL;
            pMap = NULL;
        }

        iterator(void *p)
        {
            ptr = (value_type *) p;
            pMap = NULL;
        }

        iterator(void *pEntry, void *map)
        {
            ptr = (value_type *) pEntry;
            pMap = (HashTemplate < TKey, TElement> *)map;
        }

        inline bool operator ==(const iterator & iter)
        {
            return iter.ptr == ptr;
        }
        inline bool operator !=(const iterator & iter)
        {
            return iter.ptr != ptr;
        }

        //! prefix: increase and then fetch
        iterator & operator ++()
        {
            TKey *pKey;
            TElement *pValue;
            void *pos;
            pos = (char *) ptr - sizeof(SEntry *);
            if (pMap->FindNext(pos, pKey, pValue))
                ptr = (value_type *) ((u_char8 *) pos + sizeof(SEntry *));
            else
                ptr = NULL;
            return *this;
        }

        //! postfix: fetch and then increase
        const iterator operator ++(int)
        {
            iterator tmp = *this;
            TKey *pKey;
            TElement *pValue;
            void *pos;
            pos = (u_char8 *) ptr - sizeof(SEntry *);
            if (pMap->FindNext(pos, pKey, pValue))
                ptr = (value_type *) ((u_char8 *) pos + sizeof(SEntry *));
            else
                ptr = NULL;

            return tmp;
        }

        inline value_type & operator *() const
        {
            return (value_type &) *ptr;
        }

        inline value_type *operator ->() const
        {
            return (value_type *) ptr;
        }

protected:
        value_type * ptr;
        HashTemplate < TKey, TElement> *pMap;
    };

    typedef typename iterator::key_type key_type;
    typedef typename iterator::value_type value_type;
    typedef typename iterator::pointer pointer;
    typedef typename iterator::reference reference;

    iterator begin()
    {
        TKey *pKey;
        TElement *pValue;
        void *pos;
        if (FindFirst(pos, pKey, pValue))
        {
            return iterator((u_char8 *) pos + sizeof(SEntry *), this);
        }
        else
        {
            return NULL;
        }
    }

    //! destroy all entries in the map
    //! @notice the function does not free all the memory allcated by this map
    void recycle()
    {
        assert(m_pHashTable);
        if (m_pHashTable != NULL)
        {
            if (m_nCount > 0)
            {
                uint32_t nHash;
                SEntry *pEntry;
                SEntry *pNext;
                for (nHash = 0; nHash < m_nHashTableSize; nHash++)
                {
                    for (pEntry = m_pHashTable[nHash]; pEntry != NULL;)
                    {
                        pNext = pEntry->pNext;
                        FreeEntry(pEntry); // recycle the memory to m_pFreeList
                        pEntry = pNext;
                    }
                }
                assert(m_nCount == 0);
                assert(m_pFreeList != NULL);
            }
            memset(m_pHashTable, 0, sizeof(SEntry *) * m_nHashTableSize);
        }
    }

    //! destroy all entries in the map
    //! @notice the function does not free all the memory allcated by this map
    void clear()
    {
        assert(m_pHashTable);
        if (m_pHashTable != NULL)
        {
            if (m_nCount > 0)
            {
                uint32_t nHash;
                SEntry *pEntry;
                SEntry *pNext;
                for (nHash = 0; nHash < m_nHashTableSize; nHash++)
                {
                    for (pEntry = m_pHashTable[nHash]; pEntry != NULL;)
                    {
                        pNext = pEntry->pNext;
                        FreeEntry(pEntry); // recycle the memory to m_pFreeList
                        pEntry = pNext;
                    }
                }
                assert(m_nCount == 0);
                assert(m_pFreeList != NULL);
            }
            memset(m_pHashTable, 0, sizeof(SEntry *) * m_nHashTableSize);
        }

        if (m_pBlocks != NULL)
        {
            m_pBlocks->FreeDataChain();
        }
        m_pBlocks = NULL;
        m_pFreeList = NULL; // need not free m_pFreeList, as it's alias of free space in m_pBlocks
    }

    bool empty() const
    {
        return (m_nCount == 0);
    }

    iterator end() const
    {
        return NULL;
    }

    bool insert(const TKey & key, const TElement & value)
    {
        iterator iter;
        iter = find(key);
        if (iter != end())
        {
            iter->second = value;
            return false;
        }
        else
        {
            SafeAdd(key, value);
            return true;
        }
    }

    pair < iterator, bool> insert(const value_type & x)
    {
        const TKey & key = x.first;
        const TElement & value = x.second;
        pair < iterator, bool> ret;
        iterator iter;

        iter = find(key);
        if (iter != end())
        {
            iter->second = value;
            ret.second = false;
        }
        else
        {
            uint32_t nHash;
            SEntry *pEntry;
            pEntry = NewEntry(key);
            nHash = HashValue(key) % m_nHashTableSize;

            // put into hash table
            pEntry->pNext = m_pHashTable[nHash];
            m_pHashTable[nHash] = pEntry;

            pEntry->value = value;
            iter = iterator((u_char8 *) pEntry + sizeof(SEntry *), this);
            ret.second = true;
        }
        ret.first = iter;
        return ret;
    }

    inline iterator find(const TKey & key)
    {
        SEntry *pEntry = GetEntryAt(key);
        if (pEntry == NULL)
        {
            return NULL;
        }
        else
        {
            return iterator((u_char8 *) pEntry + sizeof(SEntry *), this);
        }
    }

    inline uint32_t size() const
    {
        return m_nCount;
    }

    void swap(HashTemplate & other)
    {
        uint32_t t;
        SEntry **pt;
        SDataChain *st;
        SEntry *ft;

        t = other.m_nCount;
        other.m_nCount = m_nCount;
        m_nCount = t;
        t = other.m_nBlockSize;
        other.m_nBlockSize = m_nBlockSize;
        m_nBlockSize = t;
        t = other.m_nHashTableSize;
        other.m_nHashTableSize = m_nHashTableSize;
        m_nHashTableSize = t;
        pt = other.m_pHashTable;
        other.m_pHashTable = m_pHashTable;
        m_pHashTable = pt;
        st = other.m_pBlocks;
        other.m_pBlocks = m_pBlocks;
        m_pBlocks = st;
        ft = other.m_pFreeList;
        other.m_pFreeList = m_pFreeList;
        m_pFreeList = ft;
    }

    inline void reserve(uint32_t nHint, uint32_t nBlockSize = 0)
    {
        ReSize(nHint, nBlockSize);
    }

    inline void resize(uint32_t nHint, uint32_t nBlockSize = 0)
    {
        ReSize(nHint, nBlockSize);
    }

    /*
     the constructor, you may specify a hint size for the hash table

     Note: nHint in 10% - 200% of total hash entries works well
     BlockSize = 10 is sufficient in most case
     */
    explicit HashTemplate(int nHint = 10, int nBlockSize = 0)
    {
        m_nHashTableSize = 0;
        m_nBlockSize = 0;
        m_nCount = 0;
        m_pHashTable = NULL;
        m_pFreeList = NULL;
        m_pBlocks = NULL;
        ReSize(nHint, nBlockSize);
    }

    explicit HashTemplate(const HashTemplate & other)
    {
        m_nHashTableSize = 0;
        m_nBlockSize = 0;
        m_nCount = 0;
        m_pHashTable = NULL;
        m_pFreeList = NULL;
        m_pBlocks = NULL;
 
        *this = other;
    }

    ~HashTemplate()
    {
        RemoveAll();
    }

    //! add a new entry,  key must be unique
    TElement *Add(const TKey & key)
    {
        uint32_t nHash;
        SEntry *pEntry;
        assert(m_pHashTable != NULL);
        assert(GetEntryAt(key) == NULL);

        // always add a new entry
        pEntry = NewEntry(key);
        nHash = HashValue(key) % m_nHashTableSize;

        // put into hash table
        pEntry->pNext = m_pHashTable[nHash];
        m_pHashTable[nHash] = pEntry;
        return &(pEntry->value); // return pointer to created element
    }

    //! safely add a new entry,  key must be unique
    TElement *SafeAdd(const TKey & key, const TElement & value)
    {
        uint32_t nHash;
        SEntry *pEntry;
        assert(m_pHashTable != NULL);
        assert(GetEntryAt(key) == NULL);

        // always add a new entry
        pEntry = NewEntry(key);
        nHash = HashValue(key) % m_nHashTableSize;

        // set value for new entry
        pEntry->value = value;

        // put into hash table
        pEntry->pNext = m_pHashTable[nHash];
        m_pHashTable[nHash] = pEntry;

        return &(pEntry->value); // return pointer to created element
    }

    bool FindFirst(void *&pos, TKey * &rKey, TElement * &rValue) const
    {
        if (m_nCount == 0)
        {
            pos = NULL;
            rKey = NULL;
            rValue = NULL;
            return false;
        }
        else
        {
            pos = BEFORE_STARTING_POS;
            return FindNext(pos, rKey, rValue);
        }
    }

    //! @sa FindFirst
    bool FindNext(void *&pos, TKey * &rKey, TElement * &rValue) const
    {
        uint32_t nBucket;
        SEntry *pEntryRet = (SEntry *) pos;
        assert(m_pHashTable != NULL);
        // never call on empty map

        if (pEntryRet == (SEntry *) BEFORE_STARTING_POS)
        {
            // find the first entry
            for (nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
                if ((pEntryRet = m_pHashTable[nBucket]) != NULL)
                    break;
            assert(pEntryRet != NULL);
            // must find something
        }
        else
        {
            // find next entry
            if (NULL != pEntryRet->pNext)
            {
                pEntryRet = pEntryRet->pNext;
            }
            else
            {
                // go to next bucket
                uint32_t nHash = HashValue(pEntryRet->key) % m_nHashTableSize;
                pEntryRet = NULL; // set default
                for (nBucket = nHash + 1; nBucket < m_nHashTableSize; nBucket++)
                {
                    if ((pEntryRet = m_pHashTable[nBucket]) != NULL)
                    {
                        break;
                    }
                }
            }
        }

        // fill in return data
        if (pEntryRet == NULL)
        {
            pos = NULL;
            rKey = NULL;
            rValue = NULL;
            return false;
        }
        else
        {
            pos = (void *) pEntryRet;
            rKey = &(pEntryRet->key);
            rValue = &(pEntryRet->value);
            return true;
        }
    }

    //! number of elements
    inline uint32_t GetCount() const
    {
        return m_nCount;
    }

    //! whether created
    inline bool IsValid() const
    {
        return m_pHashTable != NULL;
    }

    /*! Lookup table
     @return whether found
     */
    bool Lookup(const TKey & key) const
    {
        SEntry *pEntry = GetEntryAt(key);
        if (pEntry == NULL)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    /*! Lookup table
     @param key search to find record matching key
     @param pValue return the pointer of stored value, if found
     @return whether found
     */
    bool Lookup(const TKey & key, TElement * &pValue) const
    {
        SEntry *pEntry = GetEntryAt(key);
        if (pEntry == NULL)
        {
            pValue = NULL;
            return false;
        }
        pValue = &(pEntry->value);
        return true;
    }

    /*! Lookup table
     @param key search to find record matching key
     @param rValue store value if the key was found in the table
     @return whether found
     */
    bool Lookup(const TKey & key, TElement & rValue) const
    {
        SEntry *pEntry = GetEntryAt(key);
        if (pEntry == NULL)
        {
            return false; // not in map
        }
        rValue = (pEntry->value);
        return true;
    }

    //! Lookup or add if not found
    TElement & operator[](const TKey & key)
    {
        SEntry *pEntry;
        assert(m_pHashTable != NULL);
        if ((pEntry = GetEntryAt(key)) == NULL)
        {
            return *(Add(key));
        }
        else
        {
            return pEntry->value; // return new reference
        }
    }

    //! overload
    const TElement operator[](const TKey & key) const
    {
        SEntry *pEntry;
        assert(m_pHashTable != NULL);
        if ((pEntry = GetEntryAt(key)) == NULL)
        {
            return TElement();
        }
        else
        {
            return pEntry->value;
        }
    }

    //! Duplicate map, do deep copy
    const HashTemplate < TKey, TElement> &operator =(const HashTemplate & src)
    {
        if (this != &src)
        {
            RemoveAll();
            ReSize(src.m_nHashTableSize);

            void *pos;
            TKey *pKey;
            TElement *pValue;
            bool more = src.FindFirst(pos, pKey, pValue);
            while (more)
            {
                (*this)[*pKey] = *pValue;
                more = src.FindNext(pos, pKey, pValue);
            }
        }
        return *this;
    }

    //! removing existing key-value pair
    //! @return whether succeessful
    bool Remove(const TKey & key)
    {
        if (m_pHashTable == NULL)
        {
            return false; // nothing in the table
        }

        SEntry **ppEntryPrev;
        ppEntryPrev = &m_pHashTable[HashValue(key) % m_nHashTableSize];

        SEntry *pEntry;
        for (pEntry = *ppEntryPrev; pEntry != NULL; pEntry = pEntry->pNext)
        {
            if (pEntry->key == key)
            {
                // remove it
                *ppEntryPrev = pEntry->pNext; // remove from list
                FreeEntry(pEntry);
                return true;
            }
            ppEntryPrev = &pEntry->pNext;
        }
        return false; // not found
    }

    /*!
     @brief set the size of hashtable
     @note always call this function before other operations
     @param nHashSize the possible total entries in this map
     */
    void ReSize(uint32_t nHint, uint32_t nBlockSize = 0)
    {
        uint32_t i;
        uint32_t nHashSize;

        if (nBlockSize <= 0)
        {
            nBlockSize = 120 / sizeof(SEntry);
            if (nBlockSize < 10)
            {
                nBlockSize = 10;
            }
        }

        if (m_nBlockSize < nBlockSize)
        {
            m_nBlockSize = nBlockSize;
        }

        uint32_t nListSize = sizeof(internal_prime_list) / sizeof(internal_prime_list[0]);
        for (i = 0; i < nListSize; i++)
        {
            if (internal_prime_list[i] >= nHint)
            {
                break;
            }
        }
        nHashSize = internal_prime_list[i];

        if (NULL == m_pHashTable)
        {
            m_pHashTable = new SEntry *[nHashSize];
            memset(m_pHashTable, 0, sizeof(SEntry *) * nHashSize);
            m_nHashTableSize = nHashSize;
        }
        else
        {
            SEntry *pEntry;
            SEntry **ppNewtable;
            uint32_t i, nNewhash, nOld;
            nOld = m_nHashTableSize;
            ppNewtable = new SEntry *[nHashSize];
            memset(ppNewtable, 0, sizeof(SEntry *) * nHashSize);

            for (i = 0; i < nOld; i++)
            {
                pEntry = m_pHashTable[i];
                while (pEntry)
                {
                    nNewhash = HashValue(pEntry->key) % nHashSize;
                    m_pHashTable[i] = pEntry->pNext;
                    pEntry->pNext = ppNewtable[nNewhash];
                    ppNewtable[nNewhash] = pEntry;
                    pEntry = m_pHashTable[i];
                }
            }

            delete[] m_pHashTable;
            m_pHashTable = ppNewtable;
            m_nHashTableSize = nHashSize;
        }
    }

protected:
    //internal structures
    struct SEntry
    {
        SEntry *pNext;
        TKey key;
        TElement value;
    };

    // linked list
    struct SDataChain // warning variable length structure
    {
        SDataChain *pNext;
        uint32_t nSize; // allocated size, added to align on 8 byte boundary(it's not a requirement)

        // memory: pNext, nSize, data, pNext, nSize, data ...
        void *data()
        {
            return this + 1;
        }

        // like 'calloc' but no zero fill
        static SDataChain *Create(SDataChain * &pHead, uint32_t nMax, uint32_t cbElement)
        {
            assert(nMax> 0 && cbElement> 0);
            uint32_t nSize = sizeof(SDataChain) + nMax * cbElement;
            SDataChain *p = (SDataChain *) new u_char8[nSize];
            p->nSize = nSize;
            p->pNext = pHead;
            pHead = p; // change head (adds in reverse order for simplicity)
            return p;
        }

        void FreeDataChain() // free this one and links
        {
            SDataChain *p = this;
            while (p != NULL)
            {
                u_char8 *bytes = (u_char8 *) p;
                SDataChain *pNext = p->pNext;
                delete[] bytes;
                p = pNext;
            }
        }
    };

protected:
    SEntry ** m_pHashTable; //the entry table whose index is the hash_value
    struct SDataChain *m_pBlocks; // trunk-allocated space to store the hash entries
    SEntry *m_pFreeList; // pointer to the available room in the trunk
    uint32_t m_nHashTableSize; //!< size of hash table
    uint32_t m_nBlockSize; //!< # of batch allocated SEntry objects
    uint32_t m_nCount; //!< key-value pairs in the table

    // Implementation
    SEntry *NewEntry(const TKey & key)
    {
        //      SEntry's are singly linked all the time
        //      static const TKey zeroKey = TKey();
        //      static const TElement zeroElement = TElement();
        int32_t i;
        SEntry * pEntry;

        // automatically expand the hash table for efficiency
        if (m_nCount > 4 * m_nHashTableSize)
        {
            ReSize(m_nCount);
        }

        if (m_pFreeList == NULL)
        {
            // add another block
            SDataChain *newBlock = SDataChain::Create(m_pBlocks, m_nBlockSize, sizeof(SEntry));
            // chain them into free list
            pEntry = (SEntry *) newBlock->data();

            // free in reverse order to make it easier to debug
            pEntry += m_nBlockSize - 1;
            for (i = m_nBlockSize - 1; i >= 0; i--, pEntry--)
            {
                pEntry->pNext = m_pFreeList;
                m_pFreeList = pEntry;
            }
        }
        assert(m_pFreeList != NULL);
        // we must have something

        pEntry = m_pFreeList;
        m_pFreeList = m_pFreeList->pNext;
        m_nCount++;

        obj_construct(&pEntry->key, key);
        obj_construct(&pEntry->value);
        return pEntry;
    }

    // find entry (or return NULL)
    inline SEntry *GetEntryAt(const TKey & key) const
    {
        assert(m_pHashTable != NULL);
        // never call on empty map
        if (m_pHashTable == NULL)
        {
            return NULL;
        }

        uint32_t nHash = HashValue(key) % m_nHashTableSize;

        // see if it exists
        SEntry *pEntry;
        for (pEntry = m_pHashTable[nHash]; pEntry != NULL; pEntry = pEntry->pNext)
        {
            if (MapCompareKey(pEntry->key, key) == true)
            {
                return pEntry;
            }
        }
        return NULL;
    }

    void FreeEntry(SEntry * pEntry)
    {
        // free up string data
        pEntry->key. ~ TKey();
        pEntry->value. ~ TElement();
        pEntry->pNext = m_pFreeList;
        m_pFreeList = pEntry;
        m_nCount--;
    }

    //! remove all entries, free all the allocated memory
    void RemoveAll()
    {
        if (m_pHashTable != NULL)
        { // destroy elements
            if (m_nCount > 0)
            {
                uint32_t nHash;
                for (nHash = 0; nHash < m_nHashTableSize; nHash++)
                {
                    SEntry *pEntry;
                    for (pEntry = m_pHashTable[nHash]; pEntry != NULL; pEntry = pEntry->pNext)
                    {
                        pEntry->key. ~ TKey();
                        pEntry->value. ~ TElement();
                    }
                }
            }
            // free hash table
            delete[] m_pHashTable;
            m_pHashTable = NULL;
        }

        m_nCount = 0;
        if (m_pBlocks != NULL)
        {
            m_pBlocks->FreeDataChain();
            m_pBlocks = NULL;
        }
        /*
         m_pFreeList pointed memory are part of m_pBlocks pointed,
         therefore it need not free
         */
        m_pFreeList = NULL;
        m_nHashTableSize = 0;
        m_nBlockSize = 0;
    }

#ifdef _DEBUG
    // for performace testing
public:
    void stat()
    {
        if (m_pHashTable && m_nCount)
        { // destroy elements
            const uint32_t BIN_CNT = 10;
            uint32_t nHash;
            uint32_t nUsed = 0, nMax = 0, nTotal = 0, nLength;
            uint32_t ayDist[BIN_CNT];
            memset(ayDist, 0, sizeof(ayDist));
            for (nHash = 0; nHash < m_nHashTableSize; nHash++)
            {
                SEntry *pEntry;
                nLength = 0;
                for (pEntry = m_pHashTable[nHash]; pEntry != NULL; pEntry = pEntry->pNext)
                {
                    ++nLength;
                }
                nTotal += nLength;
                if (nLength > 0)
                {
                    ++nUsed;
                }
                if (nLength > nMax)
                {
                    nMax = nLength;
                }
                if (nLength < BIN_CNT)
                {
                    ++ayDist[nLength];
                }
            }
            printf("%d table size, %d items, %d table-cell used, %.2f%% coverage\n",
                    m_nHashTableSize, m_nCount, nUsed, 100.0 * nUsed / m_nHashTableSize);
            printf("Link length: average %.2f, max %d\n", 1.0 * nTotal / nUsed, nMax);
            for (uint32_t i = 0; i < BIN_CNT; ++i)
            {
                printf("%d\t%d\n", i, ayDist[i]);
            }
        }
    }
#endif

};

//! compare key
template<class TKey>
inline bool MapCompareKey(const TKey & key1, const TKey & key2)
{
    return (key1 == key2);
}

/*!
 calculate hash key
 in the manner of an additive linear congruential
 random number generator
 */

inline uint32_t HashValue(const void *pIn, uint32_t size)
{
    register const u_char8 *pKey = (const u_char8 *) pIn;
    uint32_t sign1 = 0;

    if (size <= sizeof(sign1))
    {
        memcpy(&sign1, pKey, size);
    }
    else
    {
        register const u_char8 *pEnd = pKey + size;
        for (; pKey < pEnd; ++pKey)
        {
            sign1 = (sign1 << 4) + sign1 + *pKey;
        }
    }
    return sign1;
}

template<class TKey> inline uint32_t HashValue(const TKey & key)
{
    return HashValue(&key, sizeof(TKey));
}

// !specialized function template
inline bool MapCompareKey(const char *key1, const char *key2)
{
    if (strcmp(key1, key2) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

inline uint32_t HashValue(const char *str)
{
    if (!str || !*str)
    {
        return 0;
    }
    else
    {
        return HashValue(str, strlen(str));
    }
}

inline uint32_t HashValue(int n)
{
    return n;
}

/*!
 @brief TSignature is a specialized TKey class for using in HashTemplate.
 It transforms the actural key to signatures through its set_key() function.

 By this way, variable-size objects (usually C strings) can be treated as fixed-size signatures

 Benefits:
 need not store original keys(for later compare)
 fixed size storage
 fast hashing and compare
 Shortcomings:
 can't restore original keys;
 in very very rare cases, different keys may generate same signatures
 */

struct TSignature
{
    TSignature() :
        sign1(0), sign2(0)
    {
    }

    TSignature(int32_t s1, int32_t s2) :
        sign1(s1), sign2(s2)
    {
    }

    template<class TKey> TSignature(const TKey & key)
    {
        set_key(key);
    }

    TSignature(const TSignature & other)
    {
        *this = other;
    }

    const TSignature & operator =(const TSignature & other)
    {
        sign1 = other.sign1;
        sign2 = other.sign2;
        return *this;
    }

    bool operator ==(const TSignature & other) const
    {
        if (other.sign1 == sign1 && other.sign2 == sign2)
            return true;
        else
            return false;
    }

    bool operator !=(const TSignature & other) const
    {
        if (other.sign1 != sign1 || other.sign2 != sign2)
            return true;
        else
            return false;
    }

    void set_key(const void *pIn, uint32_t size)
    {
        register const u_char8 *pKey = (const u_char8 *) pIn;
        sign1 = 0;
        sign2 = 0;

        if (size <= sizeof(sign1) + sizeof(sign2))
        {
            memcpy(m_pBytes, pKey, size);
        }
        else
        {
            register const u_char8 *pEnd = pKey + size;

            for (; pKey < pEnd; ++pKey)
            {
                sign1 *= 16777619; // 16777619 == 2^24 + 403
                sign1 ^= *pKey;
                sign2 = (sign2 << 4) + sign2 + *pKey;
            }
        }
    }

    template<class TKey> void set_key(const TKey & key)
    {
        set_key(&key, sizeof(key));
    }

    template<class TKey> void set_key(const TKey key)
    {
        set_key(&key, sizeof(key));
    }

    void set_sign(int s1, int s2)
    {
        sign1 = s1;
        sign2 = s2;
    }

    union
    {
        struct
        {
            uint32_t sign1;
            uint32_t sign2;
        };
        char m_pBytes[8];
    };
};

template<> inline uint32_t HashValue(const TSignature & sig)
{
    return sig.sign1 + sig.sign2;
}

#ifdef _WSTRING_H_
typedef HashTemplate <WString, int32_t> MapWString2Int;
inline bool MapCompareKey(const WString & key1, const WString & key2)
{
    return (0 == key1.Compare(key2));
}

inline uint32_t HashValue(const WString & key)
{
    return key.HashValue();
}

inline void TSignature::set_key(const WString & key)
{
    set_key(PCSTR(CStr(key)));
}
#endif //_WSTRING_H_

END_DECLARE_HB_NAMESPACE(common)

#endif
