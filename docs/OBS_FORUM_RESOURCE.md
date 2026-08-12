# OBS Forum Resource掲載用原稿

## Title

WHIP Custom Simulcast

## Tagline

Configure resolution, frame rate, and bitrate independently for OBS WHIP Simulcast layers.

## Description

WHIP Custom Simulcast is a community plugin for OBS Studio 32.2.1 on Windows x64. It keeps OBS's built-in WHIP and
WebRTC implementation while adding independent resolution, frame-rate, and bitrate controls for Simulcast Layers
2–4. Layer 1 always follows the main OBS encoder settings.

The plugin adds a compact one-row-per-layer editor to Settings > Stream > WHIP > Simulcast. A fallback editor is
available from Tools > WHIP Custom Simulcast if an OBS UI change prevents panel injection.

### Requirements

- Windows x64
- OBS Studio 32.2.1
- WHIP service with Simulcast enabled

### Installation

1. Exit OBS.
2. Download and run the Windows x64 Setup from GitHub Releases.
3. Select either current-user or all-users installation.
4. Start OBS and configure the plugin under Settings > Stream.

Running the same Setup again provides Repair and Uninstall options. A portable ZIP is also available for manual
all-users installation.

### Project links

- [Downloads](https://github.com/yuyutti/WHIP-Custom-Simulcast/releases)
- [Source](https://github.com/yuyutti/WHIP-Custom-Simulcast)
- [Issues](https://github.com/yuyutti/WHIP-Custom-Simulcast/issues)
- License: GPL-2.0-or-later

### Beta notice

This is a beta release. It targets OBS Studio 32.2.1 and is not an official OBS Project plugin. Build success alone
does not prove SFU interoperability; verify each RID and its received media properties with your WHIP service.
