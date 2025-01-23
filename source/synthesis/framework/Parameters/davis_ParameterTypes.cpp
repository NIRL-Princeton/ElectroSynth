////
//// Created by Davis Polito on 1/22/25.
////
//#include "davis_ParameterTypes.h"
//namespace davis
//{
//    InternalFloatParameter::InternalFloatParameter (const std::string& parameterID,
//        const std::string& parameterName,
//        juce::NormalisableRange<float> normalisableRange,
//        float defaultValue,
//        std::atomic<float>* valuePtr,
//        const std::function<void (float)>& setterFunc,
//        const davis::InternalParameterFloatAttributes& attributes)
//        : InternalNormalisableRangedParameter (parameterID, parameterName, attributes.getInternalParameterWithIDAttributes()),
//          range (normalisableRange),
//          valueDefault (defaultValue),
//          stringFromValueFunction (attributes.getStringFromValueFunction()),
//          valueFromStringFunction (attributes.getValueFromStringFunction())
//
//    {
//        //allow it to either reference some other object i.e. for use in leaf or to internally manage its value
//        if (valuePtr != nullptr)
//        {
//            value = valuePtr;
//        }
//        else
//        {
//            value = &_value;
//        }
//        setFunc = std::move (setterFunc);
//        if (stringFromValueFunction == nullptr)
//        {
//            auto numDecimalPlacesToDisplay = [this] {
//                int numDecimalPlaces = 7;
//
//                if (!juce::approximatelyEqual (range.interval, 0.0f))
//                {
//                    if (juce::approximatelyEqual (std::abs (range.interval - std::floor (range.interval)), 0.0f))
//                        return 0;
//
//                    auto v = std::abs (juce::roundToInt (range.interval * pow (10, numDecimalPlaces)));
//
//                    while ((v % 10) == 0 && numDecimalPlaces > 0)
//                    {
//                        --numDecimalPlaces;
//                        v /= 10;
//                    }
//                }
//
//                return numDecimalPlaces;
//            }();
//
//            stringFromValueFunction = [numDecimalPlacesToDisplay] (float v, int length) {
//                std::string asText (v, numDecimalPlacesToDisplay);
//                return length > 0 ? asText.substr (0, length) : asText;
//            };
//        }
//
//        if (valueFromStringFunction == nullptr)
//            valueFromStringFunction = [] (const std::string& text) { return std::stof (text); };
//    }
//
//    InternalFloatParameter::InternalFloatParameter (const std::string& pid, const std::string& nm, float minValue, float maxValue, std::atomic<float>* valuePtr,const std::function<void (float)>& setterFunc, float def)
//        : InternalFloatParameter (pid, nm, { minValue, maxValue, 0.01f }, def, valuePtr,setterFunc)
//    {
//    }
//
//    InternalFloatParameter::~InternalFloatParameter()
//    {
//#if __cpp_lib_atomic_is_always_lock_free
//        static_assert (std::atomic<float>::is_always_lock_free,
//            "InternalFloatParameter requires a lock-free std::atomic<float>");
//#endif
//    }
//
//    float InternalFloatParameter::getValue() const { return convertTo0to1 (*value); }
//    void InternalFloatParameter::setValue (float newValue)
//    {
//        *value = convertFrom0to1 (newValue);
//        valueChanged (get());
//    }
//    float InternalFloatParameter::getDefaultValue() const { return convertTo0to1 (valueDefault); }
//    int InternalFloatParameter::getNumSteps() const { return InternalParameterWithID::getNumSteps(); }
//    std::string InternalFloatParameter::getText (float v, int length) const { return stringFromValueFunction (convertFrom0to1 (v), length); }
//    float InternalFloatParameter::getValueForText (const std::string& text) const { return convertTo0to1 (valueFromStringFunction (text)); }
//    void InternalFloatParameter::valueChanged (float) {}
//
//    InternalFloatParameter& InternalFloatParameter::operator= (float newValue)
//    {
//        if (!juce::approximatelyEqual ((float) *value, newValue))
//            setValue (newValue);
//        //setValueNotifyingHost (convertTo0to1 (newValue));
//
//        return *this;
//    }
//    void InternalFloatParameter::applyMonophonicModulation (double modulationValue)
//    {
//        modulationAmount = (float) modulationValue;
//    }
//
//    float InternalFloatParameter::getCurrentValue() const noexcept
//    {
//        return range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, range.convertTo0to1 (get()) + modulationAmount));
//    }
//
//
//
//    InternalParameterBool::~InternalParameterBool()
//    {
//#if __cpp_lib_atomic_is_always_lock_free
//        static_assert (std::atomic<float>::is_always_lock_free,
//            "InternalParameterBool requires a lock-free std::atomic<float>");
//#endif
//    }
//
//    float InternalParameterBool::getValue() const                               { return *value; }
//    void InternalParameterBool::setValue (float newValue)                       { *value = newValue; valueChanged (get()); }
//    float InternalParameterBool::getDefaultValue() const                        { return valueDefault; }
//    int InternalParameterBool::getNumSteps() const                              { return 2; }
//    bool InternalParameterBool::isDiscrete() const                              { return true; }
//    bool InternalParameterBool::isBoolean() const                               { return true; }
//    void InternalParameterBool::valueChanged (bool)                             {}
//
//    float InternalParameterBool::getValueForText (const std::string& text) const
//    {
//        return boolFromStringFunction (text) ? 1.0f : 0.0f;
//    }
//
//    std::string InternalParameterBool::getText (float v, int maximumLength) const
//    {
//        return stringFromBoolFunction (v >= 0.5f, maximumLength);
//    }
//
//    InternalParameterBool& InternalParameterBool::operator= (bool newValue)
//    {
//        if (get() != newValue)
//            setValueNotifyingHost (newValue ? 1.0f : 0.0f);
//
//        return *this;
//    }
//
//
//
//    InternalParameterChoice::InternalParameterChoice (const std::string& idToUse,
//        const std::string& nameToUse,
//        const juce::StringArray& c,
//        int def,
//        std::atomic<float>* valuePtr,
//        const std::function<void (float)>& setterFunc,
//        const InternalParameterChoiceAttributes& attributes)
//        : InternalNormalisableRangedParameter (idToUse, nameToUse, attributes.getInternalParameterWithIDAttributes()),
//          choices (c),
//          range ([this] {
//              juce::NormalisableRange<float> rangeWithInterval { 0.0f, (float) choices.size() - 1.0f, [] (float, float end, float v) { return juce::jlimit (0.0f, end, v * end); }, [] (float, float end, float v) { return juce::jlimit (0.0f, 1.0f, v / end); }, [] (float start, float end, float v) { return (float) juce::roundToInt (juce::jlimit (start, end, v)); } };
//              rangeWithInterval.interval = 1.0f;
//              return rangeWithInterval;
//          }()),
//          _value ((float) def),
//          defaultValue (convertTo0to1 ((float) def)),
//          stringFromIndexFunction (attributes.getStringFromValueFunction() != nullptr
//                                       ? attributes.getStringFromValueFunction()
//                                       : [this] (int index, int) { return choices[index].toStdString(); }),
//          indexFromStringFunction (attributes.getValueFromStringFunction() != nullptr
//                                       ? attributes.getValueFromStringFunction()
//                                       : [this] (const std::string& text) { return choices.indexOf (text); })
//    {
//        jassert (choices.size() > 1); // you must supply an actual set of items to choose from!
//        //allow it to either reference some other object i.e. for use in leaf or to internally manage its value
//        if (valuePtr != nullptr)
//        {
//            value = valuePtr;
//        }
//        else
//        {
//            value = &_value;
//        }
//    }
//
//    InternalParameterChoice::~InternalParameterChoice()
//    {
//#if __cpp_lib_atomic_is_always_lock_free
//        static_assert (std::atomic<float>::is_always_lock_free,
//            "InternalParameterChoice requires a lock-free std::atomic<float>");
//#endif
//    }
//
//    float InternalParameterChoice::getValue() const { return convertTo0to1 (*value); }
//    void InternalParameterChoice::setValue (float newValue)
//    {
//        *value = convertFrom0to1 (newValue);
//        valueChanged (getIndex());
//    }
//    float InternalParameterChoice::getDefaultValue() const { return defaultValue; }
//    int InternalParameterChoice::getNumSteps() const { return choices.size(); }
//    bool InternalParameterChoice::isDiscrete() const { return true; }
//    float InternalParameterChoice::getValueForText (const std::string& text) const { return convertTo0to1 ((float) indexFromStringFunction (text)); }
//    std::string InternalParameterChoice::getText (float v, int length) const { return stringFromIndexFunction ((int) convertFrom0to1 (v), length); }
//    void InternalParameterChoice::valueChanged (int) {}
//
//    InternalParameterChoice& InternalParameterChoice::operator= (int newValue)
//    {
//        if (getIndex() != newValue)
//            setValueNotifyingHost (convertTo0to1 ((float) newValue));
//
//        return *this;
//    }
//}