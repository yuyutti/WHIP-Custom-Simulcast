# WHIP Custom Simulcast

[![Build](https://github.com/yuyutti/WHIP-Custom-Simulcast/actions/workflows/push.yaml/badge.svg?branch=main)](https://github.com/yuyutti/WHIP-Custom-Simulcast/actions/workflows/push.yaml)
[![Release](https://img.shields.io/github/v/release/yuyutti/WHIP-Custom-Simulcast?include_prereleases)](https://github.com/yuyutti/WHIP-Custom-Simulcast/releases)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL--2.0--or--later-blue.svg)](LICENSE)

English | [日本語](README.md)

WHIP Custom Simulcast is an OBS Studio plugin that lets you configure the resolution, frame rate, and bitrate of
each secondary layer while retaining OBS Studio 32.2.1's built-in WHIP Simulcast output.

## Replace automatic equal splits with exact layer settings

OBS's built-in WHIP Simulcast UI only lets you select the total number of layers. OBS then derives secondary
resolution and bitrate values by evenly dividing the main output, while every layer keeps the main output FPS. This
behavior follows the
[OBS Studio 32.2.1 implementation](https://github.com/obsproject/obs-studio/blob/32.2.1/frontend/utility/WHIPSimulcastEncoders.hpp).

- 2 layers: approximately 50% and 100% of the main output
- 3 layers: approximately 33%, 67%, and 100%
- 4 layers: 25%, 50%, 75%, and 100%

This plugin removes that fixed split. Layer 1 still follows OBS, while you explicitly specify the **resolution, FPS,
and bitrate of every Layer 2–4**.

For example, with a `1920x1080 / 60 fps / 8000 kbps` main output, the secondary layers can be designed as
`1280x720 / 60 fps / 4500 kbps` and `854x480 / 30 fps / 1200 kbps` instead of being forced to 50% or 33% steps.

It does not replace OBS, `obs-webrtc`, SDP generation, or the WebRTC transport. It uses the public Frontend and
libobs APIs for encoder control, and injects its settings into the existing OBS WHIP settings panel.

> [!IMPORTANT]
> This is currently a beta for Windows x64, built against OBS Studio 32.2.1. OBS versions earlier than 32.2.1 are
> unsupported. A future OBS UI change may require using the fallback dialog from the Tools menu.
> Initial Setup and DLL releases are unsigned, so Windows displays an Unknown publisher warning.

This is a community project and is not an official OBS Project plugin.

## Download

Download `whip-custom-simulcast-<version>-windows-x64-setup.exe` from
[GitHub Releases](https://github.com/yuyutti/WHIP-Custom-Simulcast/releases).

A portable ZIP is also provided for manual installation.

## Features

- Windows x64 and OBS Studio 32.2.1
- Activates only for the built-in WHIP output
- Synchronizes with OBS's 1–4 Total Layers setting
- Always preserves OBS settings for Layer 1 / encoder index 0
- Independently configures Layers 2–4:
  - Resolution
  - Frame rate
  - Bitrate
- One compact row per active layer
- Plugin enable/disable switch
- Apply/OK saves and Cancel discards changes
- Atomic JSON configuration storage in the OBS module config directory
- Fallback settings dialog under Tools > WHIP Custom Simulcast
- Per-layer rollback when an encoder override fails
- Japanese and English UI, with built-in text fallback when locale files are unavailable

## How it works

At `OBS_FRONTEND_EVENT_STREAMING_STARTING`, the plugin obtains the current streaming output and applies a layer only
when all of the following conditions are true:

```text
Plugin enabled
AND Output ID == whip_output
AND WHIP Total Layers >= 2
AND the target layer configuration is valid
AND the target encoder exists and is not active yet
```

Encoder and WHIP RID mapping:

| Encoder index | WHIP RID | Behavior |
|---|---:|---|
| 0 | 0 | Preserve OBS main settings |
| 1 | 1 | Apply Layer 2 settings |
| 2 | 2 | Apply Layer 3 settings |
| 3 | 3 | Apply Layer 4 settings |

## Defaults

The plugin is disabled on first launch. The following values are prepared:

| Layer | Resolution | FPS | Bitrate |
|---|---:|---:|---:|
| 1 | OBS Main | OBS Main | OBS Main |
| 2 | 854x480 | 30 | 700 kbps |
| 3 | 640x360 | 30 | 500 kbps |
| 4 | 426x240 | 15 | 250 kbps |

## Configuration UI

1. Open Settings > Stream in OBS.
2. Select WHIP as the service.
3. Choose 1–4 under Simulcast > Total Layers.
4. Enable WHIP Custom Simulcast.
5. Configure the displayed Layer 2–4 rows.
6. Select Apply or OK.

Layer 1 displays the current OBS output resolution, frame rate, and streaming bitrate and cannot be edited. Layers
above Total Layers are hidden. Canceling or closing the settings window discards unsaved plugin changes.

If the plugin cannot inject its panel after an OBS UI update, use Tools > WHIP Custom Simulcast. The fallback dialog
uses the current Total Layers value from the active OBS profile.

## Validation

- Width and height: at least 32 and no larger than the current OBS output resolution
- Width and height: even values
- Bitrate: 50–100000 kbps
- FPS: an integer divisor of the nominal OBS base FPS

At 60 fps, valid choices include 60, 30, 20, 15, 12, and 10. For 59.94 fps and 29.97 fps, divisors are calculated
using nominal 60 fps and 30 fps values. Selecting 30 fps with a 59.94 fps base therefore sends approximately 29.97
fps.

## Configuration file

Configuration is stored in the OBS module configuration directory:

```text
plugin_config/whip-custom-simulcast/config.json
```

The OBS profile value `Stream1.WHIPSimulcastTotalLayers` is the source of truth for the active layer count. The plugin
keeps settings for up to four layers in its JSON file, so values are restored if Total Layers is reduced and later
increased. Saves use temporary and backup files.

## Building

This repository uses the official OBS Plugin Template build structure. The first configure downloads OBS Studio
32.2.1 sources and matching dependencies into `.deps`.

Requirements:

- Visual Studio 2022 with Desktop development with C++
- Windows 10/11 SDK
- CMake 3.28–3.30

PowerShell:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Build output:

```text
build_x64/RelWithDebInfo/whip-custom-simulcast.dll
```

## Installation

1. Exit OBS.
2. Run the Setup downloaded from the release.
3. Select either Install for me only or Install for all users.
4. Complete installation and start OBS.

The current-user option does not require administrator access and installs the plugin only for that Windows account.
The all-users option requests administrator access and installs to `C:\ProgramData\obs-studio\plugins`.

> [!NOTE]
> OBS 32.2.1 for Windows does not search a per-user plugin directory by default. Current-user installation therefore
> configures the `OBS_PLUGINS_PATH` and `OBS_PLUGINS_DATA_PATH` user environment variables. Setup stops without
> overwriting them if existing values point to another custom plugin directory.

Run the same Setup again after installation to select a maintenance action:

- Repair: restore the DLL, locale files, and documentation
- Uninstall: remove the plugin files

Plugin configuration is retained during uninstall.

### Manual ZIP installation

Copy the ZIP's `whip-custom-simulcast` folder to `C:\ProgramData\obs-studio\plugins`. This installs the plugin for
all users.

Installed layout:

```text
C:\ProgramData\obs-studio\plugins\whip-custom-simulcast\
├── bin\64bit\whip-custom-simulcast.dll
├── data\locale\
│   ├── en-US.ini
│   └── ja-JP.ini
├── CHANGELOG.md
├── LICENSE
├── README.en.md
└── README.md
```

### Updating and uninstalling

- Update: exit OBS and run the new Setup.
- Repair or uninstall: exit OBS and run the Setup used for installation again.
- You can also uninstall from Windows Installed apps.
- To also reset plugin settings, remove `%APPDATA%\obs-studio\plugin_config\whip-custom-simulcast`.

## Verification status

Release builds, configuration tests, and installer/ZIP content checks are automated. Users do not need
to run development tests.

Interoperability and received media quality can still vary between WHIP/SFU implementations and cannot be fully
guaranteed by automated checks. The plugin is therefore currently published as a beta. If a problem occurs, report
it with a sanitized OBS log in GitHub Issues.

## Out of scope for the beta

- Custom WHIP/WHEP implementation
- SDP or RTP manipulation
- Customization of the main layer
- Per-layer codec or encoder preset selection
- Dynamic resolution or FPS changes while streaming
- 5–10 layers
- Official macOS or Linux support

## Support and contributing

Use [GitHub Issues](https://github.com/yuyutti/WHIP-Custom-Simulcast/issues) for bugs and feature requests. A bug report
should include the OBS version, plugin version, encoder, reproduction steps, and OBS log. Remove WHIP URLs, bearer
tokens, stream keys, and other secrets before attaching a log.

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), and [CHANGELOG.md](CHANGELOG.md) for project
policies and release history.

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).
