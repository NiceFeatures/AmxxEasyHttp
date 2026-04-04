# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-04-04

- **Primary Stable Release (AmxxEasyHttp v2.0.0 by Polarhigh & iceeedR)**: Major stabilization, security hardening, and API consolidation by Polarhigh & iceeedR.
- **Native Renaming**: Renamed `_ezhttp_steam_to_steam64` to `ezhttp_steam_to_steam64` for better API consistency.
- **Improved Validation**: Enhanced parameter count validation in all natives to prevent "vector too long" and out-of-bounds crashes.

### Added
- **Security Hardening**:
    - **Global Exception Boundaries**: Implemented `try/catch` blocks across all major entry points (StartFrame, OnAmxxAttach, Async Tasks) to prevent HLDS crashes.
    - **Path Traversal Protection**: Integrated `ValidatePath` to ensure all file operations (`download`, `save_to_file`) are restricted to the server directory.
    - **Memory Safety**: Added a 10MB limit to `user_data` to prevent excessive memory consumption.
- **New Natives**:
    - `ezhttp_destroy_options(EzHttpOptions:options)`: Allows manual cleanup of options handles.
    - `ezhttp_steam_to_steam64(...)`: Public utility for SteamID conversion.
- **Binary Data Support**: Added `.data` and `.data_len` parameters to `ezhttp_get`, `ezhttp_post`, `ezhttp_put`, `ezhttp_patch`, and `ezhttp_delete` for raw binary payloads.

### Fixed
- Fixed a critical crash in `ezhttp_post` and other natives caused by incorrect argument count calculation.
- Fixed potential null-pointer dereferences when passing invalid strings from AMXX.
- Restored missing `enum` tags in headers to fix "tag mismatch" warnings in Pawn scripts.

## [2.4.0] - Previous Release
- Initial implementation of asynchronous HTTP/HTTPS/FTP.
- Basic JSON support via Parson.
