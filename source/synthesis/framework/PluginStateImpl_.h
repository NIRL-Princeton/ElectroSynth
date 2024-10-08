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

template<typename T>
struct LEAFParams : public chowdsp::ParamHolder
{
    LEAFParams (T& _module) : module (_module)
    {

    }

    T& module;
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
        explicit PluginStateImpl_ (Module & _mod, juce::UndoManager* um = nullptr);

        /** Constructs the state and adds all the state parameters to the given processor */
        explicit PluginStateImpl_ (Module & _mod, juce::AudioProcessor& proc, juce::UndoManager* um = nullptr);

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
    PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::PluginStateImpl_ (Module & _mod, juce::UndoManager* um) : params(_mod)
    {
        initialise (params, nullptr, um);
    }

    template <typename ParameterState,typename Module, typename NonParameterState, typename Serializer>
    PluginStateImpl_<ParameterState, Module, NonParameterState, Serializer>::PluginStateImpl_ (Module & _mod, juce::AudioProcessor& proc, juce::UndoManager* um) : params(_mod)
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
