//
// Created by Davis Polito on 1/22/25.
//

#ifndef ELECTORSYNTH_DAVIS_PARAMTYPEHELPERS_H
#define ELECTORSYNTH_DAVIS_PARAMTYPEHELPERS_H
#pragma once
#include "davis_ParameterTypes.h"
#include <type_traits>
namespace davis
{
#ifndef DOXYGEN
    /** This API is unstable and should not be used directly! */
    namespace ParameterTypeHelpers
    {
        template <typename T, typename = void>
        struct ParameterElementTypeImpl;

        template <typename T>
        struct ParameterElementTypeImpl<T, typename std::enable_if_t<std::is_base_of_v<InternalFloatParameter, T>>>
            : std::true_type
        {
            using base_type = InternalFloatParameter;
            using element_type = float;
        };

        template <typename T>
        struct ParameterElementTypeImpl<T, typename std::enable_if_t<std::is_base_of_v<InternalParameterChoice, T>>>
            : std::true_type
        {
            using base_type = InternalParameterChoice
            using element_type = int;
        };

        template <typename T>
        struct ParameterElementTypeImpl<T, typename std::enable_if_t<std::is_base_of_v<InternalParameterBool, T>>>
            : std::true_type
        {
            using base_type =InternalParameterBool;
            using element_type = bool;
        };
        /** Returns the base type of the parameter */
        template <typename ParamType>
        using ParameterBaseType = typename ParameterElementTypeImpl<ParamType>::base_type;

        /** Returns the element type of the parameter */
        template <typename ParamType>
        using ParameterElementType = typename ParameterElementTypeImpl<ParamType>::element_type;

        template <typename ParamType>
        ParameterElementType<ParamType> getValue (const ParamType& param)
        {
            if constexpr (std::is_base_of_v<InternalFloatParameter, ParamType> || std::is_base_of_v<InternalParameterBool, ParamType>)
                return param.get();
            else if constexpr (std::is_base_of_v<InternalParameterChoice, ParamType>)
                return param.getIndex();
        }

        /** Resets a parameter to it's default value. */
        inline void resetParameter (InternalParameter& param)
        {
            param.setValueNotifyingHost(param.getDefaultValue());
        }

        template <typename Serializer, typename ParamType>
        void serializeParameter (typename Serializer::SerializedType& serial, const ParamType& param)
        {

            Serializer::addChildElement (serial, param.paramID,getValue (param));
        }

        template <typename ParamType>
        void setValue (ParameterElementType<ParamType> val, ParamType& param)
        {
            static_cast<ParameterBaseType<ParamType>&> (param) = val;
        }
//
//        template <typename Serializer, typename ParamType>
//        std::enable_if_t<std::is_same_v<Serializer, chowdsp::XMLSerializer>, void>
//            deserializeParameter (const typename Serializer::SerializedType& serial, ParamType& param)
//        {
//            ParameterElementType<ParamType> val;
//            Serialization::deserialize<Serializer> (serial,param.paramID, val);
//            if constexpr (std::is_same_v<ParameterElementType<ParamType>,bool>)
//            {
//                DBG("paramid" + param.paramID + "val " + juce::String(static_cast<int>(val)));
//            } else
//            {
//                DBG("paramid" + param.paramID + "val " + juce::String(val));
//            }
//
//            setValue (val, param);
//        }
//        template <typename Serializer, typename ParamType>
//        std::enable_if_t<std::is_same_v<Serializer, XMLSerializer>, void>
//            deserializeParameter (const typename Serializer::DeserializedType& serial, ParamType& param)
//        {
//            ParameterElementType<ParamType> val;
//            Serialization::deserialize<Serializer> (serial,param.paramID, val);
//            //        if constexpr (std::is_same_v<ParameterElementType<ParamType>,bool>)
//            //        {
//            //            DBG("paramid" + param.paramID + "val " + juce::String(static_cast<int>(val)));
//            //        } else
//            //        {
//            //            DBG("paramid" + param.paramID + "val " + juce::String(val));
//            //        }
//
//            setValue (val, param);
//        }
    } // namespace ParameterTypeHelpers
#endif
} // namespace chowdsp

#endif //ELECTORSYNTH_DAVIS_PARAMTYPEHELPERS_H
