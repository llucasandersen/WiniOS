from pathlib import Path
import shutil

module = Path("FEX/Source/Windows/ARM64EC/Module.cpp")
text = module.read_text(encoding="utf-8")
needle = 'extern "C" void NotifyImageMap(void* Address) {\n  if (!InvalidationTracker || !Address) {\n    return;\n  }\n'
assert needle in text, "Pinned loader hook changed; inspect before patching"
text = text.replace(needle, needle + '\n  FexLoaderCallbackGuard CallbackGuard(GetCPUArea().Area);\n', 1)
text = text.replace('#include "BTInterface.h"', '#include "BTInterface.h"\n#include "FexLoaderCallbackGuard.h"', 1)
module.write_text(text, encoding="utf-8")
shutil.copyfile(".github/FexLoaderCallbackGuard.h", module.parent / "FexLoaderCallbackGuard.h")
print("Patched loader callback gate for interval bookkeeping allocations")
