# Changelog

## 2.0.0-beta.2

- Add `Windowed` to the INI: `1` selects windowed mode and `0` selects fullscreen.
- Recognize the native `r_fullscreen` CVar and enforce its inverse in memory through the existing verified setters.
- Keep float, integer and boolean CVar representations consistent, preserving saved configuration strings.
- Add windowed/fullscreen, invalid-value and missing-fullscreen-signature tests.

## 2.0.0-beta.1

- Replace the interactive installation workflow with a native x86 ASI and commented English INI.
- Bundle the pinned, checksum-verified Ultimate ASI Loader v9.7.4 with its license.
- Apply camera, menu/HUD, mouse and clipping corrections in process memory.
- Override resolution through five verified typed CVar setters; preserve unrelated variables.
- Preserve configured FOV with automatic aspect correction and an optional explicit BaseFOV.
- Detect the desktop resolution or accept custom width/height from the INI.
- Normalize recognized earlier project patches in memory and include an optional legacy restoration tool.
- Add native detour/gateway tests, real-loader integration, invalid-input checks and x86 GUI emulation.
- Keep this release in beta pending in-game visual and additional real-build validation.

## 1.1.0-beta.1

- Translate all prompts, errors, command scripts and current documentation into English.
- Remove the executable-name and whole-file checksum allowlist.
- Replace fixed patch offsets and virtual addresses with PE-aware engine signature detection.
- Discover compatible game executables and configuration files regardless of filename.
- Support optional `-Executable` and `-Config` selections when multiple candidates exist.
- Recognize and normalize earlier project patches before generating a new configuration.
- Preserve backup integrity checks and reject missing or ambiguous engine signatures.
- Upgrade version 1.0 backup metadata without replacing its original snapshots.
- Add renamed, resource-modified, raw-section-repacked and rebased synthetic fixtures.

Support is based on the recognizable Vision 7.1 x86 instruction family. Testing
with synthetic variants does not establish compatibility with every regional,
retail, digital or packed executable.

## 1.0.0-beta.1

Initial configurable-resolution release, with fixed build-specific offsets,
menu/HUD mode selection, backups, restore and automated packaging. Superseded
by the language-independent recognizer in version 1.1.
