//#include "InternalAudioParameters.h"
//#include "chowdsp_data_structures/Structures/chowdsp_OptionalPointer.h"
//namespace davis
//{
//    namespace ParamUtils
//    {
//        /** Mixin for parameters that recognize some form of modulation. */
//        struct ModParameterMixin
//        {
//            virtual ~ModParameterMixin() = default;
//
//            /** Returns true if this parameter supports monophonic modulation. */
//            virtual bool supportsMonophonicModulation() { return false; }
//
//            /** Returns true if this parameter supports polyphonic modulation. */
//            virtual bool supportsPolyphonicModulation() { return false; }
//
//            /** Base function for applying monophonic modulation to a parameter. */
//            [[maybe_unused]] virtual void applyMonophonicModulation (double /*value*/)
//            {
//            }
//
//            /** Base function for applying polyphonic modulation to a parameter. */
//            [[maybe_unused]] virtual void applyPolyphonicModulation (int32_t /*note_id*/, int16_t /*port_index*/, int16_t /*channel*/, int16_t /*key*/, double /*value*/)
//            {
//            }
//        };
//    } // namespace ParamUtils
//    /** Properties of an InternalParameterFloat.
//
//@see InternalParameterFloat(), RangedInternalParameterAttributes()
//
//                                    @tags{Audio}
//                                */
//    class InternalParameterFloatAttributes : public InternalRangedParameterAttributes<InternalParameterFloatAttributes, float>
//    {
//    };
//
//    class InternalFloatParameter : public InternalNormalisableRangedParameter,
//                                   public ParamUtils::ModParameterMixin
//    {
//    public:
//        /** Creates a InternalParameterFloat with the specified parameters.
//
//    Note that the attributes argument is optional and only needs to be
//        supplied if you want to change options from their default values.
//
//        Example usage:
//        @code
//        auto attributes = InternalParameterFloatAttributes().withStringFromValueFunction ([] (auto x, auto) { return String (x * 100); })
//                              .withLabel ("%");
//    auto param = std::make_unique<InternalParameterFloat> ("paramID", "Parameter Name", NormalisableRange<float>(), 0.5f, attributes);
//    @endcode
//
//        @param parameterID         The parameter ID to use
//        @param parameterName       The parameter name to use
//        @param normalisableRange   The NormalisableRange to use
//        @param defaultValue        The non-normalised default value
//              @param attributes          Optional characteristics
//                  */
//        InternalFloatParameter (const std::string& parameterID,
//            const std::string& parameterName,
//            juce::NormalisableRange<float> normalisableRange,
//            float defaultValue,
//            std::atomic<float>* valuePtr = nullptr,
//            const std::function<void (float)>& setterFunc = nullptr,
//            const InternalParameterFloatAttributes& attributes = {});
//
//        /** Creates a InternalParameterFloat with an ID, name, and range.
//        On creation, its value is set to the default value.
//        For control over skew factors, you can use the other
//        constructor and provide a NormalisableRange.
//    */
//        InternalFloatParameter (const std::string& parameterID,
//            const std::string& parameterName,
//            float minValue,
//            float maxValue,
//            std::atomic<float>* valuePtr,
//            const std::function<void (float)>& setterFunc,
//            float defaultValue);
//        /** Destructor. */
//        ~InternalFloatParameter() override;
//
//        /** Returns the parameter's current value. */
//        float get() const noexcept { return *value; }
//
//        /** Changes the parameter's current value. */
//        InternalFloatParameter& operator= (float newValue);
//
//        /** Returns the range of values that the parameter can take. */
//        const juce::NormalisableRange<float>& getNormalisableRange() const override { return range; }
//
//        /** Provides access to the parameter's range. */
//        juce::NormalisableRange<float> range;
//        /** TRUE! */
//        bool supportsMonophonicModulation() override { return true; }
//
//        /** Applies monphonic modulation to this parameter. */
//        void applyMonophonicModulation (double value) override;
//
//        /** Returns the current parameter value accounting for any modulation that is currently applied. */
//        float getCurrentValue() const noexcept;
//
//        /** Returns the current parameter value accounting for any modulation that is currently applied. */
//        operator float() const noexcept { return getCurrentValue(); } // NOSONAR, NOLINT(google-explicit-constructor): we want to be able to do implicit conversion here
//
//        /** Print debug info. */
//        void printDebug() const
//        {
//            DBG (juce::String (paramID) + " : " + juce::String (get()));
//        }
//        std::function<void (float)> setFunc;
//    protected:
//        /** Override this method if you are interested in receiving callbacks
//        when the parameter value changes.
//    */
//        virtual void valueChanged (float newValue);
//
//    private:
//        //==============================================================================
//        float getValue() const override;
//        void setValue (float newValue) override;
//        float getDefaultValue() const override;
//        int getNumSteps() const override;
//        std::string getText (float, int) const override;
//        float getValueForText (const std::string&) const override;
//        float modulationAmount = 0.0f;
//        std::atomic<float>* value;
//        std::atomic<float> _value;
//        const float valueDefault;
//        std::function<std::string (float, int)> stringFromValueFunction;
//        std::function<float (const std::string&)> valueFromStringFunction;
//    };
//
//    /*
//  ==============================================================================
//
//This file is part of the JUCE framework.
//    Copyright (c) Raw Material Software Limited
//
//    JUCE is an open source framework subject to commercial or open source
//       licensing.
//
//       By downloading, installing, or using the JUCE framework, or combining the
//                                                 JUCE framework with any other source code, object code, content or any other
//                     copyrightable work, you agree to the terms of the JUCE End User Licence
//    Agreement, and all incorporated terms including the JUCE Privacy Policy and
//    the JUCE Website Terms of Service, as applicable, which will bind you. If you
//                   do not agree to the terms of these agreements, we will not license the JUCE
//    framework to you, and you must discontinue the installation or download
//                                                 process and cease use of the JUCE framework.
//
//                                                 JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
//                                                                                           JUCE Privacy Policy: https://juce.com/juce-privacy-policy
//                                                                                                                         JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/
//
//                                                                                                                                                                 Or:
//
//    You may also use this code under the terms of the AGPLv3:
//    https://www.gnu.org/licenses/agpl-3.0.en.html
//
//            THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
//    WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
//    MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.
//
//        ==============================================================================
//        */
//
//    /** Properties of an InternalParameterBool.
//
//    @see InternalParameterBool(), RangedInternalParameterAttributes()
//
//    @tags{Audio}
//*/
//    class InternalParameterBoolAttributes : public InternalRangedParameterAttributes<InternalParameterBoolAttributes, bool>
//    {
//    }; //==============================================================================
//    /**
//    Provides a class of AudioProcessorParameter that can be used as a boolean value.
//
//    @see InternalParameterFloat, InternalParameterInt, InternalParameterChoice
//
//    @tags{Audio}
//*/
//    class JUCE_API InternalParameterBool : public InternalNormalisableRangedParameter
//    {
//    public:
//        /** Creates a InternalParameterBool with the specified parameters.
//
//        Note that the attributes argument is optional and only needs to be
//        supplied if you want to change options from their default values.
//
//        Example usage:
//        @code
//        auto attributes = InternalParameterBoolAttributes().withStringFromValueFunction ([] (auto x, auto) { return x ? "On" : "Off"; })
//                                                        .withLabel ("enabled");
//        auto param = std::make_unique<InternalParameterBool> ("paramID", "Parameter Name", false, attributes);
//        @endcode
//
//        @param parameterID         The parameter ID to use
//        @param parameterName       The parameter name to use
//        @param defaultValue        The default value
//        @param attributes          Optional characteristics
//    */
//        InternalParameterBool (const std::string& parameterID,
//            const std::string& parameterName,
//            bool defaultValue,
//            std::atomic<float>* valuePtr,
//            const std::function<void (float)>& setterFunc,
//            const InternalParameterBoolAttributes& attributes = {})
//            : InternalNormalisableRangedParameter (parameterID, parameterName, attributes.getInternalParameterWithIDAttributes()),
//              _value (defaultValue ? 1.0f : 0.0f),
//              valueDefault (defaultValue),
//              stringFromBoolFunction (attributes.getStringFromValueFunction() != nullptr
//                                          ? attributes.getStringFromValueFunction()
//                                          : [] (bool v, int) { return v ? TRANS ("On").toStdString() : TRANS ("Off").toStdString(); }),
//              boolFromStringFunction (attributes.getValueFromStringFunction() != nullptr
//                                          ? attributes.getValueFromStringFunction()
//                                          : [] (const std::string& text) {
//                                                static const juce::StringArray onStrings { TRANS ("on"), TRANS ("yes"), TRANS ("true") };
//                                                static const juce::StringArray offStrings { TRANS ("off"), TRANS ("no"), TRANS ("false") };
//
//                                                //                                              //juce::String lowercaseText (text.toLowerCase());
//                                                //
//                                                //                                              for (auto& testText : onStrings)
//                                                //                                                  if (lowercaseText == testText)
//                                                //                                                      return true;
//                                                //
//                                                //                                              for (auto& testText : offStrings)
//                                                //                                                  if (lowercaseText == testText)
//                                                //                                                      return false;
//
//                                                return true;
//                                            })
//        {
//            //allow it to either reference some other object i.e. for use in leaf or to internally manage its value
//            if(valuePtr != nullptr)
//            {
//                value = valuePtr;
//            } else
//            {
//                value = &_value;
//            }
//            setFunc = std::move (setterFunc);
//        }
//
//        /** Destructor. */
//        ~InternalParameterBool() override;
//
//        /** Returns the parameter's current boolean value. */
//        bool get() const noexcept { return *value >= 0.5f; }
//
//        /** Returns the parameter's current boolean value. */
//        operator bool() const noexcept { return get(); }
//
//        /** Changes the parameter's current value to a new boolean. */
//        InternalParameterBool& operator= (bool newValue);
//
//        /** Returns the range of values that the parameter can take. */
//        const juce::NormalisableRange<float>& getNormalisableRange() const override { return range; }
//        void printDebug() const
//        {
//            DBG (juce::String (paramID) + " : " + juce::String (static_cast<int> (get())));
//        }
//        using Ptr = chowdsp::OptionalPointer<InternalParameterBool>;
//        std::function<void (float)> setFunc;
//
//    protected:
//        /** Override this method if you are interested in receiving callbacks
//        when the parameter value changes.
//    */
//        virtual void valueChanged (bool newValue);
//
//    private:
//        //==============================================================================
//        float getValue() const override;
//        void setValue (float newValue) override;
//        float getDefaultValue() const override;
//        int getNumSteps() const override;
//        bool isDiscrete() const override;
//        bool isBoolean() const override;
//        std::string getText (float, int) const override;
//        float getValueForText (const std::string&) const override;
//
//        const juce::NormalisableRange<float> range { 0.0f, 1.0f, 1.0f };
//        std::atomic<float>* value;
//        std::atomic<float> _value;
//        const float valueDefault;
//        std::function<std::string (bool, int)> stringFromBoolFunction;
//        std::function<bool (const std::string&)> boolFromStringFunction;
//
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalParameterBool)
//    };
//
//    /** Properties of an AudioParameterChoice.
//
//    @see AudioParameterChoice(), RangedAudioParameterAttributes()
//
//                                         @tags{Audio}
//                                     */
//    class InternalParameterChoiceAttributes : public InternalRangedParameterAttributes<InternalParameterChoiceAttributes, int>
//    {
//    };
//
//    //==============================================================================
//    /**
//    Provides a class of AudioProcessorParameter that can be used to select
//    an indexed, named choice from a list.
//
//    @see AudioParameterFloat, AudioParameterInt, AudioParameterBool
//
//    @tags{Audio}
//*/
//    class  InternalParameterChoice : public InternalNormalisableRangedParameter
//    {
//    public:
//        /** Creates a AudioParameterChoice with the specified parameters.
//
//        Note that the attributes argument is optional and only needs to be
//        supplied if you want to change options from their default values.
//
//        Example usage:
//        @code
//        auto attributes = AudioParameterChoiceAttributes().withLabel ("selected");
//        auto param = std::make_unique<AudioParameterChoice> ("paramID", "Parameter Name", StringArray { "a", "b", "c" }, 0, attributes);
//        @endcode
//
//        @param parameterID         The parameter ID to use
//        @param parameterName       The parameter name to use
//        @param choices             The set of choices to use
//        @param defaultItemIndex    The index of the default choice
//        @param attributes          Optional characteristics
//    */
//        InternalParameterChoice (const std::string& parameterID,
//            const std::string& parameterName,
//            const juce::StringArray& choices,
//            int defaultItemIndex,
//            std::atomic<float>* valuePtr,
//            const std::function<void (float)>& setterFunc,
//            const InternalParameterChoiceAttributes& attributes = {});
//             /** Creates a AudioParameterChoice with the specified parameters.
//
//
//        /** Destructor. */
//        ~InternalParameterChoice() override;
//
//        /** Returns the current index of the selected item. */
//        int getIndex() const noexcept { return juce::roundToInt ((*value).load()); }
//
//        /** Returns the current index of the selected item. */
//        operator int() const noexcept { return getIndex(); }
//
//        /** Returns the name of the currently selected item. */
//        std::string getCurrentChoiceName() const noexcept { return choices[getIndex()].toStdString(); }
//
//        /** Returns the name of the currently selected item. */
//        operator juce::String() const noexcept { return getCurrentChoiceName(); }
//
//        /** Changes the selected item to a new index. */
//        InternalParameterChoice& operator= (int newValue);
//
//        /** Returns the range of values that the parameter can take. */
//        const juce::NormalisableRange<float>& getNormalisableRange() const override { return range; }
//
//        /** Provides access to the list of choices that this parameter is working with. */
//        const juce::StringArray choices;
//
//    protected:
//        /** Override this method if you are interested in receiving callbacks
//        when the parameter value changes.
//    */
//        virtual void valueChanged (int newValue);
//
//    private:
//        //==============================================================================
//        float getValue() const override;
//        void setValue (float newValue) override;
//        float getDefaultValue() const override;
//        int getNumSteps() const override;
//        bool isDiscrete() const override;
//        std::string getText (float, int) const override;
//        float getValueForText (const std::string&) const override;
//
//        const juce::NormalisableRange<float> range;
//
//        std::atomic<float>* value;
//        std::atomic<float> _value;
//        const float defaultValue;
//        std::function<std::string (int, int)> stringFromIndexFunction;
//        std::function<int (const std::string&)> indexFromStringFunction;
//
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalParameterChoice)
//    };
//
//}; //namespace
