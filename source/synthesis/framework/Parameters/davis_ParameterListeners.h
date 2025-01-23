////
//// Created by Davis Polito on 1/22/25.
////
//
//#ifndef ELECTORSYNTH_DAVIS_PARAMETERLISTENERS_H
//#define ELECTORSYNTH_DAVIS_PARAMETERLISTENERS_H
//#pragma once
//#include <juce_core/juce_core.h>
//#include <juce_events/juce_events.h>
//#include <Parameters/InternalAudioParameters.h>
//#if JUCE_MODULE_AVAILABLE_juce_dsp
//JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wimplicit-const-int-float-conversion")
//    #include <juce_dsp/juce_dsp.h>
//JUCE_END_IGNORE_WARNINGS_GCC_LIKE
//#else
//    #include "../../../common/chowdsp_core/JUCEHelpers/juce_FixedSizeFunction.h"
//#endif
//#include "Parameters/davis_ParamHolder.h"
//#include <chowdsp_listeners/chowdsp_listeners.h>
//#include <chowdsp_dsp_data_structures/chowdsp_dsp_data_structures.h>
//#include <chowdsp_data_structures/Helpers/chowdsp_Iterators.h>
//namespace davis
//{
//    /** Enum to specify which thread a parameter listener callback should happen on. */
//    enum class ParameterListenerThread
//    {
//        MessageThread,
//        AudioThread,
//    };
//
//    /** Utility class to manage a set of parameter listeners. */
//    class ParameterListeners : private juce::TimeSliceClient,
//                               private InternalParameter::Listener
//    {
//    public:
//        /** Initialises the listeners with a set of parameters. */
//        explicit ParameterListeners (ParamHolder& parameters,
//            const juce::TimeSliceThread& thread,
//            /*const juce::AudioProcessor* parentProcessor = nullptr,*/
//            int intervalMilliseconds = 20);
//        ~ParameterListeners() override;
//
//        /**
//     * Runs any queued listeners on the audio thread.
//     *
//     * If you're using chowdsp::PluginBase, this will get called automatically,
//         * dula
//     * otherwise, users must call this in the audio callback themselves!
//     */
//        void callAudioThreadBroadcasters();
//
//        /**
//     * If you are changing a bunch of parameters at once (e.g. preset loading),
//     * and want the MessageThread changes to propagate immediately, this function
//     * will do that for you!
//     */
//        void updateBroadcastersFromMessageThread();
//
//        /** Creates a new parameter listener. */
//        template <typename... ListenerArgs>
//        [[nodiscard]] chowdsp::ScopedCallback addParameterListener (const InternalNormalisableRangedParameter& param, ParameterListenerThread listenerThread, ListenerArgs&&... args)
//        {
//            const auto paramInfoIter = std::find_if (paramInfoList.begin(), paramInfoList.end(), [&param] (const ParamInfo& info)
//                { return info.paramCookie == &param; });
//
//            if (paramInfoIter == paramInfoList.end())
//            {
//                jassertfalse; // trying to listen to a parameter that is not part of this state!
//                return {};
//            }
//
//            const auto index = (size_t) std::distance (paramInfoList.begin(), paramInfoIter);
//            auto& broadcasterList = listenerThread == ParameterListenerThread::MessageThread ? messageThreadBroadcasters : audioThreadBroadcasters;
//            return broadcasterList[index].connect (std::forward<ListenerArgs...> (args...));
//        }
//
//    private:
//        void callMessageThreadBroadcaster (size_t index);
//        void callAudioThreadBroadcaster (size_t index);
//
////        void timerCallback() override;
//        int useTimeSlice() override;
//        void parameterValueChanged (int, float) override;
//        void parameterGestureChanged (int, bool) override {}
//
//        struct ParamInfo
//        {
//            InternalNormalisableRangedParameter* paramCookie = nullptr;
//            float value = 0.0f;
//        };
//
//        const size_t totalNumParams;
//        std::vector<ParamInfo> paramInfoList { totalNumParams };
//
//        std::vector<chowdsp::Broadcaster<void()>> messageThreadBroadcasters { totalNumParams };
//
//        static constexpr size_t actionSize = 16; // sizeof ([this, i = index] { callMessageThreadBroadcaster (i); })
//        std::vector<chowdsp::Broadcaster<void()>> audioThreadBroadcasters { totalNumParams };
//        using AudioThreadAction = juce::dsp::FixedSizeFunction<actionSize, void()>;
//        moodycamel::ReaderWriterQueue<AudioThreadAction> audioThreadBroadcastQueue { totalNumParams };
//        int defaultInterval;
//        const juce::TimeSliceThread& _parent;
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterListeners)
//    };
//} // namespace chowdsp
//
//#endif //ELECTORSYNTH_DAVIS_PARAMETERLISTENERS_H
