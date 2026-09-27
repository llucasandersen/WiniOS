import SwiftUI

@main
struct MadeiraApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
                .modifier(ClaimGamepadEvents())
                .onAppear {
                    let documents = FileManager.default.urls(for: .documentDirectory,
                                                              in: .userDomainMask)[0]
                    let prefix = documents.appendingPathComponent("wine").path
                    prefix.withCString { madeira_seed_prefix_if_needed($0) }
                    GamepadInput.shared.start()
                }
        }
    }
}
