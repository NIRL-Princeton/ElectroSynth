//
// Created by Davis Polito on 12/18/24.
//
#include "common.h"
#include "constants.h"
#include "ModulationConnection.h"

namespace electrosynth
{
    namespace
    {
        const std::string kModulationSourceDelimiter = "_";
        const std::set<std::string> kBipolarModulationSourcePrefixes = {
            "lfo",
            "stereo",
            "random",
            "pitch"
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

    bool ModulationConnection::isModulationSourceDefaultBipolar (const std::string& source)
    {
        std::size_t pos = source.find (kModulationSourceDelimiter);
        std::string prefix = source.substr (0, pos);
        return kBipolarModulationSourcePrefixes.count (prefix) > 0;
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
            tMapping_init(&(mappings.at(to)->mapping), leaf);

            return mappings.at(to).get();
        }

    }
    ModulationConnection* ModulationConnectionBank::createConnection (const std::string& from, const std::string& to)
    {
        int index = 1;
        for (auto& connection : all_connections_)
        {
            std::string invalid_connection = "modulation_" + std::to_string (index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable (connection.get()))
            {
                connection->resetConnection (from, to);


                connection->mapping = createMapping(to);
                //connection->modulation_processor->setBipolar(ModulationConnection::isModulationSourceDefaultBipolar(from));
                return connection.get();
            }
        }

        return nullptr;
    }

}