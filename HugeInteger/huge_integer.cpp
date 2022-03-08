#include "huge_integer.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <regex>
#include <limits>
#include <cmath>



HugeInt::HugeInt(BaseInt val) : size(0)
{
    data.static_val = val;
}


static void extract_FirstNumber(std::string &value, bool &is_negative)
{
    std::regex regex("(\\d+|-\\d+)");
    std::sregex_iterator first_num(value.begin(), value.end(), regex);
    if (first_num == std::sregex_iterator())
        value = "";

    value = first_num->str();
    is_negative = value[0] == '-';
}


static constexpr double log_2_10 = 3.32192809;



HugeInt::HugeInt(std::string value)
{
    bool is_negative;
    extract_FirstNumber(value, is_negative);

    size_t num_digits = value.size();
    if (is_negative && num_digits > 0)
        --num_digits;

    size = std::ceil(value.size() * (log_2_10 / (sizeof(BaseUint) * 8)));
    if (size > 1)
        data.ptr = new BaseUint[size]();
    else
    {
        size = 0; data.static_val = 0;
    }

    HugeInt power_of_10(size, 0), decimal_base(size, 0);
    power_of_10.set_BaseUint(0, 1);

    auto value_rend = value.rend() - int(is_negative);

    for (auto it = value.rbegin(); it != value_rend; ++it)
    {
        BaseUint digit = *it - '0';
        power_of_10.copy_and_shift_ContentsTo(decimal_base, 0);
        multiply_by_BaseUint(decimal_base, is_negative ? -digit : digit);
        sum(*this, decimal_base);
        multiply_by_BaseUint(power_of_10, 10);
    }
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
    HugeInt::BaseUint s = a + b + c;
    c = b >= ~c || (b + c) >= ~a;
    return s;
}


char HugeInt::sum(HugeInt &a, const HugeInt &b, char carry)
{
    for (size_t i = 0; i < a.get_TechnicalSize(); ++i)
        a.set_BaseUint(i, sum_BaseUints(a.get_BaseUint(i), b.get_BaseUint(i), carry));
    return carry;
}


void HugeInt::multiply(const HugeInt &a, const HugeInt &b, HugeInt &c, HugeInt &temp_var)
{
    size_t a_size = a.get_TechnicalSize();
    size_t b_size = b.get_TechnicalSize();

    for (size_t i = 0; i < b_size; ++i)
    {
        temp_var.clear();
        a.copy_and_shift_ContentsTo(temp_var, i);
        multiply_by_BaseUint(temp_var, b.get_BaseUint(i));
        sum(c, temp_var);
    }
}


void HugeInt::multiply_by_BaseUint(HugeInt &a, BaseUint b)
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
        HugeInt result(std::max(size, arg.size), 1); // 1 int more capacity for overloaded sum
        copy_and_shift_ContentsTo(result, 0);
        char carry = sum(result, arg, 0);

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

    multiply(*this, arg, result, product_by_int);

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


std::ostream& operator<<(std::ostream &os, const HugeInt &x)
{
    return os << x.to_String();
}
