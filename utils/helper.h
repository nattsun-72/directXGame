#ifndef HELPER_H
#define HELPER_H

#include <random>
#include <type_traits>

namespace helper {

    inline int random_int(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> distribution(min, max);
        return distribution(gen);
    }

    template<typename EnumType>
    EnumType random_enum(EnumType min = EnumType(0), EnumType max = EnumType::End) {
        static_assert(std::is_enum<EnumType>::value, "Template parameter must be an enum type");
        
        using Underlying = std::underlying_type_t<EnumType>;
        Underlying minVal = static_cast<Underlying>(min);
        Underlying maxVal = static_cast<Underlying>(max) - 1;
        Underlying value = static_cast<Underlying>(random_int(minVal, maxVal));
        return static_cast<EnumType>(value);
    }

}
#endif // HELPER_H
