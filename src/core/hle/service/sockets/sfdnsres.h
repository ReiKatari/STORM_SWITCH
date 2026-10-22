// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Sockets {

class SFDNSRES final : public ServiceFramework<SFDNSRES> {
public:
    explicit SFDNSRES(Core::System& system_);
    ~SFDNSRES() override;

private:
    void GetHostByNameRequest(HLERequestContext& ctx);
    void GetGaiStringErrorRequest(HLERequestContext& ctx);
    void GetHostByNameRequestWithOptions(HLERequestContext& ctx);
    void GetAddrInfoRequest(HLERequestContext& ctx);
    void GetAddrInfoRequestWithOptions(HLERequestContext& ctx);
    void GetNameInfoRequest(HLERequestContext& ctx);
    void GetNameInfoRequestWithOptions(HLERequestContext& ctx);
    void RequestCancelHandleRequest(HLERequestContext& ctx);
    void CancelRequest(HLERequestContext& ctx);
    void GetHostByAddrRequestWithOptions(HLERequestContext& ctx);
    void ResolverSetOptionRequest(HLERequestContext& ctx);
    void ResolverGetOptionRequest(HLERequestContext& ctx);
    void ReportTelemetry(HLERequestContext& ctx); // [23.0.0+]
    void QueryAddrInfoRequestWithOptions(HLERequestContext& ctx); // [23.0.0+]
    void FetchAddrInfoRequestWithOptions(HLERequestContext& ctx); // [23.0.0+]
    void CancelQuery(HLERequestContext& ctx); // [23.0.0+]
};

} // namespace Service::Sockets
