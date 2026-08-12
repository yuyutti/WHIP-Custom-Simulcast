# Changelog

All notable changes to this project are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-08-12

### Added

- Initial Windows x64 beta for OBS Studio 32.2.1.
- Per-layer resolution, frame rate, and bitrate controls for WHIP Simulcast Layers 2–4.
- Read-only display of the OBS-controlled main layer.
- Settings panel integration and a Tools menu fallback dialog.
- Japanese and English localization with built-in fallback strings.
- JSON configuration persistence, validation, encoder rollback, and unit tests.
- Windows installer with current-user/all-users modes and repair/uninstall maintenance actions.
- Automated Windows builds, package validation, and draft GitHub Releases.

### Fixed

- Restored all previously changed encoders if any custom layer cannot be applied.
- Corrected Release artifact matching to upload only the Windows Setup and ZIP.
- Prevented current-user and all-users installations from existing at the same time.

### Security

- Verified cached build dependencies against their pinned SHA-256 hashes.
- Pinned third-party GitHub Actions to commit hashes and removed unnecessary secret inheritance.
- Removed PDB files and local build paths from distributed artifacts.
- Enabled Control Flow Guard and CET compatibility for the Windows plugin DLL.

[Unreleased]: https://github.com/yuyutti/WHIP-Custom-Simulcast/compare/0.1.0...HEAD
[0.1.0]: https://github.com/yuyutti/WHIP-Custom-Simulcast/releases/tag/0.1.0
