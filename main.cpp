#include <algorithm>
#include <cstdint>
#include <iostream>
#include <array>
#include <type_traits>

static void copyBit(uint8_t& dest, size_t destPosition, uint8_t src, size_t srcPosition)
{
    if ((src & (1u << srcPosition)) != 0) {
        // bit set in src, set in dest
        dest |= (1u << destPosition);
    } else {
        // bit not set in src, unset in dest
        dest &= ~(1u << destPosition);
    }
}

/**
 * @brief Order of bits in bytes.
 * 
 * @warning This is not Endianess!
 * 
 */
enum class BitOrder {
    /**
     * @brief The least significant bit is masked by (1u << 0).
     * 
     * @details This implies MSB is masked by (1u << 7).
     * This is the normal machine bitorder on all machines.
     * 
     */
    LSBAtZero,
    /**
     * @brief The most significant bit is masked by (1u << 0).
     * 
     * @details This implies LSB is masked by (1u << 7).
     * This is not the normal machine bitorder. For the machine
     * to be able to work with this, the bits have to be swapped.
     * 
     */
    MSBAtZero
};

/**
 * @brief Template class to hold information about a single bitfield.
 * 
 * @warning Can only be used with fundamental types. The last enable_if_t implements
 * a protection against using anything else with this functions.
 * When tried with another type, will result in compiler error similar to
 * "no type named ‘type’".
 * 
 * @tparam T type of the value in the bitfield. Must be fundamental.
 * @tparam position 0-indexed position in bits
 * @tparam size size of the bitfield in bits
 * @tparam MSBAtZero Bitorder. Most significant bit at zero index when using << operator.
 * @tparam littleEndian whether the incomming field is using little endian.
 */
template <class T, size_t position, size_t size, BitOrder bitOrder= BitOrder::LSBAtZero, bool littleEndian=false, typename std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
struct Bitfield {};

/**
 * @brief specialization for MSB at Bit 7.
 * 
 * @details Same bitorder as processor, can copy chunks instead of single bits.
 * 
 * @tparam T 
 * @tparam position 
 * @tparam size 
 */
template <class T, size_t position, size_t size, bool littleEndian>
struct Bitfield<T, position, size, BitOrder::LSBAtZero, littleEndian> {
    static T get(uint8_t data) {
        static_assert(position + size <= 8, "out of boundries");
        T temp{};
        auto ref = reinterpret_cast<uint8_t*>(&temp);
        ref[0] = (data >> position) & (0xFF >> (8-size));
        return temp;
    }

    static void set(uint8_t& data, const T& value) {
        static_assert(position + size <= 8, "out of boundries");
        auto ref = reinterpret_cast<const uint8_t*>(&value);
        // mask out old value in target field
        data &= ~((0xFF >> (8-size)) << position);
        // copy new value in
        data |= ((ref[0]) & (0xFF >> (8-size))) << position;
    }
};

/**
 * @brief specialization for MSB at Bit 0.
 * 
 * @details The bits will have to be copied in reverse order
 * and on eby one in this case.
 * 
 * @tparam T 
 * @tparam position 
 * @tparam size 
 */
template <class T, size_t position, size_t size, bool littleEndian>
struct Bitfield<T, position, size, BitOrder::MSBAtZero, littleEndian> {
public:
    static T get(uint8_t data) {
        static_assert(position + size <= 8, "out of boundries");
        T temp{};
        auto ref = reinterpret_cast<uint8_t*>(&temp);
        for (auto posSrc = position; posSrc < position + size; ++posSrc) {
            // copy in reverse order
            auto posDes = position+size-1 - posSrc;
            copyBit(ref[0], posDes, data, posSrc);
        }
        return temp;
    }

    static void set(uint8_t& data, const T& value) {
        static_assert(position + size <= 8, "out of boundries");
        auto ref = reinterpret_cast<const uint8_t*>(&value);
        for (auto posSrc = 0; posSrc < size; ++posSrc) {
            // copy in reverse order
            auto posDes = position+size-1 - posSrc;
            copyBit(data, posDes, ref[0], posSrc);
        }
    }
};

class A {};

int main(int argc, char* argv[]) {
    uint8_t x = 0b10100011;
    using bit0 = Bitfield<int, 0, 1>;
    using bit1 = Bitfield<int, 1, 1>;
    using bit2 = Bitfield<int, 2, 1>;
    using bit3 = Bitfield<int, 3, 1>;
    using bit4 = Bitfield<int, 4, 1>;
    using bit5 = Bitfield<int, 5, 1>;
    using bit6 = Bitfield<int, 6, 1>;
    using bit7 = Bitfield<int, 7, 1>;

    using Data = Bitfield<int, 0, 4, BitOrder::MSBAtZero>;

    Data::set(x, 5);

    std::cout << "bits: {" << bit7::get(x)
        << bit6::get(x)
        << bit5::get(x)
        << bit4::get(x)
        << bit3::get(x)
        << bit2::get(x)
        << bit1::get(x)
        << bit0::get(x) << "}" << std::endl;

    std::cout << "Data: " << Data::get(x) << std::endl;
    std::cout << "random inplace mask " << Bitfield<int, 4, 4, BitOrder::MSBAtZero>::get(x) << std::endl;
    return 0;
}