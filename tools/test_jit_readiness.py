"""Exercise the production readiness state with Swift on the CI Mac."""
from pathlib import Path
import subprocess
import tempfile

source = Path("app/Madeira/StikJITHelper.swift").read_text()
state = source.split("// BEGIN JIT READINESS STATE", 1)[1].split("// END JIT READINESS STATE", 1)[0]
test = r"""
let state = MadeiraJITReadiness()
precondition(!state.isReady(debuggerAttached: false))
precondition(state.isReady(debuggerAttached: true))
// Attachment alone does not prove that allocation succeeded.
precondition(!state.hasPreparedPool())
precondition(!state.isReady(debuggerAttached: false))
state.recordPreparedPool()
// Intentional early detach must not turn the running session's badge off.
precondition(state.isReady(debuggerAttached: false))
precondition(state.isReady(debuggerAttached: true))
// Readiness is process local, never persisted across a fresh launch.
precondition(!MadeiraJITReadiness().isReady(debuggerAttached: false))
DispatchQueue.concurrentPerform(iterations: 1000) { _ in
    state.recordPreparedPool()
    precondition(state.isReady(debuggerAttached: false))
}
print("JIT readiness: allocation, detach, fresh launch and concurrent reads passed")
"""
with tempfile.TemporaryDirectory() as directory:
    program = Path(directory) / "readiness.swift"
    program.write_text("import Foundation\n" + state + test)
    subprocess.run(["swift", str(program)], check=True)
