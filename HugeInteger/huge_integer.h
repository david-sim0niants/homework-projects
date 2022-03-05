#ifndef __HUGE_INTEGER__
#define __HUGE_INTEGER__


#include <string>


class HugeInt
{
public:
    using BaseInt = int;
    using BaseUint = unsigned int;

private:
    union Data
    {
        BaseInt *ptr;
        BaseInt static_val;
    } data;
    size_t size;

    HugeInt(size_t size, size_t additional_capacity);

    /**
     * @brief Get the int in the powers of 2 representation of huge int, which is multiplied by 2^(sizeof(BaseInt) * i)
     */
    BaseInt get_BaseInt(size_t i) const
    {
        if (size > 0)
        {
            if (i < size)
                return data.ptr[i];
            else 0;
        }
        else { return data.static_val; }
    }

public:
    HugeInt(BaseInt value);
    HugeInt(std::string value);


    HugeInt(const HugeInt &other);
    HugeInt(HugeInt &&other) noexcept;
    HugeInt& operator=(const HugeInt &rhs);
    HugeInt& operator=(HugeInt &&rhs) noexcept;
    ~HugeInt();


    HugeInt operator+(const HugeInt &arg) const;
    HugeInt operator*(const HugeInt &arg) const;
};


#endif