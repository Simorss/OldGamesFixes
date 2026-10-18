# Changelog

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
