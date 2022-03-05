#include "huge_integer.h"
#include <vector>



HugeInt::HugeInt(BaseInt val) : size(0)
{
    data.static_val = val;
}


HugeInt::HugeInt(std::string value)
{
    // 
}


HugeInt::HugeInt(size_t size, size_t additional_capacity)
{
    size_t capacity = size + additional_capacity;
    if (capacity > 1)
    {
        this->size = size;
        data.ptr = new BaseInt[capacity];
    }
    else
    {
        this->size = 0;
        data.static_val = 0;
    }
}


HugeInt::HugeInt(const HugeInt &other) : data(other.data), size(other.size)
{
    if (size > 0)
    {
        data.ptr = new BaseInt[size];
        std::copy(other.data.ptr, other.data.ptr + size, data.ptr);
    }
}


HugeInt::HugeInt(HugeInt &&other) noexcept : data(other.data), size(other.size)
{
    if (other.size > 0)
    {
        other.data.ptr = nullptr;
        other.size = 0;
    }
}


HugeInt& HugeInt::operator=(const HugeInt &rhs)
{
    if (rhs.size > 0)
    {
        auto *data_ptr = new BaseInt[rhs.size];
        std::copy(rhs.data.ptr, rhs.data.ptr + rhs.size, data_ptr);
        if (size > 0)
            delete[] data.ptr;
        data.ptr = data_ptr;
    }
    else
        data = rhs.data;
    size = rhs.size;
    return *this;
}


HugeInt& HugeInt::operator=(HugeInt &&rhs) noexcept
{
    if (size > 0)
        delete[] data.ptr;
    
    data = rhs.data;
    size = rhs.size;

    if (rhs.size > 0)
    {
        rhs.data.ptr = nullptr;
        rhs.size = 0;
    }
    return *this;
}


HugeInt::~HugeInt()
{
    if (size > 0)
        delete[] data.ptr;
}


static inline char sign(long int x)
{
    return x > 0 ? 1 : x ? -1 : 0;
}


static inline char sum_BaseInts(HugeInt::BaseInt a, HugeInt::BaseInt b, char c, HugeInt::BaseInt &s)
{
    using b_uint = HugeInt::BaseUint;
    s = a + b + c;
    return b_uint(b) >= b_uint(~c) || b_uint(b + c) >= b_uint(~a);
}


HugeInt HugeInt::operator+(const HugeInt &arg) const
{
    if (arg.size == 0 && size == 0)
    {
        BaseInt result;
        if (sum_BaseInts(arg.data.static_val, data.static_val, 0, result))
        {
            HugeInt huge_result(2, 0);
            huge_result.data.ptr[0] = result;
            huge_result.data.ptr[1] = -sign(result);
            return huge_result;
        }
        else
            return HugeInt(result);
    }
    else
    {
        HugeInt result(std::max(arg.size, size), 1);
        char carry = 0;
        for (size_t i = 0; i < result.size; ++i)
        {
            carry = sum_BaseInts(get_BaseInt(i), arg.get_BaseInt(i), carry, result.data.ptr[i]);
        }
        if (carry)
        {
            result.data.ptr[result.size] = -sign(result.get_BaseInt(result.size - 1));
            ++result.size;
        }
        return result;
    }
}


HugeInt HugeInt::operator*(const HugeInt &arg) const
{

}