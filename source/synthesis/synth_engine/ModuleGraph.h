//
// Created by airship on 8/16/26.
//

#ifndef ELECTORSYNTH_MODULEGRAPH_H
#define ELECTORSYNTH_MODULEGRAPH_H

#include <functional>
#include <vector>

#include "ConnectionRecord.h"

namespace electrosynth
{
    class ModuleGraph
    {
    public:
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
        std::vector<ConnectionRecord> connections_;
    };

}
#endif // ELECTORSYNTH_MODULEGRAPH_H
