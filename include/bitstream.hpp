#pragma once

#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <new>

/*
The ieee754.hpp header takes care of breaking the double down to its constituent bits.
Once the double is broken down, we need a way to write variable length bits to memory.

std::vector is a safe and convenient choice for this, but it is relatively slow with
expensive overheads.

To deal with this I designed a custom linear buffer that is cache aligned.
The buffer memory is explicitly managed using std::aligned_alloc and
the buffer size is aligned to 64 bytes (typical CPU cache line size).

This is to ensure that all load/stores interact cleanly with the cache hierarchy.
*/

namespace comp
{
    /*
    Custom Container: linBuffer
        - 64 bytes (cache friendly)
        - no hot path resizing (fixed size/manual check)
        - no zero initialization
    */

    class linBuffer
    {
        public:

            static constexpr size_t MAX_SIZE = 64; //Aligned to 64 bytes (typical L1 cache line width)

            explicit linBuffer(size_t cap) : size_(0), capacity_(cap)
            {
                //Calc required bytes
                size_t reqdBytes = cap * sizeof(uint64_t);

                if(reqdBytes % MAX_SIZE != 0)
                {
                    reqdBytes += MAX_SIZE - (reqdBytes % MAX_SIZE); //Ensures allocation size is a multiple of MAX_SIZE (alignment)
                }

                data_ = static_cast<uint64_t*>(std::aligned_alloc(MAX_SIZE, reqdBytes));

                if(!data_)
                {
                    throw std::bad_alloc();
                }
            }

            ~linBuffer()
            {
                if(data_)
                {
                    std::free(data_);
                }
            }

            /*
            Copy constructor and copy assignment operator are deleted to
            avoid accidental expensive copies
            */

            linBuffer(const linBuffer &) = delete;
            linBuffer &operator=(const linBuffer &) = delete;

            //Move constructor preserved for cheap transfer of data
            linBuffer(linBuffer &&dup) noexcept
                : data_(dup.data_), size_(dup.size_), capacity_(dup.capacity_)
            {
                dup.data_ = nullptr;
                dup.size_ = 0;
                dup.capacity_ = 0;
            }

            //HOT PATH: unsafe push (caller guarantees capacity)
            inline void unsafe_push(uint64_t val) __attribute__((always_inline))
            {
                data_[size_++] = val;
            }

            //Accessors
            const uint64_t *front() const
            {
                return data_;
            }

            const uint64_t *back() const
            {
                return data_ + size_;
            }

            size_t size() const
            {
                return size_;
            }

            size_t capacity() const
            {
                return capacity_;
            }

            void clear()
            {
                /*
                Resets without deallocating memory (to be used carefully)
                */
               size_ = 0;
            }

            uint64_t *data() //dangerous, to be used with caution
            {
                return data_; 
            }

        private:
            
            uint64_t *data_;
            size_t size_;
            size_t capacity_;
    };

    class bitWriter
    {
        public:

            explicit bitWriter(size_t capacity) : buffer_((capacity + 63)/64)
            {
                scratch_ = 0;
                bits_ = 0;
            }

            inline void write_bits(uint64_t value, int count)
            {
                assert(count <= 64); //to be removed in final build

                value &= (count == 64) ? ~0ULL : ((1ULL << count) - 1);

                scratch_ |= (value << bits_);
                bits_ += count;

                if(bits_ >= 64)
                {
                    if(buffer_.size() >= buffer_.capacity())
                    {
                        throw std::runtime_error("Buffer Overflow: Pre-allocate more memory.");
                    }

                    buffer_.unsafe_push(scratch_);

                    bits_ -= 64;
                    scratch_ = (bits_ > 0) ? (value >> (count - bits_)) : 0;
                }
            }

            void flush()
            {
                if(bits_ > 0)
                {
                    if(buffer_.size() >= buffer_.capacity())
                    {
                        throw std::runtime_error("Buffer Overflow.");
                    }

                    buffer_.unsafe_push(scratch_);
                    bits_ = 0;
                    scratch_ = 0;
                }
            }

            const linBuffer &get_buffer() const
            {
                return buffer_;
            }

            size_t byteSize() const
            {
                return (buffer_.size() * sizeof(uint64_t)) + ((bits_ + 7) / 8);
                //High bits of final word are undefined, decoder must keep track of bit count.
            }

        private:

            linBuffer buffer_;
            uint64_t scratch_;
            int bits_;
    };

    class bitReader
    {
        public:

            // constructor that accepts raw pointers
            bitReader(const uint64_t *data, size_t size)
                : data_(data), size_(size), index_(0), posn_(0) { }
            
            inline uint64_t read_bits(int count)
            {
                if(index_ >= size_)
                {
                    return 0;
                }

                uint64_t value{0};
                uint64_t curr{data_[index_]};

                uint64_t avail_{static_cast<uint64_t>(64) - posn_};

                if(static_cast<uint64_t>(count) <= avail_)
                {
                    uint64_t mask = (count == 64) ? ~0ULL : ((1ULL << count) - 1);
                    value = (curr >> posn_) & mask;

                    posn_ += count;

                    if(posn_ == 64)
                    {
                        posn_ = 0;
                        index_++;
                    }
                }
                else
                {
                    value = (curr >> posn_);
                    int taken_{static_cast<int>(avail_)};
                    int leftover_{count - taken_};

                    index_++;
                    if(index_ < size_)
                    {
                        uint64_t next_{data_[index_]};
                        value |= (next_ & ((1ULL << leftover_) - 1)) << taken_;
                    }

                    posn_ = leftover_;
                }

                return value;
            }

        private:

            const uint64_t *data_;
            size_t size_;
            size_t index_;
            int posn_;
    };
}