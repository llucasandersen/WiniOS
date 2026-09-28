#include <cassert>
#include <shared_mutex>
#include <stdexcept>
#include <string>

struct CHPE_V2_CPU_AREA_INFO { bool InSyscallCallback = false; };
#include "../.github/FexLoaderCallbackGuard.h"

static std::shared_mutex intervals;
static CHPE_V2_CPU_AREA_INFO area;

static void allocation_notification() {
  if (area.InSyscallCallback) return;
  // Mirrors nested tracking of an allocation made by an interval insertion.
  intervals.lock();
  intervals.unlock();
}

static void image_map() {
  intervals.lock();
  allocation_notification();
  intervals.unlock();
}

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "unguarded") {
    image_map();
    return 0;
  }
  {
    FexLoaderCallbackGuard guard(&area);
    image_map();
    assert(area.InSyscallCallback);
  }
  assert(!area.InSyscallCallback);
  area.InSyscallCallback = true;
  { FexLoaderCallbackGuard guard(&area); image_map(); }
  assert(area.InSyscallCallback);
  area.InSyscallCallback = false;
  try { FexLoaderCallbackGuard guard(&area); throw std::runtime_error("test"); }
  catch (const std::runtime_error&) {}
  assert(!area.InSyscallCallback);
  { FexLoaderCallbackGuard guard(nullptr); }
}
