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

#include <iostream>
#include "module_type_info.h"
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

//reinterepret_cast is undefined behavior as soo nas you do anyhting with it.
//should change this to not use it
template <typename T>
struct LEAFParams : public chowdsp::ParamHolder
{
    LEAFParams ( LEAF* leaf) : chowdsp::ParamHolder(module_strings[map.get<T>()])
    {
        //reinterpret_cast<T> allows for type unsafe casting
        std::array<float,MAX_NUM_PARAMS> mutable_params = empty_params;
        int uuid = getNextUuid(leaf);
        for (int i = 0; i < MAX_NUM_VOICES; ++i) {
            leaf::module_init_map[map.get<T>()](reinterpret_cast<void**>(&modules[i]), mutable_params.data(), uuid, leaf);
            leaf::proc_init_map[map.get<T>()](reinterpret_cast<void*>(modules[i]), &processors[i]);
            for (int j = 0 ;j< MAX_NUM_PARAMS; j++) {
                all_params[j][i] = &processors[i].inParameters[j];
            }
        }
    }
    std::array<std::array<std::atomic<float>**,MAX_NUM_VOICES>,MAX_NUM_PARAMS> all_params;
    leaf::tProcessor processors[MAX_NUM_VOICES];
    T* modules[MAX_NUM_VOICES];
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
