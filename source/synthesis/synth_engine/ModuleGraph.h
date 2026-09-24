//
// Created by airship on 8/16/26.
//

#ifndef ELECTORSYNTH_MODULEGRAPH_H
#define ELECTORSYNTH_MODULEGRAPH_H

#include <map>
#include <vector>

#include "ConnectionRecord.h"

class ModuleBase;

namespace electrosynth
{
    class ModuleGraph
    {
    public:
        enum class NodeKind
        {
            Unknown,
            AudioModule,
            Modulator
        };

        struct NodeRecord
        {
            ModuleBase* module = nullptr;
            NodeKind kind = NodeKind::Unknown;
            int groupIndex = -1;
            int orderIndex = -1;
            bool inLane = false;
            bool inProcessorChain = false;
            bool terminal = false;
        };

        void registerNode(const juce::String& nodeId,
                          ModuleBase* module = nullptr,
                          NodeKind kind = NodeKind::Unknown,
                          int groupIndex = -1,
                          int orderIndex = -1,
                          bool inLane = false,
                          bool inProcessorChain = false);
        void unregisterNode(const juce::String& nodeId);
        void setNodeKind(const juce::String& nodeId, NodeKind kind);
        void setNodePlacement(const juce::String& nodeId,
                              int groupIndex,
                              int orderIndex,
                              bool inLane = false,
                              bool inProcessorChain = false);
        void setNodeTerminal(const juce::String& nodeId, bool terminal);
        bool hasNode(const juce::String& nodeId) const;
        const NodeRecord* getNode(const juce::String& nodeId) const;
        std::vector<juce::String> getNodeIds(NodeKind kind, int groupIndex = -1) const;

        bool connect(const electrosynth::ConnectionRecord& connection);
        bool update(const electrosynth::ConnectionRecord& connection);
        void disconnect(const juce::String& connectionId);
        juce::String toDebugString() const;
        void debugPrint(const juce::String& header = {}) const;

        const std::vector<ConnectionRecord>& getConnections() const noexcept { return connections_; }
        std::vector<ConnectionRecord> getIncoming(const juce::String& nodeId) const;
        std::vector<ConnectionRecord> getOutgoing(const juce::String& nodeId) const;

    private:
        std::map<juce::String, NodeRecord> nodes_;
        std::vector<ConnectionRecord> connections_;
    };

}
#endif // ELECTORSYNTH_MODULEGRAPH_H
