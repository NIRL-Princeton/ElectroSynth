//
// Created by Davis Polito on 2/13/25.
//
#include "leaf.h"
#include "mapping.h"
#include "RtMidi.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include <catch2/catch_test_macros.hpp>

#include "chowdsp_core/third_party/span-lite/test/lest/lest_cpp03.hpp"
#include "juce_events/juce_events.h"
#include "juce_gui_extra/misc/juce_PushNotifications.h"
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <iostream>
#include <catch2/catch_approx.hpp>
/////use std::cout when writing tests if you want output when the test is successful
#include "midi_manager.h"

#include <cmath>
#define MAX_SYSEX_MSG_SIZE 64  // Maximum size of a single SysEx message chunk

TEST_CASE("Test send fake midi sysex", "[midi]") {




    // Create and initialize a tProcessorPreset instance
    leaf::tProcessorPreset originalPreset;

    originalPreset.processorTypeID = 84;
    originalPreset.processorUniqueID = 126;
    originalPreset.proc_chain = 21;
    originalPreset.index = 63;

    // Populate params with test float values
    for (int i = 0; i < MAX_NUM_PARAMS; ++i) {
        originalPreset.params[i] = static_cast<float>(i + 0.5f); // Example: 0.5, 1.5, 2.5, ...
    }







        juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

        auto devices = juce::MessageManager::callSync([] {
            return juce::MidiOutput::getAvailableDevices();
        });
        std::unique_ptr<juce::MidiOutput> midi_output;
        for (auto &device: *devices) {
            std::cout<< device.name << std::endl;
            if (device.name == "Electrosteel") {
                midi_output = juce::MidiOutput::openDevice(device.identifier);
                break;
            }
        }

        if (midi_output != nullptr) {
            // Initialize the 7-bit struct to store the split data
            std::array<std::byte,62> buffer;
            for(uint8_t i = 0; i< 62; i++)
            {
                buffer[i] = static_cast<std::byte>(i);


            }

            midi_output->sendMessageNow(juce::MidiMessage::createSysExMessage(buffer.data(), buffer.size()));


        }




   REQUIRE(1==1);
}
