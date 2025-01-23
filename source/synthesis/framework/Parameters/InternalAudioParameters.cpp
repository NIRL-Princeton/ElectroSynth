//#include "InternalAudioParameters.h"
//void InternalParameter::addListener (InternalParameter::Listener* newListener)
//{
//    const juce::ScopedLock sl (listenerLock);
//    ]
//    //listeners.addIfNotAlreadyThere (newListener);
//}
//
//void InternalParameter::removeListener (InternalParameter::Listener* listenerToRemove)
//{
//    const juce::ScopedLock sl (listenerLock);
//    //listeners.removeFirstMatchingValue (listenerToRemove);
//}
//InternalParameterWithID::InternalParameterWithID (const std::string& idToUse,
//    const std::string& nameToUse,
//    const InternalParameterWithIDAttributes& attributes)
//    :
//      paramID (idToUse),
//      name (nameToUse),
//      label (attributes.getLabel())
//{
//}
//void InternalParameter::sendValueChangedMessageToListeners (float newValue)
//{
//    juce::ScopedLock lock (listenerLock);
//
////    for (int i = listeners.size(); --i >= 0;)
////        if (auto* l = listeners [i])
////            l->parameterValueChanged (juce::getParameterIndex(), newValue);
//
//
//}
//void InternalParameter::setValueNotifyingHost (float newValue)
//{
//    setValue (newValue);
//    sendValueChangedMessageToListeners (newValue);
//}
//
//
//std::string InternalParameterWithID::getName (int maximumStringLength) const        { return name.substr(0, maximumStringLength); }
//std::string InternalParameterWithID::getLabel() const                               { return label; }
//
//template <typename T>
//int InternalRangedParameter<T>::getNumSteps() const
//{
//    const auto& range = getNormalisableRange();
//
////    if (range.interval > 0)
//        return (static_cast<int> ((range.end - range.start) / range.interval) + 1);
//
//
//    //return AudioProcessor::getDefaultNumParameterSteps();
//}
//template <typename T>
//float InternalRangedParameter<T>::convertTo0to1 (float v) const noexcept
//
//{
//    const auto& range = getNormalisableRange();
//    return range.convertTo0to1 (range.snapToLegalValue (v));
//}
//template <typename T>
//float InternalRangedParameter<T>::convertFrom0to1 (float v) const noexcept
//{
//    const auto& range = getNormalisableRange();
//    return range.snapToLegalValue (range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, v)));
//}
//
//
