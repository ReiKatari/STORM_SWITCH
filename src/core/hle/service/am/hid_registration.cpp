// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/service/am/hid_registration.h"
#include "core/hle/service/hid/hid_server.h"
#include "core/hle/service/os/process.h"
#include "core/hle/service/sm/sm.h"
#include "hid_core/resource_manager.h"

namespace Service::AM {

HidRegistration::HidRegistration(Core::System& system, Process& process)
    : m_system(system), m_process(process) {
    this->RegisterCurrentProcess();
}

void HidRegistration::EnsureHidServer() {
    if (!m_hid_server) {
        m_hid_server = m_system.ServiceManager().GetService<HID::IHidServer>("hid", false);
    }
}

void HidRegistration::RegisterCurrentProcess() {
    this->EnsureHidServer();
    if (m_hid_server && m_process.IsInitialized() && !m_is_registered) {
        const u64 pid = m_process.GetProcessId();
        LOG_INFO(Service_AM, "HidRegistration: Registering ARUID {:016X} with HID resource manager", pid);
        m_hid_server->GetResourceManager()->RegisterAppletResourceUserId(pid, true);
        m_hid_server->GetResourceManager()->SetAruidValidForVibration(pid, true);
        m_is_registered = true;
    }
}

HidRegistration::~HidRegistration() {
    this->EnsureHidServer();
    if (m_hid_server && m_process.IsInitialized() && m_is_registered) {
        m_hid_server->GetResourceManager()->SetAruidValidForVibration(m_process.GetProcessId(),
                                                                      false);
        m_hid_server->GetResourceManager()->UnregisterAppletResourceUserId(
            m_process.GetProcessId());
        m_is_registered = false;
    }
}

void HidRegistration::EnableAppletToGetInput(bool enable) {
    this->RegisterCurrentProcess();
    if (m_hid_server && m_process.IsInitialized()) {
        m_hid_server->GetResourceManager()->SetAruidValidForVibration(m_process.GetProcessId(),
                                                                      enable);
        m_hid_server->GetResourceManager()->EnableInput(m_process.GetProcessId(), enable);
    }
}

} // namespace Service::AM
