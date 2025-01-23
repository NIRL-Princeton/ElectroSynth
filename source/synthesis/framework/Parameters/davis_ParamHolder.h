////
//// Created by Davis Polito on 1/22/25.
////
//
//#ifndef ELECTORSYNTH_DAVIS_PARAMHOLDER_H
//#define ELECTORSYNTH_DAVIS_PARAMHOLDER_H
//#include "chowdsp_data_structures/Structures/chowdsp_OptionalPointer.h"
//#include "davis_ParameterTypes.h"
//#include <chowdsp_core/Types/chowdsp_TypeTraits.h>
//#include <chowdsp_version/chowdsp_version.h>
//#include <string>
//#include <type_traits>
//namespace davis
//{
//    /** A base class for storing parameters that can go into a plugin state. */
//    class ParamHolder
//    {
//    public:
//
//        /**
//     * Creates a ParamHolder. The user might want to name the ParamHolder,
//     * or make it "non-owning" so it doesn't take ownership of the parameter
//     * pointers.
//     */
//        explicit ParamHolder (const std::string& name = {}, bool isOwning = true);
//
//        ParamHolder (ParamHolder&&) noexcept = default;
//        ParamHolder& operator= (ParamHolder&&) noexcept = default;
//
//        /** Adds parameters to the ParamHolder. */
//        template <typename ParamType, typename... OtherParams>
//        std::enable_if_t<std::is_base_of_v<InternalFloatParameter, ParamType>, void>
//            add (chowdsp::OptionalPointer<ParamType>& floatParam, OtherParams&... others);
//
////        /** Adds parameters to the ParamHolder. */
////        template <typename ParamType, typename... OtherParams>
////        std::enable_if_t<std::is_base_of_v<ChoiceParameter, ParamType>, void>
////            add (OptionalPointer<ParamType>& choiceParam, OtherParams&... others);
////
////        /** Adds parameters to the ParamHolder. */
////        template <typename ParamType, typename... OtherParams>
////        std::enable_if_t<std::is_base_of_v<BoolParameter, ParamType>, void>
////            add (OptionalPointer<ParamType>& boolParam, OtherParams&... others);
//
//        /** Adds parameters to the ParamHolder. */
//        template <typename... OtherParams>
//        void add (ParamHolder& paramHolder, OtherParams&... others);
//
//        /** Adds parameters to the ParamHolder. */
//        template <typename ParamContainerType, typename... OtherParams>
//        std::enable_if_t<chowdsp::TypeTraits::IsIterable<ParamContainerType>, void>
//            add (ParamContainerType& paramContainer, OtherParams&... others);
//
//        /** Counts all the parameters held internally. */
//        [[nodiscard]] int count() const noexcept;
//
//        /** Clears any parameters currently in the ParamHolder. */
//        void clear();
//
////        /** Connects all the parameters to an AudioProcessor */
////        void connectParametersToProcessor (juce::AudioProcessor& processor);
//
//        /** Returns the paramHolder name */
//        [[nodiscard]] std::string getName() const noexcept { return name; }
//
//        /** Internal use only! */
//        template <typename ParamContainersCallable, typename ParamHolderCallable>
//        void doForAllParameterContainers (ParamContainersCallable&& paramContainersCallable, ParamHolderCallable&& paramHolderCallable);
//
//        /** Internal use only! */
//        template <typename ParamContainersCallable, typename ParamHolderCallable>
//        void doForAllParameterContainers (ParamContainersCallable&& paramContainersCallable, ParamHolderCallable&& paramHolderCallable) const;
//
//        /**
//     * Do some callable for all the stored parameters.
//     * Callable must have the signature void(ParameterType&, size_t).
//     */
//        template <typename Callable>
//        size_t doForAllParameters (Callable&& callable, size_t index = 0);
//
//        /**
//     * Do some callable for all the stored parameters.
//     * Callable must have the signature void(ParameterType&, size_t).
//     */
//        template <typename Callable>
//        size_t doForAllParameters (Callable&& callable, size_t index = 0) const;
//
//        /** Custom serializer */
//        template <typename Serializer>
//        static typename Serializer::SerializedType serialize (const ParamHolder& paramHolder);
//
//        /** Custom deserializer */
//        template <typename Serializer>
//        static void deserialize (typename Serializer::DeserializedType deserial, ParamHolder& paramHolder);
//
//        /** Recursively applies version streaming to the parameters herein. */
//        void applyVersionStreaming (const chowdsp::Version&);
//
//        /** Assign this function to apply version streaming to your non-parameter state. */
//        std::function<void (const chowdsp::Version&)> versionStreamingCallback = nullptr;
//
//    private:
//        void add() const
//        {
//            // base case!
//        }
//
//        std::vector<chowdsp::OptionalPointer<FloatParameter>> floatParams;
////        std::vector<OptionalPointer<ChoiceParameter>> choiceParams;
////        std::vector<OptionalPointer<BoolParameter>> boolParams;
//        std::vector<ParamHolder*> otherParams;
//
//        using ParamPtrVariant = std::variant<InternalFloatParameter*>
//        std::unordered_map<std::string, ParamPtrVariant> allParamsMap {};
//
//        std::string name;
//        bool isOwning;
//
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamHolder)
//    };
//}
//#endif //ELECTORSYNTH_DAVIS_PARAMHOLDER_H
