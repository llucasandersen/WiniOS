import SwiftUI

@main
struct MadeiraApp: App {
    @State private var prefixReady = false
    @State private var preparingPrefix = false
    @State private var prefixFailed = false

    var body: some Scene {
        WindowGroup {
            Group {
                if prefixReady {
                    ContentView()
                        .modifier(ClaimGamepadEvents())
                } else if prefixFailed {
                    VStack(spacing: 16) {
                        Text("Wine C: drive setup did not finish.")
                        Button("Retry Setup") {
                            Task { await prepareWinePrefix() }
                        }
                    }
                } else {
                    ProgressView("Preparing Wine C: drive...")
                }
            }
            .task {
                await prepareWinePrefix()
            }
        }
    }

    @MainActor
    private func prepareWinePrefix() async {
        guard !prefixReady && !preparingPrefix else { return }
        preparingPrefix = true
        prefixFailed = false
        let ready = await Task.detached(priority: .userInitiated) { () -> Bool in
            let documents = FileManager.default.urls(for: .documentDirectory,
                                                      in: .userDomainMask)[0]
            let prefix = documents.appendingPathComponent("wine").path
            prefix.withCString { madeira_seed_prefix_if_needed($0) }
            return FileManager.default.fileExists(atPath: prefix + "/.update-timestamp")
        }.value
        preparingPrefix = false
        if ready {
            prefixReady = true
            GamepadInput.shared.start()
        } else {
            prefixFailed = true
        }
    }
}
