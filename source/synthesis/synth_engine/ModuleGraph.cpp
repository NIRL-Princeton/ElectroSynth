//
// Created by airship on 8/16/26.
//

#include "ModuleGraph.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace electrosynth
{
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

    juce::String ModuleGraph::toDebugString(const std::function<juce::String(const juce::String&)>& nodeLabelForId) const
    {
        const auto labelForId = [&nodeLabelForId](const juce::String& nodeId)
        {
            if (nodeLabelForId)
            {
                auto label = nodeLabelForId(nodeId);
                if (label.isNotEmpty())
                    return label;
            }
            return nodeId;
        };

        juce::String out;
        out << "ModuleGraph: " << juce::String(static_cast<int>(connections_.size())) << " connection(s)\n";

        if (connections_.empty())
        {
            out << "  <empty>\n";
            return out;
        }

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
            out << "  [" << connectionTypeToString(connection.type) << "] "
                << connection.id << "  "
                << labelForId(connection.source.nodeId) << ":" << connection.source.endpointId << " -> "
                << labelForId(connection.destination.nodeId) << ":" << connection.destination.endpointId
                << "  slot=" << juce::String(connection.destinationSlot)
                << "  amount=" << juce::String(connection.amount, 4)
                << "  flags="
                << (connection.bypass ? "b" : "-")
                << (connection.bipolar ? "p" : "-")
                << (connection.stereo ? "s" : "-")
                << "\n";
        }

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

        return out;
    }

    void ModuleGraph::debugPrint(const juce::String& header,
                                 const std::function<juce::String(const juce::String&)>& nodeLabelForId) const
    {
        if (header.isNotEmpty())
            DBG(header);
        DBG(toDebugString(nodeLabelForId));
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
