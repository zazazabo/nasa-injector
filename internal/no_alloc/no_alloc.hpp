/*
    Author: xerox
    Date: 4/23/2020

    example:
        std::vector<int, going_internal::no_alloc<int, 30>> test;
        test.reserve(30);
*/

#pragma once
#include <memory>

#define HEAP_SIZE 0x2000

// taken from:
// https://github.com/charles-salvia/charles/blob/master/stack_allocator.hpp
template <class T, std::size_t heap_size = sizeof(T) * 10, class Allocator = std::allocator<T>>
class no_alloc
{
public:

    typedef typename std::allocator_traits<Allocator>::value_type value_type;
    typedef typename std::allocator_traits<Allocator>::pointer pointer;
    typedef typename std::allocator_traits<Allocator>::const_pointer const_pointer;
    typedef typename Allocator::reference reference;
    typedef typename Allocator::const_reference const_reference;
    typedef typename std::allocator_traits<Allocator>::size_type size_type;
    typedef typename std::allocator_traits<Allocator>::difference_type difference_type;
    typedef typename std::allocator_traits<Allocator>::const_void_pointer const_void_pointer;
    typedef Allocator allocator_type;

public:
    explicit no_alloc(const allocator_type& alloc = allocator_type())
        : m_allocator(alloc), m_begin(this->heap), m_end(this->heap + heap_size),
        m_stack_pointer(this->heap)
    { }

    template <class U>
    no_alloc(const no_alloc<U, heap_size, Allocator>& other)
        : m_allocator(other.m_allocator), m_begin(other.m_begin), m_end(other.m_end),
        m_stack_pointer(other.m_stack_pointer)
    { }

    constexpr static size_type capacity()
    {
        return heap_size;
    }

    pointer allocate(size_type n, const_void_pointer hint = const_void_pointer())
    {
        pointer result = m_stack_pointer;
        m_stack_pointer += n;
        return result;
    }

    void deallocate(pointer p, size_type n)
    {
        m_stack_pointer -= n;
    }

    size_type max_size() const noexcept
    {
        return m_allocator.max_size();
    }

    template <class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        m_allocator.construct(p, std::forward<Args>(args)...);
    }

    template <class U>
    void destroy(U* p)
    {
        m_allocator.destroy(p);
    }

    pointer address(reference x) const noexcept
    {
        if (pointer_to_internal_buffer(std::addressof(x)))
        {
            return std::addressof(x);
        }

        return m_allocator.address(x);
    }

    const_pointer address(const_reference x) const noexcept
    {
        if (pointer_to_internal_buffer(std::addressof(x)))
        {
            return std::addressof(x);
        }

        return m_allocator.address(x);
    }

    template <class U>
    struct rebind { typedef no_alloc<U, heap_size, allocator_type> other; };

    pointer buffer() const noexcept
    {
        return m_begin;
    }

private:

    bool pointer_to_internal_buffer(const_pointer p) const
    {
        return (!(std::less<const_pointer>()(p, m_begin)) && (std::less<const_pointer>()(p, m_end)));
    }

    allocator_type m_allocator;
    pointer m_begin;
    pointer m_end;
    pointer m_stack_pointer;
    T heap[heap_size];
};

template <class T1, std::size_t heap_size, class Allocator, class T2>
static bool operator == (const no_alloc<T1, heap_size, Allocator>& lhs,
    const no_alloc<T2, heap_size, Allocator>& rhs) noexcept
{
    return lhs.buffer() == rhs.buffer();
}

template <class T1, std::size_t heap_size, class Allocator, class T2>
static bool operator != (const no_alloc<T1, heap_size, Allocator>& lhs,
    const no_alloc<T2, heap_size, Allocator>& rhs) noexcept
{
    return !(lhs == rhs);
}

inline no_alloc<std::uint8_t, HEAP_SIZE> heap;
void* operator new(std::size_t size);
void operator delete(void* ptr, unsigned __int64);
void* malloc(std::size_t size);
void free(void*, std::size_t size);