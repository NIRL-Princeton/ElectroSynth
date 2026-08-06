#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace electrosynth::effect_order {

enum class PlacementResult {
    applied,
    movedProcessorMissing,
    anchorProcessorMissing,
    identicalProcessorAndAnchor
};

template <typename Processor>
PlacementResult placeBefore(std::vector<std::unique_ptr<Processor>>& lane,
                            Processor* movedProcessor,
                            Processor* nextProcessor) noexcept {
    const auto moved = std::find_if(lane.begin(), lane.end(),
                                    [movedProcessor](const auto& processor) {
                                        return processor.get() == movedProcessor;
                                    });
    if (moved == lane.end())
        return PlacementResult::movedProcessorMissing;

    if (nextProcessor == nullptr) {
        if (std::next(moved) != lane.end())
            std::rotate(moved, std::next(moved), lane.end());
        return PlacementResult::applied;
    }

    if (nextProcessor == movedProcessor)
        return PlacementResult::identicalProcessorAndAnchor;

    const auto next = std::find_if(lane.begin(), lane.end(),
                                   [nextProcessor](const auto& processor) {
                                       return processor.get() == nextProcessor;
                                   });
    if (next == lane.end())
        return PlacementResult::anchorProcessorMissing;

    if (std::next(moved) == next)
        return PlacementResult::applied;

    if (moved < next)
        std::rotate(moved, std::next(moved), next);
    else
        std::rotate(next, moved, std::next(moved));

    return PlacementResult::applied;
}

} // namespace electrosynth::effect_order
