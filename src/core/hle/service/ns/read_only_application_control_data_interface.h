// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "core/file_sys/control_metadata.h"
#include "core/hle/service/cmif_types.h"
#include "core/hle/service/service.h"
#include "core/hle/service/ns/language.h"
#include "core/hle/service/ns/ns_types.h"

namespace Service::NS {

class IReadOnlyApplicationControlDataInterface final
    : public ServiceFramework<IReadOnlyApplicationControlDataInterface> {
public:
    explicit IReadOnlyApplicationControlDataInterface(Core::System& system_);
    ~IReadOnlyApplicationControlDataInterface() override;

    Result GetApplicationControlData(OutBuffer<BufferAttr_HipcMapAlias> out_buffer,
                                     Out<u32> out_actual_size,
                                     ApplicationControlSource application_control_source,
                                     u64 application_id);
    Result GetApplicationDesiredLanguage(Out<ApplicationLanguage> out_desired_language,
                                         u32 supported_languages);
    Result ConvertApplicationLanguageToLanguageCode(Out<u64> out_language_code,
                                                    ApplicationLanguage application_language);
    Result GetApplicationControlData2(
        OutBuffer<BufferAttr_HipcMapAlias> out_buffer,
        Out<u64> out_total_size,
        ApplicationControlSource application_control_source,
        u8 flag1,
        u8 flag2,
        u64 application_id);
    void ListApplicationTitle(HLERequestContext& ctx);
    Result GetApplicationControlData3(
        OutBuffer<BufferAttr_HipcMapAlias> out_buffer,
        Out<u32> out_flags_a,
        Out<u32> out_flags_b,
        Out<u32> out_actual_size,
        ApplicationControlSource application_control_source,
        u8 flag1,
        u8 flag2,
        u64 application_id);

private:
    struct CachedControlData {
        std::vector<u8> nacp_bytes;
        std::vector<u8> icon_bytes;
        FileSys::LanguageEntry language_entry{};
        bool has_nacp{false};
        bool has_icon{false};
    };

    const CachedControlData& GetOrCreateCachedControl(u64 application_id);

    static std::mutex s_cache_mutex;
    static std::unordered_map<u64, CachedControlData> s_control_cache;
};

} // namespace Service::NS
