//
// Created by Callista Chong on 7/24/26.
//

#pragma once
#include "AudioNode.h"

namespace electrosynth::audio {
    struct AudioConnection {
        AudioPortAddress source;
        AudioPortAddress destination;

        bool isValid() const noexcept {
            return source.isValid() && destination.isValid() && source.direction == PortDirection::Output
                && destination.direction == PortDirection::Input && source.domain == destination.domain
                && source.nodeId != destination.nodeId;
        }
    };

}
