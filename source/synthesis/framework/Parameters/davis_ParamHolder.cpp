////
//// Created by Davis Polito on 1/22/25.
////
//#include "davis_ParamHolder.h"
//namespace davis
//{
//    inline ParamHolder::ParamHolder (const std::string& phName, bool phIsOwning)
//        : name (phName),
//          isOwning (phIsOwning)
//    {
//        juce::ignoreUnused (isOwning);
//    }
//
//    template <typename ParamType, typename... OtherParams>
//    std::enable_if_t<std::is_base_of_v<InternalFloatParameter, ParamType>, void>
//        ParamHolder::add (chowdsp::OptionalPointer<ParamType>& floatParam, OtherParams&... others)
//    {
//        auto& param = floatParams.emplace_back (isOwning ? floatParam.release() : floatParam.get(), isOwning);
//        //allParamsMap.insert ({ param->paramID.toStdString(), param.get() });
//        add (others...);
//    }
////
////    template <typename ParamType, typename... OtherParams>
////    std::enable_if_t<std::is_base_of_v<ChoiceParameter, ParamType>, void>
////        ParamHolder::add (OptionalPointer<ParamType>& choiceParam, OtherParams&... others)
////    {
////        auto& param = choiceParams.emplace_back (isOwning ? choiceParam.release() : choiceParam.get(), isOwning);
////        allParamsMap.insert ({ param->paramID.toStdString(), param.get() });
////        add (others...);
////    }
////
////    template <typename ParamType, typename... OtherParams>
////    std::enable_if_t<std::is_base_of_v<BoolParameter, ParamType>, void>
////        ParamHolder::add (OptionalPointer<ParamType>& boolParam, OtherParams&... others)
////    {
////        auto& param = boolParams.emplace_back (isOwning ? boolParam.release() : boolParam.get(), isOwning);
////        allParamsMap.insert ({ param->paramID.toStdString(), param.get() });
////        add (others...);
////    }
//
//    template <typename... OtherParams>
//    void ParamHolder::add (ParamHolder& paramHolder, OtherParams&... others)
//    {
//        otherParams.push_back (&paramHolder);
//        //allParamsMap.merge (paramHolder.allParamsMap);
//        //jassert (paramHolder.allParamsMap.empty()); // assuming no duplicate parameter IDs, all the parameters should be moved in the merge!
//        add (others...);
//    }
//
//    template <typename ParamContainerType, typename... OtherParams>
//    std::enable_if_t<chowdsp::TypeTraits::IsIterable<ParamContainerType>, void>
//        ParamHolder::add (ParamContainerType& paramContainer, OtherParams&... others)
//    {
//        for (auto& param : paramContainer)
//            add (param);
//        add (others...);
//    }
//
//    [[nodiscard]] inline int ParamHolder::count() const noexcept
//    {
//        auto numParams = int (floatParams.size() ); //;+ choiceParams.size() + boolParams.size());
//        for (const auto& holder : otherParams)
//            numParams += holder->count();
//        return numParams;
//    }
//
//    inline void ParamHolder::clear()
//    {
//        // It's generally not safe to clear the parameters if this is an owning ParamHolder
//        // since we're almost certainly leaving some dangling references lying around!
//        jassert (! isOwning);
//
//        //allParamsMap.clear();
//        floatParams.clear();
//        //choiceParams.clear();
//        //boolParams.clear();
//        otherParams.clear();
//    }
//
//    template <typename ParamContainersCallable, typename ParamHolderCallable>
//    void ParamHolder::doForAllParameterContainers (ParamContainersCallable&& paramContainersCallable, ParamHolderCallable&& paramHolderCallable)
//    {
//        paramContainersCallable (floatParams);
////        paramContainersCallable (choiceParams);
////        paramContainersCallable (boolParams);
//        for (auto& holder : otherParams)
//            paramHolderCallable (*holder);
//    }
//
//    template <typename ParamContainersCallable, typename ParamHolderCallable>
//    void ParamHolder::doForAllParameterContainers (ParamContainersCallable&& paramContainersCallable, ParamHolderCallable&& paramHolderCallable) const
//    {
//        paramContainersCallable (floatParams);
////        paramContainersCallable (choiceParams);
////        paramContainersCallable (boolParams);
//        for (const auto& holder : otherParams)
//            paramHolderCallable (*holder);
//    }
//
////    inline void ParamHolder::connectParametersToProcessor (juce::AudioProcessor& processor)
////    {
////        doForAllParameterContainers (
////            [&processor] (auto& paramVec)
////            {
////                for (auto& param : paramVec)
////                {
////                    // Parameter must be non-null and owned by its pointer before being released to the processor
////                    jassert (param != nullptr && param.isOwner());
////                    processor.addParameter (param.release());
////                }
////            },
////            [&processor] (auto& holder)
////            {
////                holder.connectParametersToProcessor (processor);
////            });
////    }
//
//    template <typename Callable>
//    size_t ParamHolder::doForAllParameters (Callable&& callable, size_t index)
//    {
//        doForAllParameterContainers (
//            [&index, call = std::forward<Callable> (callable)] (auto& paramVec)
//            {
//                for (auto& param : paramVec)
//                {
//                    call (*param, index);
//                    index++;
//                }
//            },
//            [&index, call = std::forward<Callable> (callable)] (auto& holder)
//            {
//                index = holder.doForAllParameters (std::move (call), index);
//            });
//
//        return index;
//    }
//
//    template <typename Callable>
//    size_t ParamHolder::doForAllParameters (Callable&& callable, size_t index) const
//    {
//        doForAllParameterContainers (
//            [&index, call = std::forward<Callable> (callable)] (const auto& paramVec)
//            {
//                for (const auto& param : paramVec)
//                {
//                    call (*param, index);
//                    index++;
//                }
//            },
//            [&index, call = std::forward<Callable> (callable)] (const auto& holder)
//            {
//                index = holder.doForAllParameters (std::move (call), index);
//            });
//
//        return index;
//    }
////
////    template <typename Serializer>
////    typename Serializer::SerializedType ParamHolder::serialize (const ParamHolder& paramHolder)
////    {
////        auto serial = Serializer::createBaseElement (paramHolder.name);
////        paramHolder.doForAllParameters (
////            [&serial] (auto& param, size_t)
////            {
////                chowdsp::ParameterTypeHelpers::serializeParameter<Serializer> (serial, param);
////            });
////        return serial;
////    }
//
////    template <typename Serializer>
////    void ParamHolder::deserialize (typename Serializer::DeserializedType deserial, ParamHolder& paramHolder)
////    {
////        std::stringArray paramIDsThatHaveBeenDeserialized {};
////        if (const auto numParamIDsAndVals = Serializer::getNumAttributes (deserial))
////        {
////            for (int i = 0; i < numParamIDsAndVals; i += 1)
////            {
////                std::string paramID {};
////                paramID = Serializer::getAttributeName (deserial, i);
////                //DBG("PArAMID" + paramID);
////                //Serialization::deserialize<Serializer> (, paramID);
////                //const auto paramDeserial = Serializer::getAttribute (deserial,paramID);
////
////                paramHolder.doForAllParameters (
////                    [&deserial,
////                        &paramID = std::as_const (paramID),
////                        &paramIDsThatHaveBeenDeserialized] (auto& param, size_t)
////                    {
////                        if (param.paramID == paramID)
////                        {
////                            ParameterTypeHelpers::deserializeParameter<Serializer> (deserial, param);
////                            paramIDsThatHaveBeenDeserialized.add (paramID);
////                        }
////                    });
////            }
////        }
////        else
////        {
////            // jassertfalse; // state loading error
////        }
////
////        for (auto id : paramIDsThatHaveBeenDeserialized)
////        {
////            DBG ("deserialzied " + id);
////        }
////        paramHolder.doForAllParameters (
////            [&paramIDsThatHaveBeenDeserialized] (auto& param, size_t)
////            {
////                if (! paramIDsThatHaveBeenDeserialized.contains (param.paramID))
////                    ParameterTypeHelpers::resetParameter (param);
////            });
////    }
//
//    inline void ParamHolder::applyVersionStreaming (const chowdsp::Version& version)
//    {
//        if (versionStreamingCallback != nullptr)
//            versionStreamingCallback (version);
//
//        doForAllParameterContainers (
//            [] (auto&) {},
//            [&version] (auto& holder)
//            {
//                holder.applyVersionStreaming (version);
//            });
//    }
//} // namespace chowdsp
