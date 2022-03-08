#ifndef __HUGE_INTEGER__
#define __HUGE_INTEGER__


#include <string>
#include <cstdint>
#include <ostream>



class HugeInt
{
public:
    // Signed integer type
    using BaseInt = int32_t;
    // Unsigned integer type. Representation of huge int is either an array of unsigned integers or just a single unsigned integer.
    using BaseUint = uint32_t;
    // Double base unsigned integer type. Used to handle multiplication overloads.
    using DoubleBaseUint = uint64_t;

private:
    // Union to hold pointer to array of integers or a static single integer in order to reduce memory allocations.
    union Data
    {
        BaseUint *ptr; // Pointer to array of integers if the size > 0. Last bit of the last element is a sign bit.
        // (using unsigned integers is actually easier)
        BaseUint static_val; // Single integer if the size == 0 (technically is 1). Last bit of it is a sign bit actually.
    } data;
    // Size of the array of integers. Equals to zero if there's only static single integer.
    size_t size;

    /**
     * @brief Private constructor to manually manage memory.
     *
     * @param size Size of the array that will represent huge integer.
     * @param additional_capacity Additional capacity that'll tell to allocate more memory (size + additional_capacity)
     * but the this->size will remain size. This's necessary for ex. in addition, where an overload may happen and we allocate 1 int more
     * memory to store the overloaded bits. However, if an overload doesn't happen, we don't want 'to show' that the size is more than
     * needed, because this way multiple additions will cause more unused memory to be allocated. If an overload does occur, the size will be
     * increased to match the capacity.
     */
    HugeInt(size_t size, size_t additional_capacity);

    /**
     * @brief Get i-th element from the array representation or just single integer if size == 0, i == 0.
     * Basically - (BaseUint)(huge_int >> ((sizeof(BaseUint) * i))).
     */
    inline BaseUint get_BaseUint(size_t i) const
    {
        if (size > 0)
        {
            if (i < size)
                return data.ptr[i];
            else return 0;
        }
        else if (i == 0)
            return data.static_val;
        else return 0;
    }

    /**
     * @brief Set i-th element from the array representation or just single integer if size == 0, i == 0.
     */
    inline void set_BaseUint(size_t i, BaseUint base_int)
    {
        if (size > 0)
        {
            if (i < size)
                data.ptr[i] = base_int;
        }
        else if (i == 0)
            data.static_val = base_int;
    }

    /**
     * @brief If size == 0, it just means that we're using data.static_val but techincally size is 1 in this case.
     * @note There can be a case when size and technical size == 1 and the data is stored in dynamic array of just single element.
     * However, in that case 'techincally' we don't care.
     */
    inline size_t get_TechnicalSize() const
    {
        if (size > 0)
            return size;
        else
            return 1;
    }

    /**
     * @brief Clear, make zero.
     */
    inline void clear()
    {
        if (size > 0)
            std::fill(data.ptr, data.ptr + size, 0);
        else
            data.static_val = 0;
    }

    /**
     * @brief Copy and shift contents of this to another huge integer, without changing its size.
     */
    inline void copy_and_shift_ContentsTo(HugeInt &other, size_t shift) const
    {
        if (size > 0)
            std::copy(data.ptr, data.ptr + size, other.data.ptr + shift);
        else if (other.size > 0)
            other.data.ptr[shift] = data.static_val;
        else
            other.data.static_val = shift > 0 ? 0 : data.static_val;
    }
public:
    /**
     * @brief Add 'b' to 'a' with a carry. Assuming 'a' is long enough to not overload. Returns a new carry.
     */
    static char sum(HugeInt &a, const HugeInt &b, char carry = 0);
    /**
     * @brief Multiply 'a' and 'b' and store in 'c' using a temporary variable of the same size. Assuming all of them are
     * long enough to not overload.
     */
    static void multiply(const HugeInt &a, const HugeInt &b, HugeInt &c, HugeInt &temp_var);
    /**
     * @brief Multiply a huge int by a base int. Assuming 'a' is long enough to handle overloads.
     */
    static void multiply_by_BaseUint(HugeInt &a, BaseUint b);

    // Initialize from base integer.
    HugeInt(BaseInt value);
    // Initialize from string
    HugeInt(std::string value);

    // HugeInt stores (may store) a resource, so the big 5 rule
    HugeInt(const HugeInt &other);
    HugeInt(HugeInt &&other) noexcept;
    HugeInt& operator=(const HugeInt &rhs);
    HugeInt& operator=(HugeInt &&rhs) noexcept;
    ~HugeInt();


    HugeInt operator+(const HugeInt &arg) const;
    HugeInt operator*(const HugeInt &arg) const;


    std::string to_String() const;
};


std::ostream& operator<<(std::ostream &os, const HugeInt &x);


#endif
