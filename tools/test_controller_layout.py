"""Check the shipped controller preset uses supported actions and preserves layouts."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
content = (root / 'app/Madeira/ContentView.swift').read_text(encoding='utf-8')
touch = (root / 'app/Madeira/TouchGamepad.swift').read_text(encoding='utf-8')
pure = touch[touch.index('struct GamepadSample'):touch.index('// MARK: - UIKit touch lifetime')]
method = content[content.index('    func addGamepadLayout()'):content.index('    /// ml644: does this WINDOW')]
actions = re.findall(r'\("([^"]+)", 0\.', method)
assert len(actions) == 18 and len(set(actions)) == 18
tests = '''
var controls = [TouchControl(nx: 0.7, ny: 0.3, scale: 1.2, action: .pad("LS"))]
var visible = false
var editing = true
'''+ method + '''
let original = controls[0]
addGamepadLayout()
assert(controls.count == 18 && controls[0] == original && visible && !editing)
addGamepadLayout()
assert(controls.count == 18 && controls[0] == original)
for c in controls { assert(TouchPadAction.supported(c.action.padName!)) }
print("PASS: complete controller preset, idempotence, custom layout preservation")
'''
types = '''
enum ControlAction: Equatable { case pad(String); var padName: String? { if case .pad(let n) = self { return n }; return nil } }
struct TouchControl: Equatable { var nx: Double; var ny: Double; var scale: Double; var action: ControlAction }
'''
with tempfile.TemporaryDirectory() as tmp:
    source, binary = Path(tmp) / 'main.swift', Path(tmp) / 'check'
    source.write_text('import Foundation\n' + pure + types + tests, encoding='utf-8')
    subprocess.run(['swiftc', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
