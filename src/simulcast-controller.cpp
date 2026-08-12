// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#include "simulcast-controller.hpp"

#include <obs-module.h>
#include <util/config-file.h>

#include <algorithm>
#include <cstring>
#include <vector>

#define LOG_PREFIX "[WHIP Custom Simulcast] "

namespace whip_custom_simulcast {
namespace {

constexpr const char *kWhipOutputId = "whip_output";

} // namespace

SimulcastController::SimulcastController(ConfigManager &configManager) : configManager_(configManager) {}

SimulcastController::~SimulcastController() { uninstall(); }

void SimulcastController::install() {
    if (installed_) {
        return;
    }

    obs_frontend_add_event_callback(frontendEventCallback, this);
    installed_ = true;
}

void SimulcastController::uninstall() {
    if (!installed_) {
        return;
    }

    obs_frontend_remove_event_callback(frontendEventCallback, this);
    installed_ = false;
}

void SimulcastController::frontendEventCallback(enum obs_frontend_event event, void *privateData) {
    static_cast<SimulcastController *>(privateData)->onFrontendEvent(event);
}

void SimulcastController::onFrontendEvent(enum obs_frontend_event event) {
    if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING) {
        applyConfiguration();
    }
}

int SimulcastController::currentLayerCount() const {
    config_t *profile = obs_frontend_get_profile_config();
    if (!profile) {
        return 1;
    }

    return std::clamp(static_cast<int>(config_get_int(profile, "Stream1", "WHIPSimulcastTotalLayers")), 1,
                      static_cast<int>(kMaxLayers));
}

void SimulcastController::applyConfiguration() {
    blog(LOG_INFO, LOG_PREFIX "Streaming starting");

    const PluginConfig config = configManager_.current();
    if (!config.enabled) {
        blog(LOG_INFO, LOG_PREFIX "Plugin is disabled; keeping OBS defaults");
        return;
    }

    obs_output_t *output = obs_frontend_get_streaming_output();
    if (!output) {
        blog(LOG_WARNING, LOG_PREFIX "Streaming output not found; keeping OBS defaults");
        return;
    }

    const char *outputId = obs_output_get_id(output);
    if (!outputId || std::strcmp(outputId, kWhipOutputId) != 0) {
        blog(LOG_INFO, LOG_PREFIX "Output is not whip_output; keeping OBS defaults (actual=%s)",
             outputId ? outputId : "unknown");
        obs_output_release(output);
        return;
    }

    blog(LOG_INFO, LOG_PREFIX "WHIP output detected");

    const int layerCount = currentLayerCount();
    if (layerCount < 2) {
        blog(LOG_INFO, LOG_PREFIX "WHIP Simulcast has fewer than 2 layers; keeping OBS defaults");
        obs_output_release(output);
        return;
    }

    obs_video_info videoInfo{};
    if (!obs_get_video_info(&videoInfo)) {
        blog(LOG_WARNING, LOG_PREFIX "OBS video information is unavailable; keeping OBS defaults");
        obs_output_release(output);
        return;
    }

    struct PlannedLayer {
        obs_encoder_t *encoder;
        size_t index;
        const LayerConfig *config;
        uint32_t frameRateDivisor;
        EncoderState originalState;
    };

    std::vector<PlannedLayer> plannedLayers;
    plannedLayers.reserve(static_cast<size_t>(layerCount - 1));

    blog(LOG_INFO, LOG_PREFIX "Layer 0: using OBS main settings");

    for (int index = 1; index < layerCount; ++index) {
        const LayerConfig &layer = config.layers[static_cast<size_t>(index)];
        const LayerValidation validation = validateLayerConfig(layer, videoInfo);
        if (!validation.valid) {
            blog(LOG_WARNING, LOG_PREFIX "Configuration not applied because layer %d is invalid: %s", index,
                 layerValidationMessage(validation.error));
            obs_output_release(output);
            return;
        }

        obs_encoder_t *encoder = obs_output_get_video_encoder2(output, static_cast<size_t>(index));
        if (!encoder) {
            blog(LOG_WARNING, LOG_PREFIX "Configuration not applied because video encoder %d was not found", index);
            obs_output_release(output);
            return;
        }

        if (obs_encoder_active(encoder)) {
            blog(LOG_WARNING, LOG_PREFIX "Configuration not applied because layer %d encoder is already active", index);
            obs_output_release(output);
            return;
        }

        if (!ensureCurrentVideo(encoder, static_cast<size_t>(index))) {
            obs_output_release(output);
            return;
        }

        EncoderState originalState;
        if (!captureEncoderState(encoder, originalState)) {
            blog(LOG_WARNING, LOG_PREFIX "Configuration not applied because layer %d state could not be captured",
                 index);
            obs_output_release(output);
            return;
        }

        plannedLayers.push_back(
            {encoder, static_cast<size_t>(index), &layer, validation.frameRateDivisor, originalState});
    }

    size_t appliedLayers = 0;
    for (const PlannedLayer &planned : plannedLayers) {
        if (applyLayer(planned.encoder, planned.index, *planned.config, planned.frameRateDivisor)) {
            ++appliedLayers;
            continue;
        }

        while (appliedLayers > 0) {
            --appliedLayers;
            const PlannedLayer &applied = plannedLayers[appliedLayers];
            restoreEncoderState(applied.encoder, applied.index, applied.originalState);
        }

        blog(LOG_WARNING, LOG_PREFIX "Custom layer configuration failed; previously changed layers were restored");
        obs_output_release(output);
        return;
    }

    blog(LOG_INFO, LOG_PREFIX "Configuration applied successfully to %zu custom layer(s)", plannedLayers.size());
    obs_output_release(output);
}

bool SimulcastController::ensureCurrentVideo(obs_encoder_t *encoder, size_t index) const {
    video_t *currentVideo = obs_get_video();
    if (!currentVideo) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu could not be rebound because OBS video is unavailable", index);
        return false;
    }

    if (obs_encoder_parent_video(encoder) != currentVideo) {
        obs_encoder_set_video(encoder, currentVideo);
    }

    if (obs_encoder_parent_video(encoder) != currentVideo) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu could not be rebound to the current OBS video", index);
        return false;
    }

    return true;
}

bool SimulcastController::captureEncoderState(obs_encoder_t *encoder, EncoderState &state) const {
    obs_data_t *settings = obs_encoder_get_settings(encoder);
    if (!settings) {
        return false;
    }

    state.scalingEnabled = obs_encoder_scaling_enabled(encoder);
    state.width = obs_encoder_get_width(encoder);
    state.height = obs_encoder_get_height(encoder);
    state.frameRateDivisor = obs_encoder_get_frame_rate_divisor(encoder);
    state.bitrateKbps = obs_data_get_int(settings, "bitrate");
    obs_data_release(settings);

    return state.width > 0 && state.height > 0 && state.frameRateDivisor > 0;
}

void SimulcastController::restoreEncoderState(obs_encoder_t *encoder, size_t index, const EncoderState &state) const {
    const bool divisorRestored = obs_encoder_set_frame_rate_divisor(encoder, state.frameRateDivisor);

    if (state.scalingEnabled) {
        obs_encoder_set_scaled_size(encoder, state.width, state.height);
    } else {
        obs_encoder_set_scaled_size(encoder, 0, 0);
    }

    obs_data_t *settings = obs_data_create();
    bool bitrateRestored = false;
    if (settings) {
        obs_data_set_int(settings, "bitrate", state.bitrateKbps);
        obs_encoder_update(encoder, settings);
        obs_data_release(settings);

        obs_data_t *restoredSettings = obs_encoder_get_settings(encoder);
        if (restoredSettings) {
            bitrateRestored = obs_data_get_int(restoredSettings, "bitrate") == state.bitrateKbps;
            obs_data_release(restoredSettings);
        }
    }

    const bool scalingRestored = state.scalingEnabled ? obs_encoder_scaling_enabled(encoder) &&
                                                            obs_encoder_get_width(encoder) == state.width &&
                                                            obs_encoder_get_height(encoder) == state.height
                                                      : !obs_encoder_scaling_enabled(encoder);

    if (!divisorRestored || !scalingRestored || !bitrateRestored) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu could not be fully restored after an override failure", index);
    } else {
        blog(LOG_INFO, LOG_PREFIX "Layer %zu restored after an override failure", index);
    }
}

bool SimulcastController::applyLayer(obs_encoder_t *encoder, size_t index, const LayerConfig &layer,
                                     uint32_t frameRateDivisor) {
    if (obs_encoder_active(encoder)) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu encoder is already active; keeping OBS defaults", index);
        return false;
    }

    if (!ensureCurrentVideo(encoder, index)) {
        return false;
    }

    EncoderState originalState;
    if (!captureEncoderState(encoder, originalState)) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu state could not be captured; keeping OBS defaults", index);
        return false;
    }

    if (!obs_encoder_set_frame_rate_divisor(encoder, frameRateDivisor)) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu FPS divisor could not be set", index);
        return false;
    }

    obs_encoder_set_scaled_size(encoder, layer.width, layer.height);
    if (obs_encoder_get_width(encoder) != layer.width || obs_encoder_get_height(encoder) != layer.height) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu resolution could not be set to %ux%u", index, layer.width,
             layer.height);
        restoreEncoderState(encoder, index, originalState);
        return false;
    }

    obs_data_t *settings = obs_data_create();
    if (!settings) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu bitrate settings could not be allocated", index);
        restoreEncoderState(encoder, index, originalState);
        return false;
    }

    obs_data_set_int(settings, "bitrate", layer.bitrateKbps);
    obs_encoder_update(encoder, settings);
    obs_data_release(settings);

    obs_data_t *updatedSettings = obs_encoder_get_settings(encoder);
    const int64_t appliedBitrate = updatedSettings ? obs_data_get_int(updatedSettings, "bitrate") : 0;
    if (updatedSettings) {
        obs_data_release(updatedSettings);
    }

    if (appliedBitrate != layer.bitrateKbps) {
        blog(LOG_WARNING, LOG_PREFIX "Layer %zu bitrate could not be set: requested=%u applied=%lld kbps", index,
             layer.bitrateKbps, static_cast<long long>(appliedBitrate));
        restoreEncoderState(encoder, index, originalState);
        return false;
    }

    blog(LOG_INFO, LOG_PREFIX "Layer %zu: %ux%u @ %u fps / %u kbps (divisor=%u)", index, layer.width, layer.height,
         layer.fps, layer.bitrateKbps, frameRateDivisor);
    return true;
}

} // namespace whip_custom_simulcast
