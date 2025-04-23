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
TEST_CASE("Thread1 receives a message from Thread2 and exits", "[threads]") {
    std::mutex mtx;
    std::condition_variable cv;
    bool message_received = false;
    std::string message;

    std::thread t1([&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return message_received; });

        // Validate message content
        REQUIRE(message == "Hello from Thread2");
    });

    std::thread t2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // simulate some work

        {
            std::lock_guard<std::mutex> lock(mtx);
            message = "Hello from Thread2";
            message_received = true;
        }

        cv.notify_one();
    });

    t1.join();
    t2.join();
}

void mycallback(double deltatime, std::vector<unsigned char> *message, void */*userData*/) {
    unsigned int nBytes = message->size();
    for (unsigned int i = 0; i < nBytes; i++)
        std::cout << "Byte " << i << " = " << (int) message->at(i) << ", ";
    if (nBytes > 0)
        std::cout << "stamp = " << deltatime << std::endl;
}

TEST_CASE("Test Midi Array Device", "[midi]") {
    auto devices = juce::MessageManager::callSync([] {
        return juce::MidiOutput::getAvailableDevices();
    });
    for (auto &device: *devices) {
        DBG(device.name);
    }
    REQUIRE(1 == 1);
}

TEST_CASE("Test Spin up", "[midi]") {
    //RtMidiIn *midiin = 0;
    std::mutex mtx;
    std::mutex mtx_;
    std::condition_variable cv;
    std::condition_variable c_;
    bool message_received = false;
    bool midi_ready = false;
    std::string message;
    // std::unique_lock<std::mutex> lock(mtx);

    // juce::MessageManager* msg = juce::MessageManager::getInstance();
    std::atomic<bool> keepRunning = true;


    std::thread midi_in_thread([&]() {
        try {
            RtMidiIn midiin; {
                std::lock_guard<std::mutex> lock(mtx_);
                //open virtualport for software interaction
                //This function creates a virtual MIDI input port to which other software applications can connect. This type of functionality is currently only supported by the Macintosh OS-X, any JACK, and Linux ALSA APIs (the function returns an error for the other APIs).
                midiin.openVirtualPort();

                // Set our callback function.  This should be done immediately after
                // opening the port to avoid having incoming messages written to the
                // queue instead of sent to the callback function.
                midiin.setCallback(&mycallback);
                // Don't ignore sysex, timing, or active sensing messages.
                midiin.ignoreTypes(false, false, false);
                midi_ready = true;
            }
            c_.notify_one();
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return message_received; });
        } catch (RtMidiError &error) {
            // Handle the exception here
            error.printMessage();
        }
    });

    juce::Thread::launch([&] {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            c_.wait(lock, [&] { return midi_ready; }); // Wait until MIDI setup is ready
        }

        juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

        message = "no device found";
        auto devices = juce::MessageManager::callSync([] {
            return juce::MidiOutput::getAvailableDevices();
        });

        for (auto &device: *devices) {
            if (device.name == "RtMidi Input") {
                message = "RtMidi found";
                message_received = true;
                break;
            }
        }
        message_received = true;
        cv.notify_one();
    });

    midi_in_thread.join();

    // Validate message content
    REQUIRE(message == "RtMidi found");
}

TEST_CASE("Test Send", "[midi]") {
    //RtMidiIn *midiin = 0;
    std::mutex mtx;
    std::mutex mtx_;
    std::condition_variable cv;
    std::condition_variable c_;
    bool message_sent = false;
    bool midi_ready = false;

    // std::unique_lock<std::mutex> lock(mtx);

    // juce::MessageManager* msg = juce::MessageManager::getInstance();
    std::atomic<bool> keepRunning = true;
    struct MidiTestContext {
        std::vector<unsigned char> lastMessage;
        std::atomic<bool> messageReceived{false};
        double timestamp = 0.0;
    };
    MidiTestContext testContext;

    std::thread midi_in_thread([&]() {
        try {
            RtMidiIn midiin; {
                std::lock_guard<std::mutex> lock(mtx_);

                midiin.openVirtualPort();

                midiin.setCallback([](double timeStamp, std::vector<unsigned char> *msg, void *userData) {
                    auto *ctx = static_cast<MidiTestContext *>(userData);

                    ctx->timestamp = timeStamp;
                    ctx->lastMessage = *msg;
                    ctx->messageReceived = true;
                }, &testContext);
                // Don't ignore sysex, timing, or active sensing messages.
                midiin.ignoreTypes(false, false, false);
                midi_ready = true;

                // [ 0x90, 0x40, 0x7F
            }
            c_.notify_one();
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return message_sent; });
            // Validate message content
        } catch (RtMidiError &error) {
            // Handle the exception here
            error.printMessage();
        }
    });

    juce::Thread::launch([&] {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            c_.wait(lock, [&] { return midi_ready; }); // Wait until MIDI setup is ready
        }

        juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

        auto devices = juce::MessageManager::callSync([] {
            return juce::MidiOutput::getAvailableDevices();
        });
        std::unique_ptr<juce::MidiOutput> midi_output;
        for (auto &device: *devices) {
            if (device.name == "RtMidi Input") {
                midi_output = juce::MidiOutput::openDevice(device.identifier);
                break;
            }
        }
        REQUIRE(midi_output != nullptr);
        if (midi_output != nullptr) {
            midi_output->sendMessageNow(juce::MidiMessage(0x90, 0x40, 0x7F));
            // Sleep to allow time for the MIDI callback to be triggered.
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        message_sent = true;
        cv.notify_one();
    });

    midi_in_thread.join();
    REQUIRE(testContext.messageReceived);
    REQUIRE(testContext.lastMessage.size() == 3);
    INFO("Timestamp: " << testContext.timestamp);
    INFO("Received MIDI message: " << std::hex
        << static_cast<int>(testContext.lastMessage[0]) << " "
        << static_cast<int>(testContext.lastMessage[1]) << " "
        << static_cast<int>(testContext.lastMessage[2]));

    REQUIRE(testContext.lastMessage[0] == 0x90);
    REQUIRE(testContext.lastMessage[1] == 0x40);
    REQUIRE(testContext.lastMessage[2] == 0x7F);
}
#include <cmath>
#define MAX_SYSEX_MSG_SIZE 64  // Maximum size of a single SysEx message chunk

TEST_CASE("Test Serialize", "[midi]") {
    //RtMidiIn *midiin = 0;
    std::mutex mtx;
    std::mutex mtx_;
    std::condition_variable cv;
    std::condition_variable c_;
    bool message_sent = false;
    bool midi_ready = false;




    // Create and initialize a tProcessorPreset instance
    leaf::tProcessorPreset originalPreset;
    originalPreset.processorTag = 42;
    originalPreset.processorTypeID = 84;
    originalPreset.processorUniqueID = 126;
    originalPreset.proc_chain = 21;
    originalPreset.index = 63;

    // Populate params with test float values
    for (int i = 0; i < MAX_NUM_PARAMS; ++i) {
        originalPreset.params[i] = static_cast<float>(i + 0.5f); // Example: 0.5, 1.5, 2.5, ...
    }


    // Reconstruct the preset from the 7-bit data
   // leaf::tProcessorPreset reconstructedPreset;
    //unsplitProcessorPreset(&preset7Bit, &reconstructedPreset);
    std::atomic<bool> keepRunning = true;
    struct MidiTestContext {
        leaf::tProcessorReceiver receiver;
        leaf::tProcessorPreset reconstructedPreset;
        std::vector<unsigned char> lastMessage;
        std::atomic<bool> presetReceived{false};
        double timestamp = 0.0;
    };
    MidiTestContext testContext;
    testContext.receiver.receivedDataSize = 0;
    std::thread midi_in_thread([&]() {
        try {
            RtMidiIn midiin; {
                std::lock_guard<std::mutex> lock(mtx_);

                midiin.openVirtualPort();

                midiin.setCallback([](double timeStamp, std::vector<unsigned char> *msg, void *userData) {
                    auto *ctx = static_cast<MidiTestContext *>(userData);
                    if (ctx->receiver.receivedDataSize + (msg->size() -3) <= sizeof(leaf::tProcessorPreset7Bit)) {
                        memcpy(ctx->receiver.receivedData + ctx->receiver.receivedDataSize, msg->data() + 2, msg->size() -3);
                        ctx->receiver.receivedDataSize += msg->size() -3;
                    }
                    if (ctx->receiver.receivedDataSize == sizeof(leaf::tProcessorPreset7Bit)) {
                        leaf::tProcessorPreset7Bit preset7Bit;
                        memcpy(&preset7Bit, ctx->receiver.receivedData, sizeof(leaf::tProcessorPreset7Bit));
                        unsplitProcessorPreset(&preset7Bit, &ctx->reconstructedPreset);
                        ctx->receiver.receivedDataSize = 0;
                    }
                    ctx->timestamp = timeStamp;
                    ctx->lastMessage = *msg;
                    ctx->presetReceived = true;
                }, &testContext);
                // Don't ignore sysex, timing, or active sensing messages.
                midiin.ignoreTypes(false, false, false);
                midi_ready = true;

                // [ 0x90, 0x40, 0x7F
            }
            c_.notify_one();
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return message_sent; });
            // Validate message content
        } catch (RtMidiError &error) {
            // Handle the exception here
            error.printMessage();
        }
    });

    juce::Thread::launch([&] {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            c_.wait(lock, [&] { return midi_ready; }); // Wait until MIDI setup is ready
        }

        juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

        auto devices = juce::MessageManager::callSync([] {
            return juce::MidiOutput::getAvailableDevices();
        });
        std::unique_ptr<juce::MidiOutput> midi_output;
        for (auto &device: *devices) {
            if (device.name == "RtMidi Input") {
                midi_output = juce::MidiOutput::openDevice(device.identifier);
                break;
            }
        }
        REQUIRE(midi_output != nullptr);
        if (midi_output != nullptr) {
            // Initialize the 7-bit struct to store the split data
            leaf::tProcessorPreset7Bit preset7Bit;
            // Split the original preset
            splitProcessorPreset(&originalPreset, &preset7Bit);
            uint16_t sizeOfSysexChunk = 64 ;
            sendPresetOverMidi(preset7Bit, sizeOfSysexChunk,midi_output.get());
            // for (auto chunk : splitPresetIntoSpans(preset7Bit, sizeOfSysexChunk)) {
            //     midi_output->sendMessageNow(juce::MidiMessage::createSysExMessage(chunk));
            // }
            // Sleep to allow time for the MIDI callback to be triggered.
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        message_sent = true;
        cv.notify_one();
    });

    midi_in_thread.join();
    REQUIRE(testContext.presetReceived);
   // REQUIRE(testContext.lastMessage.size() == 3);
    // Verify that the reconstructed preset matches the original
    REQUIRE(testContext.reconstructedPreset.processorTag == originalPreset.processorTag);
    REQUIRE(testContext.reconstructedPreset.processorTypeID == originalPreset.processorTypeID);
    REQUIRE(testContext.reconstructedPreset.processorUniqueID == originalPreset.processorUniqueID);
    REQUIRE(testContext.reconstructedPreset.proc_chain == originalPreset.proc_chain);
    REQUIRE(testContext.reconstructedPreset.index == originalPreset.index);

    // Check each parameter
    for (int i = 0; i < MAX_NUM_PARAMS; ++i) {
        REQUIRE(testContext.reconstructedPreset.params[i] == Catch::Approx(originalPreset.params[i]));
    }
}
