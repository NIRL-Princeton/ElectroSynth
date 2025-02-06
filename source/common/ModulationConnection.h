//
// Created by Davis Polito on 12/10/24.
//

#ifndef ELECTROSYNTH_MODULATIONCONNECTION_H
#define ELECTROSYNTH_MODULATIONCONNECTION_H
#include <juce_data_structures/juce_data_structures.h>
#include "Identifiers.h"

#include "processors/mapping.h"
#include "ModulationWrapper.h"

namespace electrosynth {
struct MappingWrapper;
    struct ModulationConnection {
        ModulationConnection(const std::string& from, const std::string& to, LEAF& leaf,  int index)
            : source_name(from), destination_name(to),state(IDs::MODULATION),leaf_(leaf)
        {
            uuid = getNextUuid(&leaf);
            index_in_all_mods = index;
        }
        ~ModulationConnection()
        {
            //count--;
        }
        static bool isModulationSourceDefaultBipolar(const std::string& source);
        void setSource(int uuid_from)
        {
            state.setProperty(IDs::src, uuid_from, nullptr);
        }

        void setDestination(int uuid_to)
        {
            state.setProperty(IDs::src, uuid_to, nullptr);
        }

        void setModulationAmount(float amt)
        {
            state.setProperty(IDs::modAmt, amt, nullptr);
        }
        void setPolarity(bool isBipolar)
        {
            state.setProperty(IDs::isBipolar, isBipolar, nullptr);
        }
        void resetConnection(const std::string& from, const std::string& to) {
            source_name = from;
            destination_name = to;
        }

        float getCurrentBaseValue()
        {
            if(scalingValue_ != nullptr)
            {
                return scalingValue_->load();
            }
            return 0.5f;
        }
        void setScalingValue(float val)
        {
            if(scalingValue_ != nullptr)
            {
                if (isBipolar())
                    scalingValue_->store(val *0.5f);
                else
                    scalingValue_->store(val);
            }
            DBG(juce::String(val));
        }

        void setBypass(bool bypass) {}
        void setStereo(bool stereo) {}
        bool isBipolar() const { return bipolar_; }
        bool isBypass() const {return bypass_; }
        bool isStereo() const {return stereo_; }
        bool setDefaultBipolar (bool val)
        {
            defaultBipolar = val;
            setBipolar(val);
        }
        void setBipolar(bool bipolar) {
            bipolar_ = bipolar;
            if(bipolarOffset != nullptr && !defaultBipolar)
            {
                *bipolarOffset = bipolar_ ? 0.5f : 0.0f;
            }
            if(bipolarOffset != nullptr && defaultBipolar)
            {
                *bipolarOffset = bipolar_ ? 0.0f : 0.5f;
            }

        }

        std::string source_name;
        std::string destination_name;        //must be named state to be picked up by valuetreeobjectlist - dont know
        // if i'll be using this for that or not
        juce::ValueTree state;
        int index_in_all_mods;
        int index_in_mapping;
        int uuid;
        bool bipolar_;
        bool bypass_;
        bool stereo_;
        bool defaultBipolar;
        LEAF &leaf_;
        leaf::tProcessor* sourceProc_;
        std::atomic<float>* scalingValue_;
        std::atomic<float>* bipolarOffset;

        MappingWrapper* mapping_;
    };

    struct MappingWrapper
    {
        leaf::Mapping mapping_;
        std::vector<ModulationConnection*> all_connections_;
        std::string dest_;

        void reorderMapping();
    };
        typedef struct mapping_change
        {
            bool disconnecting;
            MappingWrapper* mapping;
            ModulationConnection* connection;
            std::string destination;
            std::string source;
            int dest_param_index;
            int source_param_uuid;
            leaf::tProcessor * _source;
            leaf::tProcessor * _dest;
        }mapping_change;


    class ModulationConnectionBank {
    public:
        ModulationConnectionBank(LEAF &leaf);
        ~ModulationConnectionBank();
        ModulationConnection* createConnection(const std::string& from, const std::string& to);
        MappingWrapper* createMapping( const std::string& to);
        ModulationConnection* atIndex(int index) { return all_connections_[index].get(); }
        size_t numConnections() { return all_connections_.size(); }

    private:
        LEAF& leaf;
        std::vector<std::unique_ptr<ModulationConnection>> all_connections_;
        std::map<std::string, std::unique_ptr<MappingWrapper>> mappings;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationConnectionBank)
    };
}

#endif //ELECTROSYNTH_MODULATIONCONNECTION_H
