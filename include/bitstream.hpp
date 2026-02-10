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

            /*
            64-byte alignment for L1 cache line optimization.
            On a typical x86-64 architecture, the L1 cache is 64-bytes.
            Buffer is aligned to the cache to boundaries to prevent 
            false sharing and maximize througput.
            */
            static constexpr size_t MAX_SIZE = 64;


            /*
            Constructor to construct a cache-aligned linear buffer.
            Parameter cap is the number of uint64_t elements.

            Responsible for allocating memory to 64-byte boundaries for cache efficiency.
            No zero-initialization for performance (hot-path in compression).

            throws std::bad_alloc if allocation fails.
            */
            explicit linBuffer(size_t cap) : size_(0), capacity_(cap)
            {
                //Calc required bytes
                /*
                Safety check to protect from overflow in case cap is huge
                */
                if(cap > SIZE_MAX / sizeof(uint64_t))
                {
                    throw std::invalid_argument("linBuffer::linBuffer() - buffer capacity too large!");
                }

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

            /*
            HOT PATH: push without any bounds checking
            Caller must guarantee sufficient capacity before calling.
            Used for max performace.
            */
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

            /*
            Constructs a bitWriter with specified capacity.
            Parameter capacity is the maximum number of BITS to be writen (not bytes!)

            Caller must ensure capacity is sufficient for all writes.
            No bounds checking in write_bits() hot path for performance.
            Insufficient allocation will cause buffer overflow leading to UB.

            For safe allocation keep capacity >= no_of_elements * sizeof(type)
            */
            explicit bitWriter(size_t capacity) : buffer_((capacity + 63)/64)
            {
                /*
                Invariant: Caller must pre-allocate sufficient memory to the buffer.
                No runtime bounds checking for performance purposes.
                */
                scratch_ = 0;
                bits_ = 0;
            }

            /*
            Writes variable length bits to the buffer.
            value: the value to be written
            count: number of bits to be written (bits in value) [0 - 64]

            Bits are packed into 64-bit words using little-endian order.
            When the scratch buffer fills, it is flushed to the main buffer.

            Inlined, no branch checking - caller guarantees capacity.
            */
            __attribute__((always_inline))
            inline void write_bits(uint64_t value, int count)
            {
                //Mask to count bits - handles count = 64 edge case
                if(__builtin_expect(count < 64, 1))
                {
                    value &= ((1ULL << count) - 1);
                }

                //Write bits to the scratch buffer
                scratch_ |= (value << bits_);
                bits_ += count;

                //Flush when scratch is full
                if(__builtin_expect(bits_ >= 64, 0))
                {
            
                    buffer_.unsafe_push(scratch_);
                    bits_ -= 64;

                    //Handle overflow bits that didn't fit
                    if(bits_ > 0)
                    {
                        scratch_ = value >> (count - bits_);
                    }
                    else
                    {
                        scratch_ = 0;
                    }
                }
            }

            /*
            Flush remaining bits to the buffer.
            To be called after all writes are complete to ensure data integrity.
            Pads final word with zeros if partial/incomplete.
            */
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