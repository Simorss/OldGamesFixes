**Version 1.1.0-beta.1 · Windows · Beta**

## Fixes

- **Resolution** — choose a custom display resolution, including widescreen and ultrawide modes supported by the game and graphics driver.
- **Aspect Ratio** — correct the gameplay camera for the selected display proportions.
- **Field of View** — expand the horizontal view on wider displays while preserving the original vertical view.
- **Menu and HUD** — choose a centered interface with original proportions or full-screen stretched scaling. This setting affects menu and HUD together.
- **Mouse Alignment** — adjust interface coordinates, hit testing and clipping alongside proportional menu/HUD scaling.

## Options and Tools

- **Resolution Selection** — enter width and height in the English interactive setup.
- **Interface Mode** — select `Original` for proportional scaling or `Stretched` for full-screen scaling.
- **Automatic Detection** — discover the game executable and configuration by their contents; select a file if multiple candidates are found.
- **Preview** — inspect the proposed settings without changing game files.
- **Backup and Restore** — preserve the original executable and configuration, reapply settings and restore the pre-installation files.
- **Command Line** — apply settings or restore the game without interactive prompts.

## Installation

1. Download **Emergency4-ResolutionFix-v1.1.0-beta.1.zip** from this release.
2. Extract the complete archive into a folder. Keep `Emergency4-Fix.cmd`, `Emergency4-Fix.ps1` and `EnginePatch.cs` together.
3. Close the game and run **Emergency4-Fix.cmd**.
4. Select **Apply**, choose the game folder, enter the resolution and choose the menu/HUD mode.
5. Launch the game normally. Run the setup again to change these settings.

**Requirements:** Windows PowerShell 5.1 or PowerShell 7, and write access to the game folder. Python is not required.

## Uninstallation

Run **Emergency4-Fix.cmd**, select **Restore** and choose the same game folder.
Keep `OldGamesFixes-Emergency4-Backup` until restoration is complete. Deleting
the setup files alone does not undo the installed corrections.

## Compatibility

Intended for **Emergency 4, Emergency 4 Deluxe and 911: First Responders**,
regardless of language. Recognition uses engine instruction signatures rather
than executable filenames or a whole-file checksum allowlist. Builds with
unrecognized or ambiguous instructions are left unchanged.

This is a **beta**. One actual executable and additional synthetic variants
were tested; support for every regional, retail, digital or protected build is
not established. Generalized UI appearance needs further in-game validation.
Movies, embedded 3D views and custom mod interfaces may require separate fixes.

## Links

[Instructions and configuration](https://github.com/Simorss/OldGamesFixes/blob/main/Emergency4/README.md) ·
[Source](https://github.com/Simorss/OldGamesFixes/tree/main/Emergency4/src) ·
[Changelog](https://github.com/Simorss/OldGamesFixes/blob/main/Emergency4/CHANGELOG.md) ·
[Report an issue](https://github.com/Simorss/OldGamesFixes/issues)
