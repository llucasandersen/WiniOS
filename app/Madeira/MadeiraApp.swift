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
            let fm = FileManager.default
            let documents = fm.urls(for: .documentDirectory,
                                    in: .userDomainMask)[0]
            let prefix = documents.appendingPathComponent("wine").path
            prefix.withCString { madeira_seed_prefix_if_needed($0) }
            guard fm.fileExists(atPath: prefix + "/.update-timestamp"),
                  fm.fileExists(atPath: prefix + "/drive_c/windows") else { return false }

            // A locally signed IPA can carry a Steam payload separately from
            // the standard Wine template. Import it once, even when the user
            // already has an existing Wine C: drive.
            if let steamArchive = Bundle.main.path(forResource: "steam-preload", ofType: "tar.gz") {
                let marker = documents.appendingPathComponent("wine/.steam-preload-complete")
                if !fm.fileExists(atPath: marker.path) {
                    let installed = steamArchive.withCString { archive in
                        prefix.withCString { destination in
                            madeira_extract_prefix_tgz(archive, destination) == 0
                        }
                    }
                    guard installed else { return false }
                    do {
                        try Data("Steam payload extracted\n".utf8).write(to: marker, options: .atomic)
                    } catch {
                        return false
                    }
                }
            }
            return true
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
