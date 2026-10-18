**Version 2.0.0-beta.1 · Windows · ASI · Beta**

## Fixes

- **Resolution** — choose a custom display resolution, including widescreen and ultrawide modes supported by the game and graphics driver.
- **Aspect Ratio** — correct the gameplay camera for the selected display proportions.
- **Field of View** — expand the horizontal view on wider displays while preserving the original vertical view.
- **Menu and HUD** — choose a centered interface with original proportions or full-screen stretched scaling. This setting affects menu and HUD together.
- **Mouse Alignment** — adjust interface coordinates, hit testing and clipping alongside proportional menu/HUD scaling.

## Options and Tools

- **Resolution** — automatic desktop detection or explicit `ResX` / `ResY` in the INI.
- **Interface Mode** — `KeepUIAspectRatio=1` keeps original menu/HUD proportions; `0` stretches the interface across the screen.
- **FOV** — automatic correction of the game's configured focal parameter; optional `BaseFOV` override.
- **Runtime Patches** — the ASI applies changes in memory without writing the game EXE or CFG.
- **Diagnostics** — English configuration comments and an initialization log beside the ASI.

## Installation

1. Download **Emergency4.WidescreenFix-v2.0.0-beta.1.zip** from this release.
2. Close the game. For an upgrade from the script patcher, first use the included `tools/Restore-LegacyFix.cmd` as described in the README.
3. Extract the archive into the folder containing the actual game executable, preserving the `scripts` folder.
4. Optionally edit `scripts/Emergency4.WidescreenFix.ini` to set resolution, menu/HUD scaling and FOV.
5. Launch the game normally. Restart after changing the INI.

The archive includes **x86 Ultimate ASI Loader v9.7.4** (`dinput8.dll`) and its
license. Preserve an existing loader/mod DLL and follow the README if one is
already installed. PowerShell is only used by the optional legacy restore tool.
Python and a compiler are not required for normal ASI use.

## Uninstallation

Close the game and remove this plugin's ASI and INI from `scripts`.
Remove `dinput8.dll` only if other mods do not use it. Earlier on-disk patches
remain until separately restored; the ASI itself does not write the EXE or CFG.

## Compatibility

Intended for **Emergency 4, Emergency 4 Deluxe and 911: First Responders**,
regardless of language. Recognition uses engine instruction signatures rather
than executable filenames or a whole-file checksum allowlist. Builds with
unrecognized or ambiguous instructions are left unchanged.

This is a **beta**. One actual executable and additional synthetic variants
were tested; support for every regional, retail, digital or protected build is
not established. Full engine startup and visual appearance need further in-game validation.
Movies, embedded 3D views and custom mod interfaces may require separate fixes.

## Links

[Instructions and configuration](https://github.com/Simorss/OldGamesFixes/blob/main/Emergency4/README.md) ·
[Source](https://github.com/Simorss/OldGamesFixes/tree/main/Emergency4/asi/src) ·
[Changelog](https://github.com/Simorss/OldGamesFixes/blob/main/Emergency4/CHANGELOG.md) ·
[Report an issue](https://github.com/Simorss/OldGamesFixes/issues)
