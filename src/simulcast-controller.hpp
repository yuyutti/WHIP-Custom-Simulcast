// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#pragma once

#include "plugin-config.hpp"

#include <obs-frontend-api.h>

namespace whip_custom_simulcast {

class SimulcastController {
  public:
    explicit SimulcastController(ConfigManager &configManager);
    ~SimulcastController();

    void install();
    void uninstall();

  private:
    struct EncoderState {
        bool scalingEnabled = false;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t frameRateDivisor = 1;
        int64_t bitrateKbps = 0;
    };

    static void frontendEventCallback(enum obs_frontend_event event, void *privateData);
    void onFrontendEvent(enum obs_frontend_event event);
    void applyConfiguration();
    bool applyLayer(obs_encoder_t *encoder, size_t index, const LayerConfig &layer, uint32_t frameRateDivisor);
    bool captureEncoderState(obs_encoder_t *encoder, EncoderState &state) const;
    void restoreEncoderState(obs_encoder_t *encoder, size_t index, const EncoderState &state) const;
    int currentLayerCount() const;

    ConfigManager &configManager_;
    bool installed_ = false;
};

} // namespace whip_custom_simulcast
