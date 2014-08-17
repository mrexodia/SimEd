#ifndef _DYNAMICMEM_H
#define _DYNAMICMEM_H

#define emalloc(size) malloc(size)
#define erealloc(ptr, size) realloc(size, ptr)
#define efree(ptr) free(ptr)

template<class T>
class Memory
{
public:
    Memory(const char* reason = "Memory:???")
    {
        mPtr = 0;
        mSize = 0;
        mReason = reason;
    }

    Memory(size_t size, const char* reason = "Memory:???")
    {
        mPtr = reinterpret_cast<T>(emalloc(size));
        mSize = size;
        mReason = reason;
        memset(mPtr, 0, size);
    }

    ~Memory()
    {
        efree(mPtr);
    }

    T realloc(size_t size, const char* reason = "Memory:???")
    {
        mPtr = reinterpret_cast<T>(erealloc(mPtr, size));
        mSize = size;
        mReason = reason;
        memset(mPtr, 0, size);
        return mPtr;
    }

    template<class U>
    operator U()
    {
        return (U)mPtr;
    }

    operator T()
    {
        return mPtr;
    }

    size_t size()
    {
        return mSize;
    }

private:
    T mPtr;
    size_t mSize;
    const char* mReason;
};

#endif //_DYNAMICMEM_H
