#pragma once

// FEX's loader hook is called directly, outside Wine's syscall callback gate.
// Its interval containers allocate while holding IntervalsLock. Suppress nested
// notifications for those bookkeeping allocations, preserving the caller's gate.
struct FexLoaderCallbackGuard {
  CHPE_V2_CPU_AREA_INFO* Area;
  bool Previous;
  explicit FexLoaderCallbackGuard(CHPE_V2_CPU_AREA_INFO* Area)
    : Area(Area), Previous(Area && Area->InSyscallCallback) {
    if (Area) Area->InSyscallCallback = true;
  }
  ~FexLoaderCallbackGuard() {
    if (Area) Area->InSyscallCallback = Previous;
  }
  FexLoaderCallbackGuard(const FexLoaderCallbackGuard&) = delete;
  FexLoaderCallbackGuard& operator=(const FexLoaderCallbackGuard&) = delete;
};
