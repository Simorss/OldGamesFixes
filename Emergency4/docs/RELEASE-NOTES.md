## Emergency 4 Resolution & Aspect Ratio Fix 1.1.0-beta.1

Custom resolutions and gameplay FOV correction, with a choice of centered
original-proportion or full-screen stretched menu/HUD scaling.

### Changes

- All scripts, prompts and current documentation are in English.
- No Italian-edition filename restriction or executable checksum allowlist.
- Game executable and configuration files are discovered automatically.
- PE-aware machine-code signatures locate the camera and GUI patch sites,
  including their data pointers and branch targets.
- Backup, restore, repeated setting changes and migration from earlier fixes.

### Compatibility and validation

Designed for the Emergency 4 family regardless of localization. Compatibility
depends on recognizable **Vision 7.1 / MSVC x86 engine instructions**, not the
game's language. This does not guarantee support for every retail, digital,
packed or otherwise modified executable. Missing or ambiguous signatures stop
the patcher before it writes files.

One actual game executable was available. Renamed, resource-modified, repacked
and rebased fixtures provide additional synthetic coverage. They are not tests
of independently acquired regional releases. Generalized UI appearance and
additional real builds still need in-game validation; this is a **beta** release.

### Installation

Download **Emergency4-ResolutionFix-v1.1.0-beta.1.zip**, extract it completely,
close the game and double-click **Emergency4-Fix.cmd**. Keep **EnginePatch.cs**
next to the PowerShell script. Python is not required. See the English README
for details and command-line options.
