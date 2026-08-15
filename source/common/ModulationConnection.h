//
// Created by Davis Polito on 12/10/24.
//

#ifndef ELECTROSYNTH_MODULATIONCONNECTION_H
#define ELECTROSYNTH_MODULATIONCONNECTION_H
#include <juce_data_structures/juce_data_structures.h>
#include "Identifiers.h"

#include "processors/mapping.h"
#include "ModulationWrapper.h"
#include "leaf.h"
namespace electrosynth {
    struct MappingWrapper;

    struct Connection {
        Connection(const std::string& from, const std::string& to, LEAF& leaf, int index)
            : source_name(from), destination_name(to), state(IDs::MODULATION), index_in_all_mods(index),
              index_in_mapping(-1), destination_slot(-1), uuid(getNextUuid(&leaf)), bipolar_(false), bypass_(false),
              stereo_(false), defaultBipolar(false), leaf_(leaf), sourceProc_(nullptr), scalingValue_(0.0f),
              bipolarOffset(nullptr), mapping_(nullptr) {
        }

        ~Connection() {
            //count--;
        }

        static bool isModulationSourceDefaultBipolar(const std::string& source);

        void setSource(int uuid_from) {
            state.setProperty(IDs::src, uuid_from, nullptr);
        }

        void setDestination(int uuid_to) {
            state.setProperty(IDs::dest, uuid_to, nullptr);
        }

        void setModulationAmount(float amt) {
            state.setProperty(IDs::modAmt, amt, nullptr);
        }

        void setPolarity(bool isBipolar) {
            state.setProperty(IDs::isBipolar, isBipolar, nullptr);
        }

        void resetConnection(const std::string& from, const std::string& to, int slot) {
            source_name = from;
            destination_name = to;
            destination_slot = slot;
            state.setProperty(IDs::src, juce::String(from), nullptr);
            state.setProperty(IDs::dest, juce::String(to), nullptr);
            state.setProperty(IDs::destIdx, slot, nullptr);
        }

        void clearConnection() {
            source_name.clear();
            destination_name.clear();
            destination_slot = -1;
            state.removeProperty(IDs::src, nullptr);
            state.removeProperty(IDs::dest, nullptr);
            state.removeProperty(IDs::destIdx, nullptr);
        }

        float getScaledAmountForMapping(float val) const {
            return isBipolar() ? val * 0.5f : val;
        }

        void updateEffectiveScalingValue() { // audio mapping and process mapping reads scalingValue_ to adjust modulation effect
            scalingValue_.store(bypass_ ? 0.0f : baseScalingValue_.load());
        }

        float getCurrentBaseValue() {
            return baseScalingValue_.load();
        }


        void setScalingValue(float val) {
            baseScalingValue_.store(getScaledAmountForMapping (val));
            updateEffectiveScalingValue();
        }

        void setBypass(bool bypass) {
            bypass_ = bypass;
            updateEffectiveScalingValue();
        }

        void setStereo(bool stereo) { stereo_ = stereo; }
        bool isBipolar() const { return bipolar_; }
        bool isBypass() const {return bypass_; }
        bool isStereo() const {return stereo_; }
        bool setDefaultBipolar (bool val) {
            defaultBipolar = val;
            setBipolar(val);
            return defaultBipolar;
        }
        void setBipolar(bool bipolar) {
            bipolar_ = bipolar;
            if(bipolarOffset != nullptr && !defaultBipolar) {
                *bipolarOffset = bipolar_ ? 0.5f : 0.0f;
            }
            if(bipolarOffset != nullptr && defaultBipolar) {
                *bipolarOffset = bipolar_ ? 0.0f : 0.5f;
            }
        }

        std::string source_name;
        std::string destination_name;        //must be named state to be picked up by valuetreeobjectlist - dont know
        // if i'll be using this for that or not
        juce::ValueTree state;
        int index_in_all_mods;
        int index_in_mapping;
        int destination_slot;
        int uuid;
        bool bipolar_;
        bool bypass_;
        bool stereo_;
        bool defaultBipolar;
        LEAF &leaf_;

        std::array<ModuleHeader*, MAX_NUM_VOICES>* sourceProc_;
        std::atomic<float> baseScalingValue_ { 0.0f }; // UI/user amount
        std::atomic<float> scalingValue_; // DSP effective amount
        std::atomic<float>* bipolarOffset;

        MappingWrapper* mapping_;
    };

    struct MappingWrapper {
        leaf::Mapping mapping_[MAX_NUM_VOICES];
        std::vector<Connection*> all_connections_;
        std::string dest_;

        int indexOfConnection(const Connection* connection) const;
        void addConnection(Connection* connection);
        bool removeConnection(Connection* connection);
        bool moveConnection(Connection* connection, int new_index);

        void reorderMapping();
    };
        typedef struct mapping_change
        {
            bool disconnecting;
            MappingWrapper* mapping;
            Connection* connection;
            std::string destination;
            std::string source;
            int dest_param_index;
            int source_param_uuid;
            std::array<ModuleHeader*, MAX_NUM_VOICES>*_source;
            std::array<ModuleHeader*, MAX_NUM_VOICES>*_dest;
        }  mapping_change;


    class ConnectionBank {
    public:
        ConnectionBank(LEAF &leaf);
        ~ConnectionBank();
        Connection* createConnection(const std::string& from, const std::string& to, int destination_slot);
        MappingWrapper* createMapping( const std::string& to);
        Connection* atIndex(int index) { return all_connections_[index].get(); }
        size_t numConnections() { return all_connections_.size(); }

    private:
        LEAF& leaf;
        std::vector<std::unique_ptr<Connection>> all_connections_;
        std::map<std::string, std::unique_ptr<MappingWrapper>> mappings;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionBank)
    };
}

#endif //ELECTROSYNTH_MODULATIONCONNECTION_H
