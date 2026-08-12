// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#pragma once

#include <obs.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace whip_custom_simulcast {

constexpr size_t kMaxLayers = 4;
constexpr uint32_t kMinimumDimension = 32;
constexpr uint32_t kMaximumDimension = 16384;
constexpr uint32_t kMinimumBitrateKbps = 50;
constexpr uint32_t kMaximumBitrateKbps = 100000;

struct LayerConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t fps = 0;
    uint32_t bitrateKbps = 0;

    bool operator==(const LayerConfig &other) const {
        return width == other.width && height == other.height && fps == other.fps && bitrateKbps == other.bitrateKbps;
    }
};

struct PluginConfig {
    bool enabled = false;
    std::array<LayerConfig, kMaxLayers> layers{};

    bool operator==(const PluginConfig &other) const { return enabled == other.enabled && layers == other.layers; }
};

enum class LayerValidationError {
    None,
    MinimumResolution,
    EvenDimensions,
    ExceedsOutputResolution,
    BitrateRange,
    UnsupportedBaseFps,
    UnsupportedTargetFps,
};

struct LayerValidation {
    bool valid = false;
    uint32_t frameRateDivisor = 0;
    LayerValidationError error = LayerValidationError::None;
};

PluginConfig defaultPluginConfig();
uint32_t nominalBaseFps(const obs_video_info &videoInfo);
std::vector<uint32_t> supportedTargetFps(const obs_video_info &videoInfo);
LayerValidation validateLayerConfig(const LayerConfig &layer, const obs_video_info &videoInfo);
const char *layerValidationMessage(LayerValidationError error);

class ConfigManager {
  public:
    ConfigManager();

    bool load();
    PluginConfig current() const;
    bool save(const PluginConfig &config);

  private:
    bool writeConfig(const PluginConfig &config) const;

    mutable std::mutex mutex_;
    PluginConfig config_;
    std::string configPath_;
};

} // namespace whip_custom_simulcast
