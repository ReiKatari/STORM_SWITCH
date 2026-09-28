// SPDX-FileCopyrightText: Copyright 2026 Lemon-Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace Core {
class System;
}

namespace Core::SaveState {

// Captures the current guest CPU register state (all threads) and all mapped
// process memory to a file at `path`. The caller must ensure `system` is
// already paused (no guest CPU instructions executing) before calling this -
// it is not checked here.
//
// Scope: same-session only. Does not capture GPU state or HLE service state
// (open IPC sessions, pending timers, etc.) - restoring onto a process whose
// non-CPU/memory state has diverged since the capture is not supported.
[[nodiscard]] bool Capture(Core::System& system, const std::string& path);

// Restores CPU register state and process memory previously written by
// Capture(). The caller must ensure `system` is already paused. Fails safely
// (logs and returns false, without modifying anything) if the file is
// missing/invalid, or if the live process' thread count no longer matches
// the captured state (e.g. the guest created/destroyed threads since the
// capture) - there is no way to reconcile that without recreating kernel
// objects, which is out of scope for this same-session MVP.
//
// A thread that is still asleep in a kernel wait (condvar/IPC reply/
// WaitSynchronization/address arbiter/sleep) at the moment Restore() runs is
// deliberately left untouched - neither its Svc::ThreadContext nor its own
// guest stack memory is overwritten. That thread is parked inside a live
// host fiber (KThread::GetHostContext()) whose true resume point lives on
// that native call stack, not in the serialized register/memory snapshot;
// blindly restoring over it left the fiber resuming into a world that no
// longer matched what it was holding, and the guest detected this and
// self-terminated via svcBreak (confirmed reproducible on two different
// devices with two different retail titles). Leaving it alone means that
// thread simply keeps running from wherever it currently is instead of
// rewinding - for a typical short-lived wait (an IPC reply, a condvar signal)
// this is a small, self-correcting discrepancy rather than a crash. See
// GetWaitReasonForDebugging() in core/hle/kernel/k_thread.h.
[[nodiscard]] bool Restore(Core::System& system, const std::string& path);

// Read-only: true if any live thread is currently waiting on a condition
// variable or address arbiter - the one wait kind that depends on another
// GUEST thread reaching a specific point and signalling it, rather than on
// a timer (Sleep) or a host-side service (IPC) that don't care about any of
// this. If that other thread just got rewound, or gets rewound by a
// Restore() happening right now, it may never send that signal again,
// leaving the waiter parked forever - the most likely explanation for the
// freezes observed in practice. Touches nothing and doesn't look at any
// savestate file; meant to be polled right before calling Restore(), so a
// caller can wait for a safer moment (nothing in this state) instead of
// committing to a restore while it's true.
[[nodiscard]] bool HasRiskyPendingWaits(Core::System& system);

} // namespace Core::SaveState
