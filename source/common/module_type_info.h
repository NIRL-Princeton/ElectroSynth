//
// Created by Davis Polito on 1/27/25.
//

#ifndef ELECTORSYNTH_MODULE_TYPE_INFO_H
#define ELECTORSYNTH_MODULE_TYPE_INFO_H
// Helper struct to store type-value pairs
#include <type_traits>
#include <string>
#include <array>
#include "processors/funcmaps.h"
template <typename T, int Value>
struct TypeValuePair {
    using type = T;
    static constexpr int value = Value;
};




// Compile-time map implementation
template <typename... Pairs>
struct TypeMap {
    // Constructor to initialize the TypeMap
    constexpr TypeMap(Pairs...) {}

    // Function to get value for type T
    template <typename T>
    static constexpr int get() {
        return get_impl<T, Pairs...>();
    }

private:
    // Recursively check the pairs and return the value if the type matches
    template <typename T, typename Pair, typename... Rest>
    static constexpr int get_impl() {
        if constexpr (std::is_same_v<T, typename Pair::type>) {
            return Pair::value;
        } else {
            return get_impl<T, Rest...>();
        }
    }

    // Base case: type not found
    template <typename T>
    static constexpr int get_impl() {
        static_assert(sizeof...(Pairs) > 0, "Type not found in map");
        return -1; // This line is never reached
    }
};

// Define your map using initializer syntax
constexpr TypeMap map{
    TypeValuePair<_tOscModule, 0>{},
    TypeValuePair<_tFiltModule, 1>{},
    TypeValuePair<_tEnvModule, 2>{},
    TypeValuePair<_tLFOModule, 3>{},
    TypeValuePair<_tStringModule, 4>{},
};
const std::array<std::string, 5> module_strings = { "osc", "filt", "env", "lfo","string"};


#endif //ELECTORSYNTH_MODULE_TYPE_INFO_H
