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
            : source_name(from), destination_name(to),state(IDs::MODULATION),leaf(leaf)
        {
            uuid = getNextUuid(&leaf);
            index_in_all_mods = index;
        }
        ~ModulationConnection()
        {
            //count--;
        }
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
        static bool isModulationSourceDefaultBipolar(const std::string& source);
        std::string source_name;
        std::string destination_name;
        juce::ValueTree state;
        int index_in_all_mods;
        int index_in_mapping;
        int uuid;
        LEAF &leaf;
        leaf::tProcessor* sourceProc;
        std::atomic<float>* scalingValue;
        MappingWrapper* mapping;
    };

    struct MappingWrapper
    {
        leaf::Mapping mapping;
        std::vector<ModulationConnection*> procIndex;
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
