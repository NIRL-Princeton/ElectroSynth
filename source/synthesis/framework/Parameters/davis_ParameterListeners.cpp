////
//// Created by Davis Polito on 1/22/25.
////
//#include "davis_ParameterListeners.h"
//
//namespace davis
//{
//    ParameterListeners::ParameterListeners (ParamHolder& parameters,
////        const juce::AudioProcessor* parentProcessor,
//        const juce::TimeSliceThread &thread,
//        int interval)
//        : totalNumParams ((size_t) parameters.count()), defaultInterval(interval),
//          _parent(thread)
//    {
//        parameters.doForAllParameters (
//            [this] (auto& param, size_t indexInParamHolder)
//            {
//                auto* rangedParam = static_cast<InternalNormalisableRangedParameter*> (&param);
//                const auto index = /*parentProcessor != nullptr ? static_cast<size_t> (rangedParam->getParameterIndex()) : */ indexInParamHolder;
//                paramInfoList[index] = ParamInfo { rangedParam, rangedParam->getValue() };
//
////                if (parentProcessor != nullptr)
////                    rangedParam->addListener (this);
//            });
//
//        //startTimer (interval);
//    }
//
//    ParameterListeners::~ParameterListeners()
//    {
//        for (auto& paramInfo : paramInfoList)
//            paramInfo.paramCookie->removeListener (this);
//    }
//
//    void ParameterListeners::parameterValueChanged (int paramIndex, float newValue)
//    {
//        if (! juce::MessageManager::getInstance()->isThisTheMessageThread())
//            return; // this will be handled by the timer callback!
//
//        auto index = static_cast<size_t> (paramIndex);
//        auto& paramInfo = paramInfoList[index];
//        paramInfo.value = newValue;
//        audioThreadBroadcastQueue.try_enqueue ([this, i = index]
//            { callAudioThreadBroadcaster (i); });
//        callMessageThreadBroadcaster (index);
//    }
//
//    int ParameterListeners::useTimeSlice()
//    {
//        // If the parameters are attached to a processor then most of the listeners
//        // callbacks will be handled there. The point of this timer is to handle
//        // parameter changes that might have come from the audio thread, or for
//        // parameters that may not be connected to a processor.
//        updateBroadcastersFromMessageThread();
//        return defaultInterval;
//    }
//
//    void ParameterListeners::updateBroadcastersFromMessageThread()
//    {
//        jassert (juce::MessageManager::existsAndIsCurrentThread());
//        for (const auto [index, paramInfo] : chowdsp::enumerate (paramInfoList))
//        {
//            if (juce::approximatelyEqual (paramInfo.paramCookie->getValue(), paramInfo.value))
//                continue;
//
//            paramInfo.value = paramInfo.paramCookie->getValue();
//
//            audioThreadBroadcastQueue.try_enqueue ([this, i = index]
//                { callAudioThreadBroadcaster (i); });
//            callMessageThreadBroadcaster (index);
//        }
//    }
//
//    void ParameterListeners::callAudioThreadBroadcasters()
//    {
//        AudioThreadAction action;
//        while (audioThreadBroadcastQueue.try_dequeue (action))
//            action();
//    }
//
//    void ParameterListeners::callMessageThreadBroadcaster (size_t index)
//    {
//        messageThreadBroadcasters[index]();
//    }
//
//    void ParameterListeners::callAudioThreadBroadcaster (size_t index)
//    {
//        audioThreadBroadcasters[index]();
//    }
//} // namespace chowdsp
