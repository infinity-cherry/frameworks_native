/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <InputListener.h>
#include <android-base/result.h>
#include <android/gui/FocusRequest.h>

#include <android/os/InputEventInjectionResult.h>
#include <android/os/InputEventInjectionSync.h>
#include <gui/InputApplication.h>
#include <gui/WindowInfo.h>
#include <input/InputDevice.h>
#include <input/InputTransport.h>
#include <unordered_map>

namespace android {

/* Notifies the system about input events generated by the input reader.
 * The dispatcher is expected to be mostly asynchronous. */
class InputDispatcherInterface : public InputListenerInterface {
public:
    InputDispatcherInterface() {}
    virtual ~InputDispatcherInterface() {}
    /* Dumps the state of the input dispatcher.
     *
     * This method may be called on any thread (usually by the input manager). */
    virtual void dump(std::string& dump) = 0;

    /* Called by the heatbeat to ensures that the dispatcher has not deadlocked. */
    virtual void monitor() = 0;

    /**
     * Wait until dispatcher is idle. That means, there are no further events to be processed,
     * and all of the policy callbacks have been completed.
     * Return true if the dispatcher is idle.
     * Return false if the timeout waiting for the dispatcher to become idle has expired.
     */
    virtual bool waitForIdle() = 0;

    /* Make the dispatcher start processing events.
     *
     * The dispatcher will start consuming events from the InputListenerInterface
     * in the order that they were received.
     */
    virtual status_t start() = 0;

    /* Makes the dispatcher stop processing events. */
    virtual status_t stop() = 0;

    /* Injects an input event and optionally waits for sync.
     * The synchronization mode determines whether the method blocks while waiting for
     * input injection to proceed.
     * Returns one of the INPUT_EVENT_INJECTION_XXX constants.
     *
     * If a targetUid is provided, InputDispatcher will only consider injecting the input event into
     * windows owned by the provided uid. If the input event is targeted at a window that is not
     * owned by the provided uid, input injection will fail. If no targetUid is provided, the input
     * event will be dispatched as-is.
     *
     * This method may be called on any thread (usually by the input manager). The caller must
     * perform all necessary permission checks prior to injecting events.
     */
    virtual android::os::InputEventInjectionResult injectInputEvent(
            const InputEvent* event, std::optional<gui::Uid> targetUid,
            android::os::InputEventInjectionSync syncMode, std::chrono::milliseconds timeout,
            uint32_t policyFlags) = 0;

    /*
     * Check whether InputEvent actually happened by checking the signature of the event.
     *
     * Return nullptr if the event cannot be verified.
     */
    virtual std::unique_ptr<VerifiedInputEvent> verifyInputEvent(const InputEvent& event) = 0;

    /* Sets the list of input windows per display.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual void setInputWindows(
            const std::unordered_map<int32_t, std::vector<sp<gui::WindowInfoHandle>>>&
                    handlesPerDisplay) = 0;

    /* Sets the focused application on the given display.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual void setFocusedApplication(
            int32_t displayId,
            const std::shared_ptr<InputApplicationHandle>& inputApplicationHandle) = 0;

    /* Sets the focused display.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual void setFocusedDisplay(int32_t displayId) = 0;

    /* Sets the input dispatching mode.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual void setInputDispatchMode(bool enabled, bool frozen) = 0;

    /* Sets whether input event filtering is enabled.
     * When enabled, incoming input events are sent to the policy's filterInputEvent
     * method instead of being dispatched.  The filter is expected to use
     * injectInputEvent to inject the events it would like to have dispatched.
     * It should include POLICY_FLAG_FILTERED in the policy flags during injection.
     */
    virtual void setInputFilterEnabled(bool enabled) = 0;

    /**
     * Set the touch mode state.
     * Touch mode is a per display state that apps may enter / exit based on specific user
     * interactions with input devices. If <code>inTouchMode</code> is set to true, the display
     * identified by <code>displayId</code> will be changed to touch mode. Performs a permission
     * check if hasPermission is set to false.
     *
     * This method also enqueues a a TouchModeEntry message for dispatching.
     *
     * Returns true when changing touch mode state.
     */
    virtual bool setInTouchMode(bool inTouchMode, gui::Pid pid, gui::Uid uid, bool hasPermission,
                                int32_t displayId) = 0;

    /**
     * Sets the maximum allowed obscuring opacity by UID to propagate touches.
     * For certain window types (eg. SAWs), the decision of honoring
     * FLAG_NOT_TOUCHABLE or not depends on the combined obscuring opacity of
     * the windows above the touch-consuming window.
     */
    virtual void setMaximumObscuringOpacityForTouch(float opacity) = 0;

    /* Transfers touch focus from one window to another window.
     *
     * Returns true on success.  False if the window did not actually have touch focus.
     */
    virtual bool transferTouchFocus(const sp<IBinder>& fromToken, const sp<IBinder>& toToken,
                                    bool isDragDrop) = 0;

    /**
     * Transfer touch focus to the provided channel, no matter where the current touch is.
     *
     * Return true on success, false if there was no on-going touch.
     */
    virtual bool transferTouch(const sp<IBinder>& destChannelToken, int32_t displayId) = 0;

    /**
     * Sets focus on the specified window.
     */
    virtual void setFocusedWindow(const gui::FocusRequest&) = 0;

    /**
     * Creates an input channel that may be used as targets for input events.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual base::Result<std::unique_ptr<InputChannel>> createInputChannel(
            const std::string& name) = 0;

    /**
     * Creates an input channel to be used to monitor all input events on a display.
     *
     * Each monitor must target a specific display and will only receive input events sent to that
     * display.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual base::Result<std::unique_ptr<InputChannel>> createInputMonitor(int32_t displayId,
                                                                           const std::string& name,
                                                                           gui::Pid pid) = 0;

    /* Removes input channels that will no longer receive input events.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual status_t removeInputChannel(const sp<IBinder>& connectionToken) = 0;

    /* Allows an input monitor steal the current pointer stream away from normal input windows.
     *
     * This method may be called on any thread (usually by the input manager).
     */
    virtual status_t pilferPointers(const sp<IBinder>& token) = 0;

    /**
     * Enables Pointer Capture on the specified window if the window has focus.
     *
     * InputDispatcher is the source of truth of Pointer Capture.
     */
    virtual void requestPointerCapture(const sp<IBinder>& windowToken, bool enabled) = 0;

    /**
     * Sets the eligibility of a given display to enable pointer capture. If a display is marked
     * ineligible, all attempts to request pointer capture for windows on that display will fail.
     *  TODO(b/214621487): Remove or move to a display flag.
     */
    virtual void setDisplayEligibilityForPointerCapture(int displayId, bool isEligible) = 0;

    /* Flush input device motion sensor.
     *
     * Returns true on success.
     */
    virtual bool flushSensor(int deviceId, InputDeviceSensorType sensorType) = 0;

    /**
     * Called when a display has been removed from the system.
     */
    virtual void displayRemoved(int32_t displayId) = 0;

    /*
     * Abort the current touch stream.
     */
    virtual void cancelCurrentTouch() = 0;

    /*
     * Updates key repeat configuration timeout and delay.
     */
    virtual void setKeyRepeatConfiguration(nsecs_t timeout, nsecs_t delay) = 0;
};

} // namespace android
