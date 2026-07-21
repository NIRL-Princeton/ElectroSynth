//
// Created by Davis Polito on 12/18/24.
//
#include "common.h"
#include "ModulationConnection.h"
#include <algorithm>
namespace electrosynth
{
    namespace
    {
        const std::string kModulationSourceDelimiter = "_";
        const std::set<std::string> kBipolarModulationSourcePrefixes = {
            "lfo",
            "random",
            "pitch",
            "audio"
        };

        force_inline bool isConnectionAvailable (ModulationConnection* connection)
        {
            return connection->source_name.empty() && connection->destination_name.empty();
        }
    }

    //    ModulationConnection::ModulationConnection(int index, std::string from, std::string to) :
    //                                                                                               source_name(std::move(from)), destination_name(std::move(to)) {
    //        modulation_processor = std::make_unique<ModulationConnectionProcessor>(index);
    //    }

    //    ModulationConnection::~ModulationConnection() { }
    bool ModulationConnection::isModulationSourceDefaultBipolar(const std::string& source) {
        //std::size_t pos = source.find(kModulationSourceDelimiter);
        std::size_t pos = source.find_first_of("0123456789");
        std::string prefix = source.substr(0, pos);
        return kBipolarModulationSourcePrefixes.count(prefix) > 0;
    }
    void MappingWrapper::reorderMapping()
    {
        const int previous_num_used = mapping_[0].numUsedSources;
        int i =0;
        for (auto* connection : all_connections_)
        {
            if (connection == nullptr || connection->sourceProc_ == nullptr)
                continue;

            const float scaleCurr = connection->scalingValue_.load();
            const float bipolarOffset = connection->bipolarOffset != nullptr ? connection->bipolarOffset->load() : 0.0f;
            for ( int v = 0; v < MAX_NUM_VOICES; v++) {
                mapping_[v].inUUIDS[i] = connection->uuid;
                mapping_[v].scalingValues[i] = &connection->scalingValue_;
                mapping_[v].inSources[i] = &connection->sourceProc_->at(v)->outputs[0];
            }

            connection->bipolarOffset = &mapping_[0].bipolarOffset[i];

            connection->scalingValue_.store(scaleCurr);
            connection->bipolarOffset->store(bipolarOffset);
            connection->index_in_mapping = i;
            i++;
        }

        for (int v = 0; v < MAX_NUM_VOICES; ++v) {
            for (int slot = i; slot < previous_num_used; ++slot) {
                mapping_[v].inUUIDS[slot] = 0;
                mapping_[v].inSources[slot] = nullptr;
                mapping_[v].scalingValues[slot] = nullptr;
                mapping_[v].bipolarOffset[slot] = 0.0f;
            }
            mapping_[v].numUsedSources = i;
        }

        jassert(mapping_[0].numUsedSources == i);
        //mapping_.numUsedSources = i;

    }

    int MappingWrapper::indexOfConnection(const ModulationConnection* connection) const
    {
        auto it = std::find(all_connections_.begin(), all_connections_.end(), connection);
        if (it == all_connections_.end())
            return -1;
        return static_cast<int>(std::distance(all_connections_.begin(), it));
    }

    void MappingWrapper::addConnection(ModulationConnection* connection)
    {
        if (connection == nullptr)
            return;

        if (indexOfConnection(connection) >= 0)
            return;

        all_connections_.push_back(connection);
        reorderMapping();
    }

    bool MappingWrapper::removeConnection(ModulationConnection* connection)
    {
        auto it = std::remove(all_connections_.begin(), all_connections_.end(), connection);
        if (it == all_connections_.end())
            return false;

        all_connections_.erase(it, all_connections_.end());
        reorderMapping();
        return true;
    }

    bool MappingWrapper::moveConnection(ModulationConnection* connection, int new_index)
    {
        if (connection == nullptr)
            return false;

        auto current_index = indexOfConnection(connection);
        if (current_index < 0)
            return false;

        new_index = juce::jlimit(0, static_cast<int>(all_connections_.size()) - 1, new_index);
        if (new_index == current_index)
            return true;

        auto moved = all_connections_[current_index];
        all_connections_.erase(all_connections_.begin() + current_index);
        all_connections_.insert(all_connections_.begin() + new_index, moved);
        reorderMapping();
        return true;
    }


    ModulationConnectionBank::ModulationConnectionBank (LEAF& _leaf) : leaf(_leaf)
    {
        for (int i = 0; i < kMaxModulationConnections; ++i)
        {
            std::unique_ptr<ModulationConnection> connection = std::make_unique<ModulationConnection> ("", "", leaf, i);
            all_connections_.push_back (std::move (connection));
        }
    }

    ModulationConnectionBank::~ModulationConnectionBank() {}

    MappingWrapper* ModulationConnectionBank::createMapping(const std::string &to)
    {
        try
        {
           auto& mWrapper =  mappings.at(to);
           return mWrapper.get();
        }
        catch(const std::out_of_range& ex)
        {

            mappings.emplace(to, std::make_unique<MappingWrapper>());
//            tMapping_init(&(mappings.at(to)->mapping_), leaf);
            mappings.at(to)->dest_ = to;
            return mappings.at(to).get();
        }

    }
    ModulationConnection* ModulationConnectionBank::createConnection(
        const std::string& from, const std::string& to, int destination_slot)
    {
        int index = 1;
        for (auto& connection : all_connections_)
        {
            std::string invalid_connection = "modulation_" + std::to_string (index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable (connection.get()))
            {
                connection->resetConnection(from, to, destination_slot);


                connection->mapping_ = createMapping(to);
                connection->setDefaultBipolar(ModulationConnection::isModulationSourceDefaultBipolar(from));

                return connection.get();
            }
        }

        return nullptr;
    }

}
