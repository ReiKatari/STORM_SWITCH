// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import org.yuzu.yuzu_emu.YuzuApplication
import org.yuzu.yuzu_emu.features.input.NativeInput
import org.yuzu.yuzu_emu.features.input.YuzuInputOverlayDevice
import org.yuzu.yuzu_emu.features.input.YuzuPhysicalDevice
import java.util.Locale

object InputHandler {
    var androidControllers = mapOf<Int, YuzuPhysicalDevice>()
    var registeredControllers = mutableListOf<ParamPackage>()

    private const val PREFS_CONTROLLER_SLOTS = "storm_controller_slots"

    private val controllerButtons = intArrayOf(
        KeyEvent.KEYCODE_BUTTON_A,
        KeyEvent.KEYCODE_BUTTON_B,
        KeyEvent.KEYCODE_BUTTON_X,
        KeyEvent.KEYCODE_BUTTON_Y,
        KeyEvent.KEYCODE_BUTTON_L1,
        KeyEvent.KEYCODE_BUTTON_R1,
        KeyEvent.KEYCODE_BUTTON_L2,
        KeyEvent.KEYCODE_BUTTON_R2,
        KeyEvent.KEYCODE_BUTTON_THUMBL,
        KeyEvent.KEYCODE_BUTTON_THUMBR,
        KeyEvent.KEYCODE_BUTTON_START,
        KeyEvent.KEYCODE_BUTTON_SELECT,
        KeyEvent.KEYCODE_DPAD_UP,
        KeyEvent.KEYCODE_DPAD_DOWN,
        KeyEvent.KEYCODE_DPAD_LEFT,
        KeyEvent.KEYCODE_DPAD_RIGHT
    )

    private val controllerAxes = intArrayOf(
        MotionEvent.AXIS_X,
        MotionEvent.AXIS_Y,
        MotionEvent.AXIS_Z,
        MotionEvent.AXIS_RX,
        MotionEvent.AXIS_RY,
        MotionEvent.AXIS_RZ,
        MotionEvent.AXIS_HAT_X,
        MotionEvent.AXIS_HAT_Y,
        MotionEvent.AXIS_LTRIGGER,
        MotionEvent.AXIS_RTRIGGER
    )

    // Currently, Android doesn't support Joy-Con D-pad buttons. We fall back to the scan code
    private const val LINUX_BUTTON_DPAD_UP = 0x220
    private const val LINUX_BUTTON_DPAD_DOWN = 0x221
    private const val LINUX_BUTTON_DPAD_LEFT = 0x222
    private const val LINUX_BUTTON_DPAD_RIGHT = 0x223

    fun isPhysicalGameController(device: InputDevice?): Boolean {
        device ?: return false

        if (device.isVirtual) {
            return false
        }

        val sources = device.sources
        val hasControllerSource =
            sources and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
                sources and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
        if (!hasControllerSource) {
            return false
        }

        val hasControllerButtons = device.hasKeys(*controllerButtons).any { it }
        val hasControllerAxes = device.motionRanges.any { range ->
            controllerAxes.contains(range.axis)
        }
        return hasControllerButtons || hasControllerAxes
    }

    fun InputDevice.getPersistentDescriptor(): String {
        val desc = descriptor?.trim() ?: ""
        if (desc.isNotEmpty()) {
            return desc
        }
        return String.format(Locale.ROOT, "%04x:%04x:%s", vendorId, productId, name ?: "Gamepad")
    }

    fun getControllerForDevice(device: InputDevice?): YuzuPhysicalDevice? {
        device ?: return null
        return androidControllers[device.id]
            ?: androidControllers[device.controllerNumber]
            ?: androidControllers.values.firstOrNull()
    }

    fun dispatchKeyEvent(event: KeyEvent): Boolean {
        val action = when (event.action) {
            KeyEvent.ACTION_DOWN -> NativeInput.ButtonState.PRESSED
            KeyEvent.ACTION_UP -> NativeInput.ButtonState.RELEASED
            else -> return false
        }

        var controllerData = getControllerForDevice(event.device)
        if (controllerData == null) {
            updateControllerData()
            controllerData = getControllerForDevice(event.device) ?: return false
        }

        NativeInput.onGamePadButtonEvent(
            controllerData.getGUID(),
            controllerData.getPort(),
            getButtonIdFromEvent(event),
            action
        )
        return true
    }

    fun getButtonIdFromEvent(event: KeyEvent): Int {
        if (event.keyCode == 0) {
            return when (event.scanCode) {
                LINUX_BUTTON_DPAD_UP -> KeyEvent.KEYCODE_DPAD_UP
                LINUX_BUTTON_DPAD_DOWN -> KeyEvent.KEYCODE_DPAD_DOWN
                LINUX_BUTTON_DPAD_LEFT -> KeyEvent.KEYCODE_DPAD_LEFT
                LINUX_BUTTON_DPAD_RIGHT -> KeyEvent.KEYCODE_DPAD_RIGHT
                else -> return 0
            }
        }
        return event.keyCode
    }

    fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
        val device = event.device
        var controllerData = getControllerForDevice(device)
        if (controllerData == null) {
            updateControllerData()
            controllerData = getControllerForDevice(device) ?: return false
        }
        device?.motionRanges?.forEach {
            NativeInput.onGamePadAxisEvent(
                controllerData.getGUID(),
                controllerData.getPort(),
                it.axis,
                event.getAxisValue(it.axis)
            )
        }
        return true
    }

    fun getDevices(): Map<Int, YuzuPhysicalDevice> {
        val resultControllers = mutableMapOf<Int, YuzuPhysicalDevice>()
        val deviceIds = InputDevice.getDeviceIds()
        val physicalDevices = deviceIds.toList().mapNotNull { InputDevice.getDevice(it) }
            .filter { isPhysicalGameController(it) }
            .distinctBy { it.id }

        if (physicalDevices.isEmpty()) {
            return emptyMap()
        }

        val prefs = YuzuApplication.appContext.getSharedPreferences(PREFS_CONTROLLER_SLOTS, Context.MODE_PRIVATE)
        val inputSettings = NativeConfig.getInputSettings(true)

        // Track slots claimed in this session pass (0..7 for Player 1..8)
        val assignedSlots = mutableMapOf<InputDevice, Int>()
        val usedSlots = mutableSetOf<Int>()

        // 1. First pass: restore previously assigned slots for controllers that reconnect
        for (device in physicalDevices) {
            val key = device.getPersistentDescriptor()
            val savedSlot = prefs.getInt(key, -1)
            if (savedSlot in 0..7 && !usedSlots.contains(savedSlot)) {
                assignedSlots[device] = savedSlot
                usedSlots.add(savedSlot)
            }
        }

        // 2. Second pass: assign lowest available free slot for any new controllers
        val editor = prefs.edit()
        for (device in physicalDevices) {
            if (!assignedSlots.containsKey(device)) {
                var freeSlot = 0
                while (freeSlot < 8 && usedSlots.contains(freeSlot)) {
                    freeSlot++
                }
                if (freeSlot >= 8) {
                    freeSlot = 7 // Fallback cap at Player 8
                }
                assignedSlots[device] = freeSlot
                usedSlots.add(freeSlot)
                editor.putInt(device.getPersistentDescriptor(), freeSlot)
            }
        }
        editor.apply()

        // 3. Build physical devices and index by both device.id and controllerNumber
        for ((device, slot) in assignedSlots) {
            val useSystemVibrator = if (slot < inputSettings.size) inputSettings[slot].useSystemVibrator else false
            val physicalDevice = YuzuPhysicalDevice(device, slot, useSystemVibrator)
            // Index by runtime device ID (unique per hardware device at runtime)
            resultControllers[device.id] = physicalDevice
            // Also index by controllerNumber if not conflicting
            if (!resultControllers.containsKey(device.controllerNumber)) {
                resultControllers[device.controllerNumber] = physicalDevice
            }
        }

        return resultControllers
    }

    fun updateControllerData() {
        androidControllers = getDevices()

        // Register distinct physical devices by assigned port
        val uniqueControllers = androidControllers.values.distinctBy { it.getPort() }
        uniqueControllers.forEach {
            NativeInput.registerController(it)
        }

        // Register the input overlay on a dedicated port for all player 1 vibrations
        NativeInput.registerController(YuzuInputOverlayDevice(androidControllers.isEmpty(), 100))

        registeredControllers.clear()
        NativeInput.getInputDevices().forEach {
            registeredControllers.add(ParamPackage(it))
        }
        registeredControllers.sortBy { it.get("port", 0) }

        // Ensure connected state and default button mappings for all active controllers
        ensureControllersConnectedAndMapped(uniqueControllers)
    }

    private fun ensureControllersConnectedAndMapped(activeControllers: List<YuzuPhysicalDevice>) {
        if (activeControllers.isEmpty()) return

        var needSave = false
        val inputSettings = NativeConfig.getInputSettings(true)

        for (controller in activeControllers) {
            val port = controller.getPort()
            if (port in 0..7) {
                // Ensure the slot is marked as connected in the emulator
                NativeInput.connectControllers(port, true)

                // If this player slot has no button mappings configured, auto-map with defaults
                val hasMapping = if (port < inputSettings.size) inputSettings[port].hasMapping() else false
                if (!hasMapping) {
                    val matchingParam = registeredControllers.firstOrNull { it.get("port", -1) == port }
                    if (matchingParam != null) {
                        val displayName = matchingParam.get("display", controller.getName())
                        NativeInput.updateMappingsWithDefault(port, matchingParam, displayName)
                        needSave = true
                    }
                }
            }
        }

        if (needSave) {
            NativeConfig.saveGlobalConfig()
        }

        try {
            NativeInput.reloadInputDevices()
        } catch (_: Throwable) {}
    }

    fun InputDevice.getGUID(): String = String.format("%016x%016x", productId, vendorId)
}
