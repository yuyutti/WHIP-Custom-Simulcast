// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#include "plugin-config.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace whip_custom_simulcast;

extern "C" obs_module_t *obs_current_module(void) { return nullptr; }

namespace {

void require(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

obs_video_info videoInfo(uint32_t fpsNum, uint32_t fpsDen, uint32_t width = 1920, uint32_t height = 1080) {
    obs_video_info info{};
    info.fps_num = fpsNum;
    info.fps_den = fpsDen;
    info.output_width = width;
    info.output_height = height;
    return info;
}

} // namespace

int main() {
    require(nominalBaseFps(videoInfo(60, 1)) == 60, "60 fps nominal rate");
    require(nominalBaseFps(videoInfo(60000, 1001)) == 60, "59.94 fps nominal rate");
    require(nominalBaseFps(videoInfo(30000, 1001)) == 30, "29.97 fps nominal rate");
    require(nominalBaseFps(videoInfo(595, 10)) == 0, "nonstandard fractional rate is rejected");
    require(nominalBaseFps(videoInfo(240, 1)) == 240, "240 fps upper bound");
    require(nominalBaseFps(videoInfo(241, 1)) == 0, "FPS above the supported upper bound is rejected");

    const std::vector<uint32_t> expectedFps = {60, 30, 20, 15, 12, 10, 6, 5, 4, 3, 2, 1};
    require(supportedTargetFps(videoInfo(60, 1)) == expectedFps, "60 fps divisor choices");

    const LayerConfig validLayer{854, 480, 30, 700};
    LayerValidation validation = validateLayerConfig(validLayer, videoInfo(60, 1));
    require(validation.valid && validation.frameRateDivisor == 2, "valid 854x480 30 fps layer");

    validation = validateLayerConfig(validLayer, videoInfo(60000, 1001));
    require(validation.valid && validation.frameRateDivisor == 2, "valid 29.97 fps output from nominal 59.94");

    validation = validateLayerConfig({853, 480, 30, 700}, videoInfo(60, 1));
    require(validation.error == LayerValidationError::EvenDimensions, "odd resolution is rejected");

    validation = validateLayerConfig({1922, 1080, 30, 700}, videoInfo(60, 1));
    require(validation.error == LayerValidationError::ExceedsOutputResolution,
            "resolution above OBS output is rejected");

    validation = validateLayerConfig({854, 480, 24, 700}, videoInfo(60, 1));
    require(validation.error == LayerValidationError::UnsupportedTargetFps, "non-divisor FPS is rejected");

    validation = validateLayerConfig({854, 480, 30, 49}, videoInfo(60, 1));
    require(validation.error == LayerValidationError::BitrateRange, "bitrate below minimum is rejected");

    validation = validateLayerConfig({854, 480, 30, 50}, videoInfo(60, 1));
    require(validation.valid, "minimum bitrate is accepted");

    std::cout << "All plugin configuration tests passed.\n";
    return 0;
}
