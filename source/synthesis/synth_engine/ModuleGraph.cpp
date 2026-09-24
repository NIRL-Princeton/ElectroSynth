//
// Created by airship on 8/16/26.
//

#include "ModuleGraph.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "ModuleBase.h"

namespace electrosynth
{
    void ModuleGraph::registerNode(const juce::String& nodeId,
                                   ModuleBase* module,
                                   NodeKind kind,
                                   int groupIndex,
                                   int orderIndex,
                                   bool inLane,
                                   bool inProcessorChain)
    {
        if (nodeId.isEmpty())
            return;

        auto& record = nodes_[nodeId];
        if (module != nullptr)
            record.module = module;
        if (kind != NodeKind::Unknown || record.kind == NodeKind::Unknown)
            record.kind = kind;
        if (groupIndex >= 0)
            record.groupIndex = groupIndex;
        if (orderIndex >= 0)
            record.orderIndex = orderIndex;
        record.inLane = inLane;
        record.inProcessorChain = inProcessorChain;
    }

    void ModuleGraph::unregisterNode(const juce::String& nodeId)
    {
        if (nodeId.isEmpty())
            return;

        nodes_.erase(nodeId);
    }

    void ModuleGraph::setNodeKind(const juce::String& nodeId, NodeKind kind)
    {
        if (nodeId.isEmpty())
            return;

        auto& record = nodes_[nodeId];
        record.kind = kind;
    }

    void ModuleGraph::setNodePlacement(const juce::String& nodeId,
                                       int groupIndex,
                                       int orderIndex,
                                       bool inLane,
                                       bool inProcessorChain)
    {
        if (nodeId.isEmpty())
            return;

        auto& record = nodes_[nodeId];
        record.groupIndex = groupIndex;
        record.orderIndex = orderIndex;
        record.inLane = inLane;
        record.inProcessorChain = inProcessorChain;
    }

    void ModuleGraph::setNodeTerminal(const juce::String& nodeId, bool terminal)
    {
        if (nodeId.isEmpty())
            return;

        auto& record = nodes_[nodeId];
        record.terminal = terminal;
    }

    bool ModuleGraph::hasNode(const juce::String& nodeId) const
    {
        return nodeId.isNotEmpty() && nodes_.find(nodeId) != nodes_.end();
    }

    const ModuleGraph::NodeRecord* ModuleGraph::getNode(const juce::String& nodeId) const
    {
        if (nodeId.isEmpty())
            return nullptr;

        const auto it = nodes_.find(nodeId);
        return it != nodes_.end() ? &it->second : nullptr;
    }

    std::vector<juce::String> ModuleGraph::getNodeIds(NodeKind kind, int groupIndex) const
    {
        std::vector<std::pair<int, juce::String>> ordered;
        ordered.reserve(nodes_.size());

        for (const auto& [nodeId, record] : nodes_)
        {
            if (record.kind != kind)
                continue;
            if (groupIndex >= 0 && record.groupIndex != groupIndex)
                continue;

            ordered.emplace_back(record.orderIndex, nodeId);
        }

        std::sort(ordered.begin(), ordered.end(),
            [] (const auto& lhs, const auto& rhs)
            {
                if (lhs.first != rhs.first)
                    return lhs.first < rhs.first;
                return lhs.second < rhs.second;
            });

        std::vector<juce::String> nodeIds;
        nodeIds.reserve(ordered.size());
        for (const auto& [_, nodeId] : ordered)
            nodeIds.push_back(nodeId);

        return nodeIds;
    }

    bool ModuleGraph::connect(const electrosynth::ConnectionRecord& connection)
    {
        if (!connection.isValid())
            return false;

        const auto existing = std::find_if(connections_.begin(), connections_.end(),
            [&connection](const ConnectionRecord& existingConnection)
            {
                return existingConnection.id == connection.id;
            });
        if (existing != connections_.end())
            return false;

        connections_.push_back(connection);
        return true;
    }

    bool ModuleGraph::update(const electrosynth::ConnectionRecord& connection)
    {
        if (!connection.isValid())
            return false;

        const auto existing = std::find_if(connections_.begin(), connections_.end(),
            [&connection](const ConnectionRecord& existingConnection)
            {
                return existingConnection.id == connection.id;
            });
        if (existing == connections_.end())
            return false;

        *existing = connection;
        return true;
    }

    void ModuleGraph::disconnect(const juce::String& connectionId)
    {
        if (connectionId.isEmpty())
            return;

        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [&connectionId](const ConnectionRecord& connection)
                {
                    return connection.id == connectionId;
                }),
            connections_.end());
    }

    juce::String ModuleGraph::toDebugString() const
    {
        const auto labelForId = [this](const juce::String& nodeId)
        {
            if (const auto* node = getNode(nodeId))
            {
                if (node->module != nullptr)
                {
                    const auto displayName = node->module->getDisplayName();
                    if (displayName.isNotEmpty())
                        return displayName;

                    const auto typeName = node->module->state.getProperty(IDs::type).toString();
                    if (typeName.isNotEmpty())
                        return typeName;
                }
            }

            return nodeId;
        };

        juce::String out;
        out << "ModuleGraph: " << juce::String(static_cast<int>(nodes_.size())) << " node(s), "
            << juce::String(static_cast<int>(connections_.size())) << " cross connection(s)\n";

        if (!nodes_.empty())
        {
            for (const auto& [nodeId, record] : nodes_)
            {
                auto kindToString = [] (NodeKind kind)
                {
                    switch (kind)
                    {
                        case NodeKind::Unknown: return "unknown";
                        case NodeKind::AudioModule: return "module";
                        case NodeKind::Modulator: return "mod";
                    }
                    return "unknown";
                };

                out << "    " << labelForId(nodeId)
                    << " kind=" << kindToString(record.kind)
                    << " inLane=" << (record.inLane ? "yes" : "no")
                    << " inProcessorChain=" << (record.inProcessorChain ? "yes" : "no")
                    << " terminal=" << (record.terminal ? "yes" : "no")
                    << " group=" << juce::String(record.groupIndex)
                    << " order=" << juce::String(record.orderIndex);
                if (record.module != nullptr)
                    out << " display=\"" << record.module->getDisplayName() << "\"";
                out << "\n";
            }
        }

        if (connections_.empty())
        {
            out << "No cross connections...\n";
            return out;
        }

        out << "Cross connections:\n";

        std::unordered_map<std::string, int> outgoingCounts;
        std::unordered_map<std::string, int> incomingCounts;
        for (const auto& connection : connections_)
        {
            ++outgoingCounts[connection.source.nodeId.toStdString()];
            ++incomingCounts[connection.destination.nodeId.toStdString()];
        }

        auto connectionTypeToString = [] (ConnectionType type)
        {
            switch (type)
            {
                case ConnectionType::Modulation: return "mod";
                case ConnectionType::Audio: return "audio";
            }
            return "unknown";
        };

        for (const auto& connection : connections_)
        {
                out << "    [" << connectionTypeToString(connection.type) << "] "
                // << connection.id << "  "
                << labelForId(connection.source.nodeId) << ":" << connection.source.endpointId << " -> "
                << labelForId(connection.destination.nodeId) << ":" << connection.destination.endpointId
                << "  slot=" << juce::String(connection.destinationSlot)
                << "  amount=" << juce::String(connection.amount, 4)
                << "  bypass=" << (connection.bypass ? "Y" : "N")
                << "  bipolar=" << (connection.bipolar ? "Y" : "N")
                << "  stereo=" << (connection.stereo ? "Y" : "N")
                << "  topology=" << (connection.topologyDerived ? "Y" : "N")
                << "\n";
        }


        /*
        out << "  Node summary:\n";
        for (const auto& [node, count] : outgoingCounts)
        {
            out << "    " << labelForId(juce::String(node)) << ": out=" << juce::String(count)
                << " in=" << juce::String(incomingCounts[node]) << "\n";
        }

        for (const auto& [node, count] : incomingCounts)
        {
            if (outgoingCounts.find(node) == outgoingCounts.end())
                out << "    " << labelForId(juce::String(node)) << ": out=0 in=" << juce::String(count) << "\n";
        }
        */

        return out;
    }

    void ModuleGraph::debugPrint(const juce::String& header) const
    {
        if (header.isNotEmpty())
            DBG(header);
        DBG(toDebugString());
    }

    std::vector<ConnectionRecord> ModuleGraph::getIncoming(const juce::String& nodeId) const
    {
        std::vector<ConnectionRecord> incoming;
        for (const auto& connection : connections_)
        {
            if (connection.destination.nodeId == nodeId)
                incoming.push_back(connection);
        }
        return incoming;
    }

    std::vector<ConnectionRecord> ModuleGraph::getOutgoing(const juce::String& nodeId) const
    {
        std::vector<ConnectionRecord> outgoing;
        for (const auto& connection : connections_)
        {
            if (connection.source.nodeId == nodeId)
                outgoing.push_back(connection);
        }
        return outgoing;
    }
}
