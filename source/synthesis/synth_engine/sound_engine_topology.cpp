#include "sound_engine.h"

#include "ModuleGraph.h"
#include "RoutingProcessor.h"
#include "Modulators/ModulatorBase.h"
#include "Modulators/EnvModuleProcessor.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace electrosynth
{
    static std::string getConnectionKey(const juce::String& sourceNodeId,
                                        const juce::String& destinationNodeId)
    {
        return sourceNodeId.toStdString() + "->" + destinationNodeId.toStdString();
    }

    static ConnectionRecord makeTopologyAudioConnection(const juce::String& sourceNodeId,
                                                        const juce::String& destinationNodeId)
    {
        ConnectionRecord connection;
        connection.id = "topology_audio:" + sourceNodeId + "->" + destinationNodeId;
        connection.type = ConnectionType::Audio;
        connection.source = {
            .type = ConnectionType::Audio,
            .nodeId = sourceNodeId,
            .endpointId = "audio_out",
            .direction = EndpointType::Source
        };
        connection.destination = {
            .type = ConnectionType::Audio,
            .nodeId = destinationNodeId,
            .endpointId = "audio_in",
            .direction = EndpointType::Destination
        };
        connection.topologyDerived = true;
        return connection;
    }

    void SoundEngine::registerModule(ModuleBase* module)
    {
        if (module == nullptr)
            return;

        const auto nodeId = module->getNodeId();
        if (nodeId.isEmpty())
            return;

        if (module->procArray != nullptr)
        {
            for (auto* header : *module->procArray)
            {
                if (header != nullptr)
                    header->outputs[0] = 0.0f;
            }
        }

        moduleRegistry_[nodeId] = module;
    }

    void SoundEngine::registerEffectLaneNodeId(int lane, const juce::String& nodeId) noexcept
    {
        if (!juce::isPositiveAndBelow(lane, static_cast<int>(laneNodeIds.size())))
            return;

        laneNodeIds[static_cast<std::size_t>(lane)] = nodeId;
    }

    void SoundEngine::registerModulePlacement(ModuleBase* module,
                                              ModuleGraph::NodeKind kind,
                                              int groupIndex,
                                              int orderIndex,
                                              bool inLane,
                                              bool inProcessorChain)
    {
        if (module == nullptr || moduleGraph_ == nullptr)
            return;

        const auto nodeId = module->getNodeId();
        if (nodeId.isEmpty())
            return;

        moduleGraph_->registerNode(nodeId, module, kind, groupIndex, orderIndex, inLane, inProcessorChain);
    }

    void SoundEngine::refreshModuleGraphTopology()
    {
        if (moduleGraph_ == nullptr)
            return;

        std::unordered_map<std::string, float> previousTopologyAudioAmounts;
        previousTopologyAudioAmounts.reserve(moduleGraph_->getConnections().size());
        for (const auto& connection : moduleGraph_->getConnections())
        {
            if (connection.type == electrosynth::ConnectionType::Audio && connection.topologyDerived)
                previousTopologyAudioAmounts.emplace(getConnectionKey(connection.source.nodeId, connection.destination.nodeId), connection.amount);
        }

        std::vector<juce::String> topologyConnectionsToRemove;
        for (const auto& connection : moduleGraph_->getConnections())
            if (connection.type == electrosynth::ConnectionType::Audio && connection.topologyDerived)
                topologyConnectionsToRemove.push_back(connection.id);
        for (const auto& connectionId : topologyConnectionsToRemove)
            moduleGraph_->disconnect(connectionId);

        chainExitModules_.clear();
        laneBridgeAudioConnections_.clear();
        for (std::size_t chainIndex = 0; chainIndex < processors.size(); ++chainIndex)
        {
            auto& chain = processors[chainIndex];
            ModuleBase* previousModule = nullptr;
            for (std::size_t orderIndex = 0; orderIndex < chain.size(); ++orderIndex)
            {
                if (auto* module = chain[orderIndex].get())
                {
                    moduleGraph_->registerNode(module->getNodeId(), module,
                        ModuleGraph::NodeKind::AudioModule,
                        static_cast<int>(chainIndex),
                        static_cast<int>(orderIndex),
                        false,
                        true);

                    if (previousModule != nullptr)
                    {
                        auto connection = makeTopologyAudioConnection(previousModule->getNodeId(), module->getNodeId());
                        if (const auto amountIt = previousTopologyAudioAmounts.find(
                                getConnectionKey(connection.source.nodeId, connection.destination.nodeId));
                            amountIt != previousTopologyAudioAmounts.end())
                        {
                            connection.amount = amountIt->second;
                        }

                        moduleGraph_->connect(connection);
                    }

                    previousModule = module;
                }
            }

            if (previousModule != nullptr)
                chainExitModules_.insert(previousModule);

            const auto* routingProcessor = chainIndex < chainPostGain.size() ? chainPostGain[chainIndex].get() : nullptr;
            if (routingProcessor == nullptr)
                continue;

            const int routingIndex = routingProcessor->getRoutingIndex();
            if (routingIndex <= 0)
                continue;

            const int laneIndex = routingIndex - 1;
            if (!juce::isPositiveAndBelow(laneIndex, static_cast<int>(effects.size())))
                continue;

            auto& lane = effects[static_cast<std::size_t>(laneIndex)];
            ModuleBase* firstLaneModule = nullptr;
            for (std::size_t laneOrderIndex = 0; laneOrderIndex < lane.size(); ++laneOrderIndex)
            {
                if (auto* module = lane[laneOrderIndex].get())
                {
                    firstLaneModule = module;
                    break;
                }
            }

            if (previousModule != nullptr && firstLaneModule != nullptr)
            {
                auto connection = makeTopologyAudioConnection(previousModule->getNodeId(), firstLaneModule->getNodeId());
                if (const auto amountIt = previousTopologyAudioAmounts.find(
                        getConnectionKey(connection.source.nodeId, connection.destination.nodeId));
                    amountIt != previousTopologyAudioAmounts.end())
                {
                    connection.amount = amountIt->second;
                }

                moduleGraph_->connect(connection);
            }
        }

        for (std::size_t laneIndex = 0; laneIndex < effects.size(); ++laneIndex)
        {
            auto& lane = effects[laneIndex];
            ModuleBase* previousModule = nullptr;
            for (std::size_t orderIndex = 0; orderIndex < lane.size(); ++orderIndex)
            {
                if (auto* module = lane[orderIndex].get())
                {
                    moduleGraph_->registerNode(module->getNodeId(), module,
                        ModuleGraph::NodeKind::AudioModule,
                        static_cast<int>(laneIndex),
                        static_cast<int>(orderIndex),
                        true,
                        false);
                    if (previousModule != nullptr)
                    {
                        auto connection = makeTopologyAudioConnection(previousModule->getNodeId(), module->getNodeId());
                        if (const auto amountIt = previousTopologyAudioAmounts.find(
                                getConnectionKey(connection.source.nodeId, connection.destination.nodeId));
                            amountIt != previousTopologyAudioAmounts.end())
                        {
                            connection.amount = amountIt->second;
                        }

                        moduleGraph_->connect(connection);
                    }
                    previousModule = module;
                }
            }
        }

        std::array<ModuleBase*, 3> firstLaneModules {};
        for (std::size_t laneIndex = 0; laneIndex < effects.size(); ++laneIndex)
        {
            for (const auto& modulePtr : effects[laneIndex])
            {
                if (modulePtr != nullptr)
                {
                    firstLaneModules[laneIndex] = modulePtr.get();
                    break;
                }
            }
        }

        for (std::size_t laneIndex = 0; laneIndex < effects.size(); ++laneIndex)
        {
            if (!juce::isPositiveAndBelow(static_cast<int>(laneIndex), static_cast<int>(laneNodeIds.size())))
                continue;

            const auto& laneNodeId = laneNodeIds[laneIndex];
            auto* firstLaneModule = firstLaneModules[laneIndex];
            if (laneNodeId.isEmpty() || firstLaneModule == nullptr)
                continue;

            for (const auto& connection : moduleGraph_->getIncoming(laneNodeId))
            {
                if (connection.type != electrosynth::ConnectionType::Audio || connection.topologyDerived)
                    continue;

                const auto* sourceNode = moduleGraph_->getNode(connection.source.nodeId);
                if (sourceNode == nullptr || sourceNode->module == nullptr)
                    continue;

                const auto sourceNodeId = sourceNode->module->getNodeId();
                const auto destinationNodeId = firstLaneModule->getNodeId();
                if (sourceNodeId.isEmpty() || destinationNodeId.isEmpty())
                    continue;

                bool alreadyBridged = false;
                for (const auto& bridge : laneBridgeAudioConnections_)
                {
                    if (bridge.sourceModule != nullptr
                        && bridge.destinationModule != nullptr
                        && bridge.sourceModule->getNodeId() == sourceNodeId
                        && bridge.destinationModule->getNodeId() == destinationNodeId)
                    {
                        alreadyBridged = true;
                        break;
                    }
                }

                if (sourceNodeId == destinationNodeId || alreadyBridged)
                    continue;

                laneBridgeAudioConnections_.push_back({
                    sourceNode->module,
                    firstLaneModule,
                    connection.amount
                });
            }
        }

        for (std::size_t laneIndex = 0; laneIndex < modSources.size(); ++laneIndex)
        {
            auto& lane = modSources[laneIndex];
            for (std::size_t orderIndex = 0; orderIndex < lane.size(); ++orderIndex)
            {
                if (auto* module = lane[orderIndex].get())
                    moduleGraph_->registerNode(module->getNodeId(), module,
                        ModuleGraph::NodeKind::Modulator,
                        static_cast<int>(laneIndex),
                        static_cast<int>(orderIndex),
                        false,
                        false);
            }
        }

        std::unordered_set<std::string> topologyAudioSources;
        topologyAudioSources.reserve(moduleGraph_->getConnections().size());
        for (const auto& connection : moduleGraph_->getConnections())
            if (connection.type == electrosynth::ConnectionType::Audio && connection.topologyDerived)
                topologyAudioSources.insert(connection.source.nodeId.toStdString());

        for (const auto& nodeId : moduleGraph_->getNodeIds(ModuleGraph::NodeKind::AudioModule))
        {
            const bool isTerminal = topologyAudioSources.find(nodeId.toStdString()) == topologyAudioSources.end();
            moduleGraph_->setNodeTerminal(nodeId, isTerminal);
        }

        terminalAudioModules_.clear();
        for (const auto& nodeId : moduleGraph_->getNodeIds(ModuleGraph::NodeKind::AudioModule))
        {
            const auto* node = moduleGraph_->getNode(nodeId);
            if (node != nullptr && node->terminal && node->module != nullptr)
                terminalAudioModules_.push_back(node->module);
        }

        moduleGraph_->debugPrint("ModuleGraph: ");
        debugPrintTerminalAudioModules("Terminal modules after topology refres...");
        debugPrintChainExitModules();

        rebuildRuntimeTopologySnapshot();
    }

    std::shared_ptr<const RuntimeTopologySnapshot> SoundEngine::getRuntimeTopologySnapshot() const noexcept
    {
        return std::atomic_load_explicit(&runtimeTopologySnapshot_, std::memory_order_acquire);
    }

    void SoundEngine::rebuildRuntimeTopologySnapshot()
    {
        auto snapshot = std::make_shared<RuntimeTopologySnapshot>();

        if (moduleGraph_ != nullptr)
        {
            snapshot->audioConnections.reserve(moduleGraph_->getConnections().size());
            snapshot->modulationConnections.reserve(moduleGraph_->getConnections().size());
            for (const auto& connection : moduleGraph_->getConnections())
            {
                if (!connection.isValid())
                    continue;

                if (connection.type == electrosynth::ConnectionType::Modulation)
                {
                    const auto stateIt = modulationStates_.find(connection.id);
                    if (stateIt != modulationStates_.end() && stateIt->second != nullptr)
                    {
                        stateIt->second->record = connection;
                        stateIt->second->scalingValue.store(connection.amount, std::memory_order_relaxed);
                        snapshot->modulationConnections.push_back({
                            stateIt->second,
                            connection.amount
                        });
                    }
                    continue;
                }

                if (connection.type != electrosynth::ConnectionType::Audio)
                    continue;

                const auto* sourceNode = moduleGraph_->getNode(connection.source.nodeId);
                const auto* destinationNode = moduleGraph_->getNode(connection.destination.nodeId);
                if (sourceNode == nullptr || destinationNode == nullptr
                    || sourceNode->module == nullptr || destinationNode->module == nullptr)
                    continue;

                snapshot->audioConnections.push_back({
                    sourceNode->module,
                    destinationNode->module,
                    connection.amount,
                    chainExitModules_.contains(sourceNode->module)
                });
            }

            snapshot->terminalAudioModules.reserve(terminalAudioModules_.size());
            for (auto* module : terminalAudioModules_)
            {
                snapshot->terminalAudioModules.push_back({
                    module,
                    module != nullptr && chainExitModules_.contains(module)
                });
            }
            snapshot->audioConnections.reserve(snapshot->audioConnections.size() + laneBridgeAudioConnections_.size());
            for (const auto& bridgeConnection : laneBridgeAudioConnections_)
            {
                snapshot->audioConnections.push_back({
                    bridgeConnection.sourceModule,
                    bridgeConnection.destinationModule,
                    bridgeConnection.amount,
                    bridgeConnection.sourceModule != nullptr
                        && chainExitModules_.contains(bridgeConnection.sourceModule)
                });
            }
            snapshot->chainExitModules.reserve(chainExitModules_.size());
            snapshot->chainExitModules.insert(snapshot->chainExitModules.end(),
                                              chainExitModules_.begin(),
                                              chainExitModules_.end());
        }

        snapshot->modulesToTick.reserve(moduleRegistry_.size());
        for (const auto& [_, module] : moduleRegistry_)
            if (module != nullptr)
                snapshot->modulesToTick.push_back(module);

        snapshot->masterEnvelope = MasterVoiceEnvelopeProcessor.get();

        const std::shared_ptr<const RuntimeTopologySnapshot> constSnapshot = snapshot;
        std::atomic_store_explicit(&runtimeTopologySnapshot_, constSnapshot, std::memory_order_release);
    }

    void SoundEngine::unregisterModule(ModuleBase* module)
    {
        if (module == nullptr)
            return;

        const auto nodeId = module->getNodeId();
        if (nodeId.isEmpty())
            return;

        auto it = moduleRegistry_.find(nodeId);
        if (it != moduleRegistry_.end() && it->second == module)
            moduleRegistry_.erase(it);
        if (moduleGraph_ != nullptr)
            moduleGraph_->unregisterNode(nodeId);
    }

    ModuleBase* SoundEngine::getModuleByNodeId(const juce::String& nodeId) const
    {
        if (nodeId.isEmpty())
            return nullptr;

        const auto it = moduleRegistry_.find(nodeId);
        return it != moduleRegistry_.end() ? it->second : nullptr;
    }

    int SoundEngine::getEffectLaneIndex(const juce::String& nodeId) const noexcept
    {
        if (nodeId.isEmpty())
            return -1;

        for (int lane = 0; lane < static_cast<int>(laneNodeIds.size()); ++lane)
        {
            if (laneNodeIds[static_cast<std::size_t>(lane)] == nodeId)
                return lane;
        }

        return -1;
    }

    bool SoundEngine::connectGraphConnection(const electrosynth::ConnectionRecord& connection)
    {
        if (moduleGraph_ == nullptr || !moduleGraph_->connect(connection))
            return false;

#if JUCE_DEBUG
        moduleGraph_->debugPrint("ModuleGraph after connect: " + connection.id);
#endif

        if (connection.type != electrosynth::ConnectionType::Modulation)
            return true;

        auto& statePtr = modulationStates_[connection.id];
        if (statePtr == nullptr)
            statePtr = std::make_shared<ModulationRuntimeState>();
        auto& state = *statePtr;
        state.record = connection;
        state.scalingValue.store(connection.amount, std::memory_order_relaxed);

        state.source = nullptr;
        if (auto* sourceModule = getModuleByNodeId(connection.source.nodeId))
            state.source = sourceModule->procArray;

        if (state.source == nullptr)
        {
            std::stringstream source_stream(connection.source.endpointId.toStdString());
            std::string source_token;
            std::getline(source_stream, source_token, '_');
            state.source = getLEAFProcessorModulator(source_token);
        }

        std::tie(state.destination, state.destinationParamIndex) = getParameterInfo(connection.destination.endpointId.toStdString());

        if (state.source == nullptr || state.destination == nullptr || state.destinationParamIndex < 0)
        {
            moduleGraph_->disconnect(connection.id);
            modulationStates_.erase(connection.id);
            rebuildRuntimeTopologySnapshot();
            return false;
        }

        for (int voice = 0; voice < MAX_NUM_VOICES; ++voice)
        {
            if (state.mapping[voice] == nullptr)
                tMapping_init(&state.mapping[voice], &leaf);

            auto* voice_mapping = state.mapping[voice];
            if (voice_mapping == nullptr
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.source->size()))
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.destination->size())))
                continue;

            auto* sourceModule = state.source->at(voice);
            auto* destinationModule = state.destination->at(voice);
            if (sourceModule == nullptr || destinationModule == nullptr)
                continue;

            auto* destValue = destinationModule->params[state.destinationParamIndex];
            if (destValue == nullptr)
                continue;

            tMappingAdd_(voice_mapping,
                &sourceModule->outputs[0],
                sourceModule->uniqueID,
                destValue,
                destinationModule->uniqueID,
                state.destinationParamIndex,
                destinationModule,
                &leaf,
                &state.scalingValue);
        }

        rebuildRuntimeTopologySnapshot();
        return true;
    }

    bool SoundEngine::updateGraphConnection(const electrosynth::ConnectionRecord& connection)
    {
        if (moduleGraph_ == nullptr || !moduleGraph_->update(connection))
            return false;

        if (connection.type == electrosynth::ConnectionType::Modulation)
        {
            auto stateIt = modulationStates_.find(connection.id);
            if (stateIt != modulationStates_.end() && stateIt->second != nullptr)
            {
                stateIt->second->record = connection;
                stateIt->second->scalingValue.store(connection.amount, std::memory_order_relaxed);
            }
        }

        return true;
    }

    void SoundEngine::disconnectGraphConnection(const juce::String& connectionId)
    {
        if (moduleGraph_ == nullptr)
            return;

        const auto& connections = moduleGraph_->getConnections();
        const auto connectionIt = std::find_if(connections.begin(), connections.end(),
            [&connectionId](const auto& connection)
            {
                return connection.id == connectionId;
            });

        const auto targetConnectionExists = connectionIt != connections.end();
        const auto* targetConnection = targetConnectionExists ? &*connectionIt : nullptr;

        const bool otherConnectionStillDrivesDestination =
            targetConnection != nullptr
            && std::any_of(connections.begin(), connections.end(),
                [targetConnection](const auto& connection)
                {
                    return connection.id != targetConnection->id
                        && connection.type == targetConnection->type
                        && connection.destination.endpointId == targetConnection->destination.endpointId
                        && connection.destinationSlot == targetConnection->destinationSlot;
                });

        auto stateIt = modulationStates_.find(connectionId);
        if (stateIt != modulationStates_.end() && stateIt->second != nullptr)
        {
            auto& state = *stateIt->second;
            for (auto& mapping : state.mapping)
            {
                if (mapping == nullptr)
                    continue;

                if (!otherConnectionStillDrivesDestination
                    && mapping->destObject != nullptr
                    && mapping->initialVal != nullptr
                    && mapping->paramID < 255)
                {
                    leaf::tMapping_setParameter(mapping->destObject, mapping->paramID, *mapping->initialVal);
                }

                auto* toFree = mapping;
                tMapping_free(&toFree);
                mapping = nullptr;
            }
            modulationStates_.erase(stateIt);
        }

        moduleGraph_->disconnect(connectionId);
        rebuildRuntimeTopologySnapshot();

#if JUCE_DEBUG
        moduleGraph_->debugPrint("ModuleGraph after disconnect: " + connectionId);
#endif
    }

    void SoundEngine::connectMapping(const electrosynth::mapping_change& change)
    {
        if (moduleGraph_ == nullptr || change.connection == nullptr)
            return;

        electrosynth::ConnectionRecord record {
            .id = juce::String(change.connection->uuid),
            .type = electrosynth::ConnectionType::Modulation,
            .source {
                .type = electrosynth::ConnectionType::Modulation,
                .nodeId = juce::String(change.connection->source_name),
                .endpointId = juce::String(change.connection->source_name),
                .direction = electrosynth::EndpointType::Source
            },
            .destination {
                .type = electrosynth::ConnectionType::Modulation,
                .nodeId = juce::String(change.connection->destination_name),
                .endpointId = juce::String(change.connection->destination_name),
                .direction = electrosynth::EndpointType::Destination
            },
            .destinationSlot = change.connection->destination_slot,
            .amount = change.connection->getCurrentBaseValue(),
            .bipolar = change.connection->isBipolar(),
            .bypass = change.connection->isBypass(),
            .stereo = change.connection->isStereo()
        };

        if (!moduleGraph_->connect(record))
            return;

        auto& statePtr = modulationStates_[record.id];
        if (statePtr == nullptr)
            statePtr = std::make_shared<ModulationRuntimeState>();
        auto& state = *statePtr;
        state.record = record;
        state.scalingValue.store(record.amount, std::memory_order_relaxed);
        state.source = change._source;
        state.destination = change._dest;
        state.destinationParamIndex = change.dest_param_index;

        if (state.source == nullptr || state.destination == nullptr || state.destinationParamIndex < 0)
        {
            moduleGraph_->disconnect(record.id);
            modulationStates_.erase(record.id);
            rebuildRuntimeTopologySnapshot();
            return;
        }

        for (int voice = 0; voice < MAX_NUM_VOICES; ++voice)
        {
            if (state.mapping[voice] == nullptr)
                tMapping_init(&state.mapping[voice], &leaf);

            auto* voice_mapping = state.mapping[voice];
            if (voice_mapping == nullptr
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.source->size()))
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.destination->size())))
                continue;

            auto* sourceModule = state.source->at(voice);
            auto* destinationModule = state.destination->at(voice);
            if (sourceModule == nullptr || destinationModule == nullptr)
                continue;

            auto* destValue = destinationModule->params[state.destinationParamIndex];
            if (destValue == nullptr)
                continue;
            tMappingAdd_(
                voice_mapping,
                &sourceModule->outputs[0],
                sourceModule->uniqueID,
                destValue,
                destinationModule->uniqueID,
                state.destinationParamIndex,
                destinationModule,
                &leaf,
                &state.scalingValue
            );
        }

        rebuildRuntimeTopologySnapshot();
        DBG("added new modulation");
    }

    void SoundEngine::disconnectMapping(const electrosynth::mapping_change& change)
    {
        if (change.connection == nullptr)
            return;

        const auto connectionId = juce::String(change.connection->uuid);
        if (moduleGraph_ != nullptr)
            moduleGraph_->disconnect(connectionId);

        auto stateIt = modulationStates_.find(connectionId);
        if (stateIt == modulationStates_.end() || stateIt->second == nullptr)
        {
            rebuildRuntimeTopologySnapshot();
            return;
        }

        auto& state = *stateIt->second;
        for (auto& mapping : state.mapping)
        {
            if (mapping == nullptr)
                continue;

            auto* toFree = mapping;
            tMapping_free(&toFree);
            mapping = nullptr;
        }

        modulationStates_.erase(stateIt);
        rebuildRuntimeTopologySnapshot();
    }
}
