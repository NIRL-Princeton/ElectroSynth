//
// Created by airship on 8/16/26.
//

#ifndef ELECTORSYNTH_MODULEGRAPH_H
#define ELECTORSYNTH_MODULEGRAPH_H

#include <functional>
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
            LaneHeader,
            Modulator
        };

        struct NodeRecord
        {
            ModuleBase* module = nullptr;
            NodeKind kind = NodeKind::Unknown;
            int groupIndex = -1;
            int orderIndex = -1;
        };

        void registerNode(const juce::String& nodeId,
                          ModuleBase* module = nullptr,
                          NodeKind kind = NodeKind::Unknown,
                          int groupIndex = -1,
                          int orderIndex = -1);
        void unregisterNode(const juce::String& nodeId);
        void setNodeKind(const juce::String& nodeId, NodeKind kind);
        void setNodePlacement(const juce::String& nodeId, int groupIndex, int orderIndex);
        bool hasNode(const juce::String& nodeId) const;
        const NodeRecord* getNode(const juce::String& nodeId) const;
        std::vector<juce::String> getNodeIds(NodeKind kind, int groupIndex = -1) const;

        bool connect(const electrosynth::ConnectionRecord& connection);
        bool update(const electrosynth::ConnectionRecord& connection);
        void disconnect(const juce::String& connectionId);
        juce::String toDebugString(const std::function<juce::String(const juce::String&)>& nodeLabelForId = {}) const;
        void debugPrint(const juce::String& header = {},
                        const std::function<juce::String(const juce::String&)>& nodeLabelForId = {}) const;

        const std::vector<ConnectionRecord>& getConnections() const noexcept { return connections_; }
        std::vector<ConnectionRecord> getIncoming(const juce::String& nodeId) const;
        std::vector<ConnectionRecord> getOutgoing(const juce::String& nodeId) const;

    private:
        std::map<juce::String, NodeRecord> nodes_;
        std::vector<ConnectionRecord> connections_;
    };

}
#endif // ELECTORSYNTH_MODULEGRAPH_H
