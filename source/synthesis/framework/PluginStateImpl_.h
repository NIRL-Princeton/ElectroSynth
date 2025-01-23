//
// Created by Davis Polito on 8/26/24.
//

#ifndef ELECTROSYNTH_PLUGINSTATEIMPL__H
#define ELECTROSYNTH_PLUGINSTATEIMPL__H

#pragma once
#include <juce_core/juce_core.h>

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <new>
#include <string_view>
#include <tuple>
#include <vector>
#include "processors/funcmaps.h"
#include <array>
#include <type_traits>
#include <iostream>
// Helper struct to store type-value pairs
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
};
const auto module_strings = std::to_array<std::string>({ "OscModule", "FiltModule", "EnvModule"});

constexpr std::array<float,MAX_NUM_PARAMS> createArray() {
    std::array<float, MAX_NUM_PARAMS> arr{};
    for (int i = 0; i < arr.size(); ++i) {
        arr[i] = 0.5;
    }
    return arr;
}
constexpr std::array< float,MAX_NUM_PARAMS> empty_params =createArray();
//constexpr std::array<float,MAX_NUM_PARAMS> createArray() {
//    std::array<float, MAX_NUM_PARAMS> arr{};
//    for (int i = 0; i < arr.size(); ++i) {
//        arr[i] = 0.5;
//    }
//    return arr;
//}
//constexpr std::array< float,MAX_NUM_PARAMS> empty_params =createArray();
template <typename T>
struct LEAFParams : public chowdsp::ParamHolder
{
    LEAFParams ( LEAF* leaf) : chowdsp::ParamHolder(module_strings[map.get<T>()])
    {
        //reinterpret_cast<T> allows for type unsafe casting
        std::array<float,MAX_NUM_PARAMS> mutable_params = empty_params;

        void** yeet = reinterpret_cast<void**>(&module);
        module_init_map[map.get<T>()](reinterpret_cast<void**>(&module), mutable_params.data(), getNextUuid(leaf) , leaf);
        proc_init_map[map.get<T>()](reinterpret_cast<void*>(module), &processor);
    }
    leaf::tProcessor processor;
    T* module;
};
    /**
 * Template type to hold a plugin's state.
 *
 * @tparam ParameterState       Struct containing all of the plugin's parameters as chowdsp::OptionalPointer's.
 * @tparam NonParameterState    Struct containing all of the plugin's non-parameter state as StateValue objects.
 * @tparam Serializer           A type that implements chowdsp::BaseSerializer (JSONSerializer by default)
 */
    template <typename ParameterState, typename Module,  typename NonParameterState = chowdsp::NonParamState, typename Serializer = chowdsp::XMLSerializer>
class PluginStateImpl_ : public chowdsp::PluginState
    {
        static_assert (std::is_base_of_v<LEAFParams<Module>, ParameterState>, "ParameterState must be a chowdsp::ParamHolder!");
        static_assert (std::is_base_of_v<chowdsp::NonParamState, NonParameterState>, "NonParameterState must be a chowdsp::NonParamState!");

    public:
        /** Constructs a plugin state with no processor */
        explicit PluginStateImpl_ (LEAF* leaf, juce::UndoManager* um = nullptr);

        /** Constructs the state and adds all the state parameters to the given processor */
        explicit PluginStateImpl_ ( LEAF* leaf, juce::AudioProcessor& proc, juce::UndoManager* um = nullptr);

        /** Serializes the plugin state to the given MemoryBlock */
        void serialize (juce::MemoryBlock& data) const override;

        /** Deserializes the plugin state from the given MemoryBlock */
        void deserialize (const juce::MemoryBlock& data) override;

        /** Serializer */
        template <typename>
        static typename Serializer::SerializedType serialize (const PluginStateImpl_& object);

        /** Deserializer */
        template <typename>
        static void deserialize (typename Serializer::DeserializedType serial, PluginStateImpl_& object);

        /** Returns the plugin non-parameter state */
        [[nodiscard]] chowdsp::NonParamState& getNonParameters() override;

        /** Returns the plugin non-parameter state */
        [[nodiscard]] const chowdsp::NonParamState& getNonParameters() const override;

        ParameterState params;
        NonParameterState nonParams;

    private:
        chowdsp::Version pluginStateVersion {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginStateImpl_)
    };

    template <typename ParameterState,typename Module, typename NonParameterState, typename Serializer>
    PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::PluginStateImpl_ (LEAF* leaf, juce::UndoManager* um) : params(leaf)
    {
        initialise (params, nullptr, um);
    }

    template <typename ParameterState,typename Module, typename NonParameterState, typename Serializer>
    PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::PluginStateImpl_ ( LEAF* leaf, juce::AudioProcessor& proc, juce::UndoManager* um) : params(leaf)
    {
        initialise (params, &proc, um);
    }

    template <typename ParameterState, typename Module,typename NonParameterState, typename Serializer>
    void PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::serialize (juce::MemoryBlock& data) const
    {
        chowdsp::Serialization::serialize<Serializer> (*this, data);
    }

    template <typename ParameterState, typename Module,typename NonParameterState, typename Serializer>
    void PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::deserialize (const juce::MemoryBlock& data)
    {
        callOnMainThread (
            [this, data]
            {
                chowdsp::Serialization::deserialize<Serializer> (data, *this);

                params.applyVersionStreaming (pluginStateVersion);
                if (nonParams.versionStreamingCallback != nullptr)
                    nonParams.versionStreamingCallback (pluginStateVersion);

                getParameterListeners().updateBroadcastersFromMessageThread();

                if (undoManager != nullptr)
                    undoManager->clearUndoHistory();
            });
    }

    /** Serializer */
    template <typename ParameterState,typename Module, typename NonParameterState, typename Serializer>
    template <typename>
    typename Serializer::SerializedType PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::serialize (const PluginStateImpl_& object)
    {
        auto serial = Serializer::template serialize<Serializer, chowdsp::ParamHolder> (object.params);
        return serial;
    }

    /** Deserializer */
    template <typename ParameterState, typename Module,typename NonParameterState, typename Serializer>
    template <typename>
    void PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::deserialize (typename Serializer::DeserializedType serial, PluginStateImpl_& object)
    {
        enum
        {
#if defined JucePlugin_VersionString
            versionChildIndex,
#endif
            nonParamStateChildIndex,
            paramStateChildIndex,
            expectedNumChildElements,
        };

        if (Serializer::getNumChildElements (serial) != expectedNumChildElements)
        {
            jassertfalse; // state load error!
            return;
        }

#if defined JucePlugin_VersionString
        Serializer::template deserialize<Serializer> (Serializer::getChildElement (serial, versionChildIndex), object.pluginStateVersion);
#else
        using namespace version_literals;
        object.pluginStateVersion = "0.0.0"_v;
#endif

        Serializer::template deserialize<Serializer, chowdsp::NonParamState> (Serializer::getChildElement (serial, nonParamStateChildIndex), object.nonParams);
        Serializer::template deserialize<Serializer, chowdsp::ParamHolder> (Serializer::getChildElement (serial, paramStateChildIndex), object.params);
    }

    template <typename ParameterState, typename Module,typename NonParameterState, typename Serializer>
    chowdsp::NonParamState& PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::getNonParameters()
    {
        return nonParams;
    }

    template <typename ParameterState, typename Module,typename NonParameterState, typename Serializer>
    const chowdsp::NonParamState& PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::getNonParameters() const
    {
        return nonParams;
    }
#endif //ELECTROSYNTH_PLUGINSTATEIMPL__H
