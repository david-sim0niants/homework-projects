#include "huge_integer.h"
#include <vector>
#include <sstream>
#include <iomanip>



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
        if (size >= rhs.size)
            std::copy(rhs.data.ptr, rhs.data.ptr + rhs.size, data.ptr);
        else
        {
            auto *data_ptr = new BaseUint[rhs.size]();
            std::copy(rhs.data.ptr, rhs.data.ptr + rhs.size, data_ptr);
            if (size > 0)
                delete[] data.ptr;
            data.ptr = data_ptr;
        }
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


void HugeInt::sum(HugeInt &a, const HugeInt &b)
{
    char carry = 0;
    for (size_t i = 0; i < a.get_TechnicalSize(); ++i)
        a.set_BaseUint(i, sum_BaseUints(a.get_BaseUint(i), b.get_BaseUint(i), carry));

    char a_sign = sign(a.get_BaseUint(a.size - 1));
    char b_sign = sign(b.get_BaseUint(b.size - 1));

    if (carry && a_sign * b_sign > 0)
    {
        a.set_BaseUint(a.size, a_sign); // set last integer as a 'sign integer'
        ++a.size; // the signed sum overloaded, so the size now matches capacity
    }
}


void HugeInt::multiply_by_int(HugeInt &a, BaseUint b)
{
    DoubleBaseUint m = 0;
    static constexpr size_t base_int_bits = sizeof(BaseUint) * 8;
    static constexpr DoubleBaseUint leading_ones_filter = DoubleBaseUint(~0) >> base_int_bits;
    for (size_t i = 0; i < a.get_TechnicalSize(); ++i)
    {
        m = (DoubleBaseUint(a.get_BaseUint(i)) & leading_ones_filter) * (b & leading_ones_filter) + (m >> base_int_bits);
        a.set_BaseUint(i, m);
    }
}


HugeInt HugeInt::operator+(const HugeInt &arg) const
{
    if (size == 0 && arg.size == 0)
    {
        // case when both operands are statically sized integers and we check if the sum of them can also be like them
        char this_sign = sign(data.static_val);
        char arg_sign = sign(arg.data.static_val);
        char carry = 0;
        BaseUint result = sum_BaseUints(data.static_val, arg.data.static_val, carry);

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
    HugeInt product_by_int(result.size, 0);


    for (size_t i = 0; i < arg_size; ++i)
    {
        product_by_int.clear();
        copy_and_shift_ContentsTo(product_by_int, i);
        multiply_by_int(product_by_int, arg.get_BaseUint(i));
        sum(result, product_by_int);
    }
    return result;
}


std::string HugeInt::to_String() const
{
    std::stringstream ss;
    ss << "0x" << std::hex;
    for (size_t i = get_TechnicalSize() - 1; i >= 0; --i)
    {
        ss << get_BaseUint(i);
        ss << std::setfill('0') << std::setw(sizeof(BaseUint) * 2);
        if (i == 0)
            break;
    }
    return ss.str();
}