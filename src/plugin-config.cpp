// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#include "plugin-config.hpp"

#include <obs-module.h>
#include <util/bmem.h>
#include <util/platform.h>

#include <algorithm>
#include <cmath>
#include <utility>

#define LOG_PREFIX "[WHIP Custom Simulcast] "

namespace whip_custom_simulcast {
namespace {

constexpr int64_t kConfigVersion = 1;

bool serializedLayerIsValid(const LayerConfig &layer) {
    return layer.width >= kMinimumDimension && layer.width <= kMaximumDimension && layer.width % 2 == 0 &&
           layer.height >= kMinimumDimension && layer.height <= kMaximumDimension && layer.height % 2 == 0 &&
           layer.fps >= 1 && layer.fps <= 240 && layer.bitrateKbps >= kMinimumBitrateKbps &&
           layer.bitrateKbps <= kMaximumBitrateKbps;
}

LayerConfig readLayer(obs_data_t *data, const LayerConfig &fallback) {
    LayerConfig layer = {
        static_cast<uint32_t>(obs_data_get_int(data, "width")),
        static_cast<uint32_t>(obs_data_get_int(data, "height")),
        static_cast<uint32_t>(obs_data_get_int(data, "fps")),
        static_cast<uint32_t>(obs_data_get_int(data, "bitrate_kbps")),
    };

    return serializedLayerIsValid(layer) ? layer : fallback;
}

} // namespace

PluginConfig defaultPluginConfig() {
    PluginConfig config;
    config.enabled = false;
    config.layers[1] = {854, 480, 30, 700};
    config.layers[2] = {640, 360, 30, 500};
    config.layers[3] = {426, 240, 15, 250};
    return config;
}

uint32_t nominalBaseFps(const obs_video_info &videoInfo) {
    if (videoInfo.fps_num == 0 || videoInfo.fps_den == 0) {
        return 0;
    }

    const double actualFps = static_cast<double>(videoInfo.fps_num) / static_cast<double>(videoInfo.fps_den);
    if (!std::isfinite(actualFps) || actualFps < 1.0 || actualFps > 240.0) {
        return 0;
    }

    const auto nominalFps = static_cast<uint32_t>(std::lround(actualFps));
    const double tolerance = std::max(0.01, static_cast<double>(nominalFps) * 0.002);

    return std::abs(actualFps - static_cast<double>(nominalFps)) <= tolerance ? nominalFps : 0;
}

std::vector<uint32_t> supportedTargetFps(const obs_video_info &videoInfo) {
    std::vector<uint32_t> values;
    const uint32_t baseFps = nominalBaseFps(videoInfo);

    if (baseFps == 0) {
        return values;
    }

    for (uint32_t divisor = 1; divisor <= baseFps; ++divisor) {
        if (baseFps % divisor == 0) {
            values.push_back(baseFps / divisor);
        }
    }

    return values;
}

LayerValidation validateLayerConfig(const LayerConfig &layer, const obs_video_info &videoInfo) {
    LayerValidation result;

    if (layer.width < kMinimumDimension || layer.height < kMinimumDimension) {
        result.error = LayerValidationError::MinimumResolution;
        return result;
    }

    if (layer.width % 2 != 0 || layer.height % 2 != 0) {
        result.error = LayerValidationError::EvenDimensions;
        return result;
    }

    if (videoInfo.output_width == 0 || videoInfo.output_height == 0 || layer.width > videoInfo.output_width ||
        layer.height > videoInfo.output_height) {
        result.error = LayerValidationError::ExceedsOutputResolution;
        return result;
    }

    if (layer.bitrateKbps < kMinimumBitrateKbps || layer.bitrateKbps > kMaximumBitrateKbps) {
        result.error = LayerValidationError::BitrateRange;
        return result;
    }

    const uint32_t baseFps = nominalBaseFps(videoInfo);
    if (baseFps == 0) {
        result.error = LayerValidationError::UnsupportedBaseFps;
        return result;
    }

    if (layer.fps == 0 || layer.fps > baseFps || baseFps % layer.fps != 0) {
        result.error = LayerValidationError::UnsupportedTargetFps;
        return result;
    }

    result.valid = true;
    result.frameRateDivisor = baseFps / layer.fps;
    return result;
}

const char *layerValidationMessage(LayerValidationError error) {
    switch (error) {
    case LayerValidationError::None:
        return "Valid";
    case LayerValidationError::MinimumResolution:
        return "Resolution must be at least 32x32.";
    case LayerValidationError::EvenDimensions:
        return "Width and height must be even numbers.";
    case LayerValidationError::ExceedsOutputResolution:
        return "Resolution exceeds the current OBS output resolution.";
    case LayerValidationError::BitrateRange:
        return "Bitrate must be between 50 and 100000 kbps.";
    case LayerValidationError::UnsupportedBaseFps:
        return "The current OBS base FPS is unsupported.";
    case LayerValidationError::UnsupportedTargetFps:
        return "FPS must be an integer divisor of the nominal OBS base FPS.";
    }

    return "Unknown validation error.";
}

ConfigManager::ConfigManager() : config_(defaultPluginConfig()) {
    char *configDirectory = obs_module_config_path("");
    char *configPath = obs_module_config_path("config.json");

    if (configDirectory) {
        os_mkdirs(configDirectory);
        bfree(configDirectory);
    }

    if (configPath) {
        configPath_ = configPath;
        bfree(configPath);
    }
}

bool ConfigManager::load() {
    std::lock_guard lock(mutex_);
    config_ = defaultPluginConfig();

    if (configPath_.empty() || !os_file_exists(configPath_.c_str())) {
        blog(LOG_INFO, LOG_PREFIX "No saved configuration found; using defaults");
        return true;
    }

    obs_data_t *data = obs_data_create_from_json_file_safe(configPath_.c_str(), "bak");
    if (!data) {
        blog(LOG_WARNING, LOG_PREFIX "Failed to load config.json; using defaults");
        return false;
    }

    const int64_t version = obs_data_get_int(data, "version");
    if (version != kConfigVersion) {
        blog(LOG_WARNING, LOG_PREFIX "Unsupported configuration version: %lld; using compatible values",
             static_cast<long long>(version));
    }

    config_.enabled = obs_data_get_bool(data, "enabled");

    obs_data_array_t *layers = obs_data_get_array(data, "layers");
    if (layers) {
        const size_t count = std::min(obs_data_array_count(layers), kMaxLayers);
        for (size_t index = 1; index < count; ++index) {
            obs_data_t *layerData = obs_data_array_item(layers, index);
            if (layerData) {
                config_.layers[index] = readLayer(layerData, config_.layers[index]);
                obs_data_release(layerData);
            }
        }
        obs_data_array_release(layers);
    }

    obs_data_release(data);
    blog(LOG_INFO, LOG_PREFIX "Configuration loaded");
    return true;
}

PluginConfig ConfigManager::current() const {
    std::lock_guard lock(mutex_);
    return config_;
}

bool ConfigManager::save(const PluginConfig &config) {
    if (!writeConfig(config)) {
        return false;
    }

    std::lock_guard lock(mutex_);
    config_ = config;
    blog(LOG_INFO, LOG_PREFIX "Configuration saved");
    return true;
}

bool ConfigManager::writeConfig(const PluginConfig &config) const {
    if (configPath_.empty()) {
        blog(LOG_WARNING, LOG_PREFIX "Module configuration path is unavailable");
        return false;
    }

    obs_data_t *data = obs_data_create();
    obs_data_array_t *layers = obs_data_array_create();
    if (!data || !layers) {
        if (layers) {
            obs_data_array_release(layers);
        }
        if (data) {
            obs_data_release(data);
        }
        blog(LOG_WARNING, LOG_PREFIX "Failed to allocate configuration data");
        return false;
    }

    obs_data_set_int(data, "version", kConfigVersion);
    obs_data_set_bool(data, "enabled", config.enabled);

    for (size_t index = 0; index < kMaxLayers; ++index) {
        obs_data_t *layerData = obs_data_create();
        if (!layerData) {
            obs_data_array_release(layers);
            obs_data_release(data);
            blog(LOG_WARNING, LOG_PREFIX "Failed to allocate layer configuration data");
            return false;
        }

        if (index == 0) {
            obs_data_set_bool(layerData, "use_obs_main", true);
        } else {
            const LayerConfig &layer = config.layers[index];
            obs_data_set_int(layerData, "width", layer.width);
            obs_data_set_int(layerData, "height", layer.height);
            obs_data_set_int(layerData, "fps", layer.fps);
            obs_data_set_int(layerData, "bitrate_kbps", layer.bitrateKbps);
        }

        obs_data_array_push_back(layers, layerData);
        obs_data_release(layerData);
    }

    obs_data_set_array(data, "layers", layers);
    obs_data_array_release(layers);

    const bool saved = obs_data_save_json_pretty_safe(data, configPath_.c_str(), "tmp", "bak");
    obs_data_release(data);

    if (!saved) {
        blog(LOG_WARNING, LOG_PREFIX "Failed to save plugin configuration");
    }

    return saved;
}

} // namespace whip_custom_simulcast
