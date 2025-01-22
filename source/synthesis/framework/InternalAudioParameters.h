#pragma once
//ripping off juce audio parameters but we don't need to notifyHost ever since these will be internal to our synth
//no need for version hints as this is again internal code
class InternalAudioParameter
{
public:
    InternalAudioParameter() noexcept = default;

    /** Destructor. */
    virtual ~AudioProcessorParameter();

    /** Called by the UI and audio thread to find out the value of this parameter.

    expected to be 0.0 to 1.0
        This could be called quite frequently, so try to make your code efficient.
            It's also likely to be called by non-UI threads, so the code in here should
            be thread-aware.
   */
    virtual float getValue() const = 0;

    /** The host will call this method to change the value of a parameter.

                  The host may call this at any time, including during the audio processing
                                                          callback, so your implementation has to process this very efficiently and
                          avoid any kind of locking.

                      If you want to set the value of a parameter internally, e.g. from your
                          editor component, then don't call this directly - instead, use the
                      setValueNotifyingHost() method, which will also send a message to
                          the host telling it about the change. If the message isn't sent, the host
                          won't be able to automate your parameters properly.

                          The value passed will be between 0 and 1.0.
                                  */
    virtual void setValue (float newValue) = 0;
    /** Sends a signal to the host to tell it that the user is about to start changing this
        parameter.
        This allows the host to know when a parameter is actively being held by the user, and
        it may use this information to help it record automation.
        If you call this, it must be matched by a later call to endChangeGesture().
    */
    void beginChangeGesture();

    /** Tells the host that the user has finished changing this parameter.
        This allows the host to know when a parameter is actively being held by the user,
        and it may use this information to help it record automation.
        A call to this method must follow a call to beginChangeGesture().
    */
    void endChangeGesture();

    /** This should return the default value for this parameter. */
    virtual float getDefaultValue() const = 0;

    /** Returns the name to display for this parameter, which should be made
        to fit within the given string length.
    */
    virtual String getName (int maximumStringLength) const = 0;

    /** Some parameters may be able to return a label string for
        their units. For example "Hz" or "%".
    */
    virtual String getLabel() const = 0;

    /** Returns the number of steps that this parameter's range should be quantised into.

        If you want a continuous range of values, don't override this method, and allow
        the default implementation to return AudioProcessor::getDefaultNumParameterSteps().

        If your parameter is boolean, then you may want to make this return 2.

        The value that is returned may or may not be used, depending on the host. If you
        want the host to display stepped automation values, rather than a continuous
        interpolation between successive values, you should override isDiscrete to return true.

        @see isDiscrete
    */
    virtual int getNumSteps() const;

    /** Returns whether the parameter uses discrete values, based on the result of
        getNumSteps, or allows the host to select values continuously.

        This information may or may not be used, depending on the host. If you
        want the host to display stepped automation values, rather than a continuous
        interpolation between successive values, override this method to return true.

        @see getNumSteps
    */
    virtual bool isDiscrete() const;

    /** Returns whether the parameter represents a boolean switch, typically with
        "On" and "Off" states.

        This information may or may not be used, depending on the host. If you
        want the host to display a switch, rather than a two item dropdown menu,
        override this method to return true. You also need to override
        isDiscrete() to return `true` and getNumSteps() to return `2`.

        @see isDiscrete getNumSteps
    */
    virtual bool isBoolean() const;

    /** Returns a textual version of the supplied normalised parameter value.
        The default implementation just returns the floating point value
        as a string, but this could do anything you need for a custom type
        of value.
    */
    virtual String getText (float normalisedValue, int /*maximumStringLength*/) const;

    /** Should parse a string and return the appropriate value for it. */
    virtual float getValueForText (const String& text) const = 0;


    /** Returns the index of this parameter in its parent processor's parameter list. */
    int getParameterIndex() const noexcept { return parameterIndex; }

    //==============================================================================
    /** Returns the current value of the parameter as a String.

        This function can be called when you are hosting plug-ins to get a
        more specialised textual representation of the current value from the
        plug-in, for example "On" rather than "1.0".

        If you are implementing a plug-in then you should ignore this function
        and instead override getText.
    */
    virtual String getCurrentValueAsText() const;

    /** Returns the set of strings which represent the possible states a parameter
        can be in.

        If you are hosting a plug-in you can use the result of this function to
        populate a ComboBox listing the allowed values.

        If you are implementing a plug-in then you do not need to override this.
    */
    virtual StringArray getAllValueStrings() const;


    //==============================================================================
    /**
        A base class for listeners that want to know about changes to an
        AudioProcessorParameter.

        Use AudioProcessorParameter::addListener() to register your listener with
        an AudioProcessorParameter.

        This Listener replaces most of the functionality in the
        AudioProcessorListener class, which will be deprecated and removed.
    */
    class JUCE_API Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Receives a callback when a parameter has been changed.

            IMPORTANT NOTE: This will be called synchronously when a parameter changes, and
            many audio processors will change their parameter during their audio callback.
            This means that not only has your handler code got to be completely thread-safe,
            but it's also got to be VERY fast, and avoid blocking. If you need to handle
            this event on your message thread, use this callback to trigger an AsyncUpdater
            or ChangeBroadcaster which you can respond to on the message thread.
        */
        virtual void parameterValueChanged (int parameterIndex, float newValue) = 0;

        /** Indicates that a parameter change gesture has started.

            E.g. if the user is dragging a slider, this would be called with gestureIsStarting
            being true when they first press the mouse button, and it will be called again with
            gestureIsStarting being false when they release it.

            IMPORTANT NOTE: This will be called synchronously, and many audio processors will
            call it during their audio callback. This means that not only has your handler code
            got to be completely thread-safe, but it's also got to be VERY fast, and avoid
            blocking. If you need to handle this event on your message thread, use this callback
            to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
            message thread.
        */
        virtual void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) = 0;
    };

    /** Registers a listener to receive events when the parameter's state changes.
        If the listener is already registered, this will not register it again.

        @see removeListener
    */
    void addListener (Listener* newListener);

    /** Removes a previously registered parameter listener

        @see addListener
    */
    void removeListener (Listener* listener);

    //==============================================================================
    /** @internal */
    void sendValueChangedMessageToListeners (float newValue);


    /** Returns an ID that is unique to this parameter.

        Parameter indices are unstable across plugin versions, which means that the
        parameter found at a particular index in one version of a plugin might move
        to a different index in the subsequent version.

        Unlike the parameter index, the ID returned by this function should be
        somewhat stable (depending on the format of the plugin), so it is more
        suitable for storing/recalling automation data.
    */
    virtual String getParameterID() const = 0;

private:
    //==============================================================================
   // AudioProcessor* processor = nullptr;
    String parameterID;
    int parameterIndex = -1;
    //int version = 0;
    CriticalSection listenerLock;
    Array<Listener*> listeners;
    mutable StringArray valueStrings;

#if JUCE_DEBUG
    bool isPerformingGesture = false;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalAudioParameter)
};
/** Copies an object, sets one of the copy's members to the specified value, and then returns the copy. */
template <typename Object, typename OtherObject, typename Member, typename Other>
[[nodiscard]] Object withMember (Object copy, Member OtherObject::* member, Other&& value)
{
    copy.*member = std::forward<Other> (value);
    return copy;
}
/**
    An instance of this class may be passed to the constructor of an AudioProcessorParameterWithID
    to set optional characteristics of that parameter.

@tags{Audio}
    */
    class InternalAudioParameterWithIDAttributes
{
    using This =  InternalAudioParameterWithIDAttributes

public:

    /** An optional label for the parameter's value */
    [[nodiscard]] auto withLabel (String x)            const { return withMember (*this, &This::label,          std::move (x)); }


    /** An optional label for the parameter's value */
    [[nodiscard]] auto getLabel()                      const { return label; }



private:
    String label;
};



//==============================================================================
/**
    This abstract base class is used by some AudioProcessorParameter helper classes.

@see AudioParameterFloat, AudioParameterInt, AudioParameterBool, AudioParameterChoice

                                                                     @tags{Audio}
                                                                     */
                                                                     class JUCE_API  AudioProcessorParameterWithID  : public HostedAudioProcessorParameter
{
public:
    /** The creation of this object requires providing a name and ID which will be constant for its lifetime.

        Given that AudioProcessorParameterWithID is abstract, you'll probably call this constructor
        from a derived class constructor, e.g.
        @code
        MyParameterType (String paramID, String name, String label, bool automatable)
            : AudioProcessorParameterWithID (paramID, name, AudioProcessorParameterWithIDAttributes().withLabel (label)
                                                                                                     .withAutomatable (automatable))
        {
        }
        @endcode

        @param parameterID      Specifies the identifier, and optionally the parameter's version hint.
        @param parameterName    The user-facing parameter name.
        @param attributes       Other parameter properties.
    */
    AudioProcessorParameterWithID (const String& parameterID,
        const String& parameterName,
        const AudioProcessorParameterWithIDAttributes& attributes = {});


        /** Provides access to the parameter's ID string. */
        const String paramID;

        /** Provides access to the parameter's name. */
        const String name;

        /** Provides access to the parameter's label. */
        const String label;


        String getName (int) const override;
        String getLabel() const override;
        Category getCategory() const override;

        String getParameterID()             const override { return paramID; }

    private:

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalProcessorParameterWithID)
    };

    /**
    @internal

    Holds common attributes of audio parameters.

    CRTP is used here because we want the Attributes types for each parameter
    (Float, Bool, Choice, Int) to be distinct and extensible in the future.
    i.e. the identifiers AudioParameterFloatAttributes and RangedAudioParameterAttributes<float>
    should not be interchangable because we might need to add float-specific attributes in
    the future. Users should not refer directly to RangedAudioParameterAttributes.

    @tags{Audio}
*/
template <typename Derived, typename Value>
class RangedAudioParameterAttributes
    {
        using This = RangedAudioParameterAttributes;

    public:
        using Category = AudioProcessorParameter::Category;

        using StringFromValue = std::function<String (Value, int)>;
        using ValueFromString = std::function<Value (const String&)>;

        /** An optional lambda function that converts a non-normalised value to a string with a maximum length. This may be used by hosts to display the parameter's value. */
        [[nodiscard]] auto withStringFromValueFunction (StringFromValue x)                       const { return withMember (asDerived(), &Derived::stringFromValue, std::move (x)); }

        /** An optional lambda function that parses a string and converts it into a non-normalised value. Some hosts use this to allow users to type in parameter values. */
        [[nodiscard]] auto withValueFromStringFunction (ValueFromString x)                       const { return withMember (asDerived(), &Derived::valueFromString, std::move (x)); }

        /** See AudioProcessorParameterWithIDAttributes::withLabel() */
        [[nodiscard]] auto withLabel (String x)                                                  const { return withMember (asDerived(), &Derived::attributes, attributes.withLabel (std::move (x))); }

        /** See AudioProcessorParameterWithIDAttributes::withCategory() */
        [[nodiscard]] auto withCategory (Category x)                                             const { return withMember (asDerived(), &Derived::attributes, attributes.withCategory (std::move (x))); }

        /** See AudioProcessorParameter::isMetaParameter() */
        [[nodiscard]] auto withMeta (bool x)                                                     const { return withMember (asDerived(), &Derived::attributes, attributes.withMeta (std::move (x))); }

        /** See AudioProcessorParameter::isAutomatable() */
        [[nodiscard]] auto withAutomatable (bool x)                                              const { return withMember (asDerived(), &Derived::attributes, attributes.withAutomatable (std::move (x))); }

        /** See AudioProcessorParameter::isOrientationInverted() */
        [[nodiscard]] auto withInverted (bool x)                                                 const { return withMember (asDerived(), &Derived::attributes, attributes.withInverted (std::move (x))); }

        /** An optional lambda function that converts a non-normalised value to a string with a maximum length. This may be used by hosts to display the parameter's value. */
        [[nodiscard]] const auto& getStringFromValueFunction()                                   const { return stringFromValue; }

        /** An optional lambda function that parses a string and converts it into a non-normalised value. Some hosts use this to allow users to type in parameter values. */
        [[nodiscard]] const auto& getValueFromStringFunction()                                   const { return valueFromString; }

        /** Gets attributes that would also apply to an AudioProcessorParameterWithID */
        [[nodiscard]] const auto& getAudioProcessorParameterWithIDAttributes()                   const { return attributes; }

    private:
        auto& asDerived() const { return *static_cast<const Derived*> (this); }

        AudioProcessorParameterWithIDAttributes attributes;
        StringFromValue stringFromValue;
        ValueFromString valueFromString;
    };