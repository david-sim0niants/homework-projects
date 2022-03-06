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


HugeInt::HugeInt(const HugeInt &other) : data(other.data), size(other.size)
{
    if (size > 0)
    {
        data.ptr = new BaseUint[size]();
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
        auto *data_ptr = new BaseUint[rhs.size]();
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


HugeInt::HugeInt(size_t size, size_t additional_capacity)
{
    size_t capacity = size + additional_capacity;
    if (capacity > 1)
    {
        this->size = size;
        data.ptr = new BaseUint[capacity]();
    }
    else
    {
        this->size = 0;
        data.static_val = 0;
    }
}


static inline char sign(HugeInt::BaseInt x)
{
    return x > 0 ? 1 : x ? -1 : 0;
}
/**
 * @brief Sum of two base unsigned integers and a carry.
 *
 * @param a First int
 * @param b Second int
 * @param c Carry
 * @return Sum
 */
static inline HugeInt::BaseUint sum_BaseUints(HugeInt::BaseUint a, HugeInt::BaseUint b, char &c)
{
    c = b >= ~c || (b + c) >= ~a;
    return a + b + c;
}


HugeInt HugeInt::operator+(const HugeInt &arg) const
{
    if (arg.size == 0 && size == 0)
    {
        // case when both operands are statically sized integers and we check if the sum of them can also be like them
        char this_sign = sign(data.static_val);
        char arg_sign = sign(arg.data.static_val);
        char carry;
        BaseUint result = sum_BaseUints(arg.data.static_val, data.static_val, carry);

        // if unsigned sum overloaded (aka returned a carry) and signed arguments we identically signed, then 'signed sum' also overloaded
        if (carry && this_sign * arg_sign > 0)
        {
            // case when the signed sum overloads and we need to store it dynamically
            HugeInt huge_result(2, 0);
            huge_result.data.ptr[0] = result;
            huge_result.data.ptr[1] = this_sign;
            return huge_result;
        }
        else
            return HugeInt(result); // case when the signed sum doesn't overload and can be stored statically
    }
    else
    {
        // case when at least one of operands is already 'huge' and dynamically stored and then so is the sum of them
        HugeInt result(std::max(arg.size, size), 1); // 1 int more capacity for overloaded sum
        char carry = 0;
        for (size_t i = 0; i < result.size; ++i)
        {
            result.set_BaseUint(i, sum_BaseUints(get_BaseUint(i), arg.get_BaseUint(i), carry));
        }

        char this_sign = sign(get_BaseUint(result.size - 1));
        char arg_sign = sign(arg.get_BaseUint(result.size - 1));

        if (carry && this_sign * arg_sign > 0)
        {
            result.set_BaseUint(result.size, this_sign); // set last integer as a 'sign integer'
            ++result.size; // the signed sum overloaded, so the size now matches capacity
        }
        return result;
    }
}


HugeInt HugeInt::operator*(const HugeInt &arg) const
{
    size_t size = get_TechnicalSize();
    size_t arg_size = arg.get_TechnicalSize();
    HugeInt result(size + arg_size, 0);

    for (size_t i = 0; i < size; ++i)
    {
        for (size_t j = 0; j < arg_size; ++j)
        {
        }
    }

    return result;
}