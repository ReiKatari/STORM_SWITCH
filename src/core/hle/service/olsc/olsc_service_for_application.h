// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/ns/ns_types.h"
#include "core/hle/service/service.h"

namespace Service::OLSC {

class IOlscServiceForApplication final : public ServiceFramework<IOlscServiceForApplication> {
public:
    explicit IOlscServiceForApplication(Core::System& system_);
    ~IOlscServiceForApplication() override;

private:
    Result Initialize(ClientProcessId process_id);
    Result GetSaveDataBackupSetting(Out<u8> out_save_data_backup_setting);
    Result SetSaveDataBackupSettingEnabled(bool enabled, NS::Uid account_id);
    Result SetCustomData();
    Result DeleteSaveDataBackupSetting();
    Result GetSaveDataBackupInfoCache(Out<u32> out_count);
    Result Unknown40(Out<u8> out_val);
    Result Unknown42();

    bool initialized{};
};

} // namespace Service::OLSC
