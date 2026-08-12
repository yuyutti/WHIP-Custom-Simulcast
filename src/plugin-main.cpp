/*
WHIP Custom Simulcast
Copyright (C) 2026 yuyutti

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "plugin-config.hpp"
#include "settings-ui.hpp"
#include "simulcast-controller.hpp"

#include <obs-module.h>

#include <memory>

#include <plugin-support.h>

#define LOG_PREFIX "[WHIP Custom Simulcast] "

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
OBS_MODULE_AUTHOR("yuyutti")

namespace {

class WhipCustomSimulcastPlugin {
  public:
    WhipCustomSimulcastPlugin() : controller_(configManager_), uiInjector_(configManager_) {}

    bool load() {
        configManager_.load();
        controller_.install();
        uiInjector_.install();
        return true;
    }

    void unload() {
        uiInjector_.uninstall();
        controller_.uninstall();
    }

  private:
    whip_custom_simulcast::ConfigManager configManager_;
    whip_custom_simulcast::SimulcastController controller_;
    whip_custom_simulcast::SettingsUiInjector uiInjector_;
};

std::unique_ptr<WhipCustomSimulcastPlugin> plugin;

} // namespace

MODULE_EXPORT const char *obs_module_description(void) {
    return "Configures OBS WHIP Simulcast layer resolution, frame rate, and bitrate.";
}

bool obs_module_load(void) {
    plugin = std::make_unique<WhipCustomSimulcastPlugin>();
    if (!plugin->load()) {
        plugin.reset();
        return false;
    }

    blog(LOG_INFO, LOG_PREFIX "Plugin loaded (version %s)", PLUGIN_VERSION);
    return true;
}

void obs_module_unload(void) {
    if (plugin) {
        plugin->unload();
        plugin.reset();
    }

    blog(LOG_INFO, LOG_PREFIX "Plugin unloaded");
}
