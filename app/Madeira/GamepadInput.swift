import Foundation
// GameController supports background handler queues but lacks Sendable annotations.
@preconcurrency import GameController
import UIKit
import SwiftUI
import Combine

/// ml1930: physical and touch controller snapshots share the same serial publisher.
/// Slot/profile/timer state belongs exclusively to `queue`. The app lifecycle
/// and observer registration belong to the main actor. Guest readers use the
/// C snapshot lock; no Swift objects cross into Wine.
final class GamepadInput: @unchecked Sendable {
    static let shared = GamepadInput()
    @MainActor static let enabled: Bool = {
        let value = MadeiraConfig.get("env.MADEIRA_XINPUT")
            ?? ProcessInfo.processInfo.environment["MADEIRA_XINPUT"]
        return value != "0"
    }()

    @MainActor static let touchEnabled: Bool = {
        let value = MadeiraConfig.get("env.MADEIRA_TOUCH_XINPUT")
            ?? ProcessInfo.processInfo.environment["MADEIRA_TOUCH_XINPUT"]
        return enabled && value != "0"
    }()

    @MainActor func configureTouch(controls: Set<UUID>) {
        let allowed = Self.touchEnabled ? controls : []
        queue.async { [self] in touchState.configure(allowed); sample() }
    }

    @MainActor func touch(owner: UUID, control: UUID, value: GamepadSample?) {
        guard Self.touchEnabled else { return }
        queue.async { [self] in
            guard active || value == nil else { return }
            touchState.update(owner: owner, control: control, value: value)
            sample()
        }
    }

    private let queue = DispatchQueue(label: "madeira.gamepad", qos: .userInteractive)
    private var controllers = [GCController?](repeating: nil, count: 4)
    private var profiles = [GCPhysicalInputProfile?](repeating: nil, count: 4)
    private var lastLogged = [GamepadSample?](repeating: nil, count: 4)
    private var inputLogs = [Int](repeating: 0, count: 4)
    private var timer: DispatchSourceTimer?
    private var active = false
    private var touchState = TouchGamepadState()
    @MainActor private var observers: [NSObjectProtocol] = []
    @MainActor private var started = false

    @MainActor func start() {
        guard !started else { return }
        started = true
        LogStore.shared.log("[xinput] ml1920 physical controllers enabled=\(Self.enabled ? 1 : 0)")
        LogStore.shared.log("[touch-xinput] ml1930 enabled=\(Self.touchEnabled ? 1 : 0)")
        guard Self.enabled else { return }
        // Publish the virtual identity before a game first enumerates XInput.
        // Hiding the overlay releases input without unplugging the device.
        configureTouch(controls: Set(TouchControlsModel.shared.controls.filter {
            $0.action.padName.map(TouchPadAction.supported) ?? false
        }.map(\.id)))
        let center = NotificationCenter.default
        for name in [Notification.Name.GCControllerDidConnect, .GCControllerDidDisconnect] {
            observers.append(center.addObserver(forName: name, object: nil, queue: .main) { [weak self] _ in
                MainActor.assumeIsolated { self?.refreshControllers() }
            })
        }
        for name in [UIApplication.willResignActiveNotification, UIApplication.didEnterBackgroundNotification] {
            observers.append(center.addObserver(forName: name, object: nil, queue: .main) { [weak self] _ in
                self?.setActive(false)
            })
        }
        observers.append(center.addObserver(forName: UIApplication.didBecomeActiveNotification,
                                            object: nil, queue: .main) { [weak self] _ in
            MainActor.assumeIsolated { self?.refreshControllers() }
            self?.setActive(true)
        })
        refreshControllers()
        setActive(UIApplication.shared.applicationState == .active)
    }

    @MainActor func refreshControllers() {
        // Capture the live profile on main. Re-fetching extendedGamepad on the
        // polling queue yielded stale axes on devices tested in the fork.
        let detected = GCController.controllers()
        LogStore.shared.log("[xinput] iOS controllers=\(detected.count)")
        let live = detected.map { controller -> (GCController, GCPhysicalInputProfile) in
            // Some third-party modes expose named elements without the
            // extended convenience profile. Do not silently discard them.
            let profile = (controller.extendedGamepad as GCPhysicalInputProfile?) ?? controller.physicalInputProfile
            controller.handlerQueue = queue
            LogStore.shared.log("[xinput] detected \(controller.vendorName ?? "Controller") extended=\(controller.extendedGamepad != nil) buttons=\(profile.buttons.keys.sorted()) dpads=\(profile.dpads.keys.sorted())")
            return (controller, profile)
        }
        queue.async { [self] in
            for i in controllers.indices {
                guard let old = controllers[i], !live.contains(where: { $0.0 === old }) else { continue }
                (profiles[i] as? GCExtendedGamepad)?.valueChangedHandler = nil
                controllers[i] = nil
                profiles[i] = nil
                fputs("[xinput] ml1920 slot=\(i) disconnected\n", stderr)
            }
            for (controller, profile) in live {
                guard !controllers.contains(where: { $0 === controller }),
                      let i = controllers.firstIndex(where: { $0 == nil }) else { continue }
                controllers[i] = controller
                profiles[i] = profile
                (profile as? GCExtendedGamepad)?.valueChangedHandler = { [weak self] _, _ in
                    // Explicit queue hop also serializes callbacks already in flight
                    // when a controller is disconnected or the app resigns active.
                    self?.queue.async { [weak self] in self?.sample() }
                }
                inputLogs[i] = 0
                lastLogged[i] = nil
                fputs("[xinput] ml1920 slot=\(i) connected\n", stderr)
            }
            updateTimer()
            sample()
        }
    }

    private func setActive(_ value: Bool) {
        queue.async { [self] in
            active = value
            fputs("[xinput] active=\(value ? 1 : 0)\n", stderr)
            if !value { touchState.clear() }
            updateTimer()
            sample()
        }
    }

    private func updateTimer() {
        let needed = active && profiles.contains(where: { $0 != nil })
        if !needed { timer?.cancel(); timer = nil; return }
        guard timer == nil else { return }
        let source = DispatchSource.makeTimerSource(queue: queue)
        source.schedule(deadline: .now(), repeating: .milliseconds(4), leeway: .milliseconds(1))
        source.setEventHandler { [weak self] in self?.sample() }
        timer = source
        source.resume()
    }

    // XInput leaves dead zones to the game. Preserve the complete signed range.
    static func axis(_ value: Float) -> Int16 {
        guard value.isFinite else { return 0 }
        let clamped = max(-1, min(1, value))
        return Int16((clamped * (clamped < 0 ? 32768 : 32767)).rounded())
    }
    static func trigger(_ value: Float) -> UInt8 {
        guard value.isFinite else { return 0 }
        return UInt8((max(0, min(1, value)) * 255).rounded())
    }

    private func sample() {
        for i in profiles.indices {
            let pad = profiles[i]
            let touchConnected = i == 0 && touchState.connected
            guard pad != nil || touchConnected else {
                winios_gamepad_set_state(Int32(i), nil)
                continue
            }
            var state = winios_gamepad()
            state.connected = 1
            // Keep the connected identity, but release all controls while the
            // app is inactive. A delayed callback cannot republish a held key.
            if active, let pad {
                let buttons: [(GCControllerButtonInput?, UInt16)] = [
                    (pad.dpads[GCInputDirectionPad]?.up, 0x0001), (pad.dpads[GCInputDirectionPad]?.down, 0x0002),
                    (pad.dpads[GCInputDirectionPad]?.left, 0x0004), (pad.dpads[GCInputDirectionPad]?.right, 0x0008),
                    (pad.buttons[GCInputButtonMenu], 0x0010), (pad.buttons[GCInputButtonOptions], 0x0020),
                    (pad.buttons[GCInputLeftThumbstickButton], 0x0040), (pad.buttons[GCInputRightThumbstickButton], 0x0080),
                    (pad.buttons[GCInputLeftShoulder], 0x0100), (pad.buttons[GCInputRightShoulder], 0x0200),
                    (pad.buttons[GCInputButtonHome], 0x0400), (pad.buttons[GCInputButtonA], 0x1000),
                    (pad.buttons[GCInputButtonB], 0x2000), (pad.buttons[GCInputButtonX], 0x4000), (pad.buttons[GCInputButtonY], 0x8000)
                ]
                for (button, mask) in buttons where button?.isPressed == true { state.buttons |= mask }
                state.left_trigger = Self.trigger(pad.buttons[GCInputLeftTrigger]?.value ?? 0)
                state.right_trigger = Self.trigger(pad.buttons[GCInputRightTrigger]?.value ?? 0)
                state.lx = Self.axis(pad.dpads[GCInputLeftThumbstick]?.xAxis.value ?? 0)
                state.ly = Self.axis(pad.dpads[GCInputLeftThumbstick]?.yAxis.value ?? 0)
                state.rx = Self.axis(pad.dpads[GCInputRightThumbstick]?.xAxis.value ?? 0)
                state.ry = Self.axis(pad.dpads[GCInputRightThumbstick]?.yAxis.value ?? 0)
            }
            if active && touchConnected {
                let physical = GamepadSample(buttons: state.buttons,
                    lt: state.left_trigger, rt: state.right_trigger,
                    lx: state.lx, ly: state.ly, rx: state.rx, ry: state.ry)
                let merged = GamepadSample.merge(physical: physical, touch: touchState.sample)
                state.buttons = merged.buttons
                state.left_trigger = merged.lt; state.right_trigger = merged.rt
                state.lx = merged.lx; state.ly = merged.ly; state.rx = merged.rx; state.ry = merged.ry
            }
            winios_gamepad_set_state(Int32(i), &state)
            let published = GamepadSample(buttons: state.buttons, lt: state.left_trigger,
                rt: state.right_trigger, lx: state.lx, ly: state.ly, rx: state.rx, ry: state.ry)
            if lastLogged[i] != published && inputLogs[i] < 16 {
                fputs("[xinput] publish slot=\(i) physical=\(pad != nil) touch=\(touchConnected) buttons=\(state.buttons) LS=\(state.lx),\(state.ly) RS=\(state.rx),\(state.ry) triggers=\(state.left_trigger),\(state.right_trigger)\n", stderr)
                inputLogs[i] += 1
            }
            lastLogged[i] = published
        }
    }
}

/// Read the same C snapshot the Wine bridge consumes, only while Settings is open.
struct ControllerInputMonitor: View {
    @State private var readings = ""
    private let tick = Timer.publish(every: 0.1, on: .main, in: .common).autoconnect()
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(readings).font(.system(.caption, design: .monospaced))
            Button("Refresh connected controllers") { GamepadInput.shared.refreshControllers() }
            Text("Press buttons and move sticks. Changing values confirm Madeira receives input. If no physical controller appears, check its model's Apple pairing mode.")
                .font(.footnote).foregroundStyle(.secondary)
        }
        .onReceive(tick) { _ in
            readings = (0..<4).map { i in
                var pad = winios_gamepad()
                guard winios_gamepad_get_state(Int32(i), &pad) != 0 else { return "Player \(i + 1): disconnected" }
                return "Player \(i + 1): buttons \(String(format: "%04X", Int(pad.buttons)))\nLS \(pad.lx),\(pad.ly) RS \(pad.rx),\(pad.ry) LT \(pad.left_trigger) RT \(pad.right_trigger) packet \(pad.packet)"
            }.joined(separator: "\n")
        }
        .modifier(ClaimGamepadEvents())
    }
}

/// iOS 18 otherwise routes stick input into UIKit/SwiftUI focus navigation.
struct ClaimGamepadEvents: ViewModifier {
    func body(content: Content) -> some View {
        if #available(iOS 18.0, *), GamepadInput.enabled {
            content.handlesGameControllerEvents(matching: .gamepad)
        } else { content }
    }
}

enum GamepadEventClaim {
    @MainActor static func install(on view: UIView) {
        guard GamepadInput.enabled else { return }
        if #available(iOS 18.0, *) {
            guard !view.interactions.contains(where: { $0 is GCEventInteraction }) else { return }
            let interaction = GCEventInteraction()
            interaction.handledEventTypes = .gamepad
            view.addInteraction(interaction)
        }
    }
}
