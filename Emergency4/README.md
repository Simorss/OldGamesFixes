# Emergency 4 Resolution & Aspect Ratio Fix

**1.1.0-beta.1**

Choose a custom resolution and whether the menu and HUD should retain their
original proportions or stretch across the entire screen. Gameplay adjusts its
horizontal field of view while preserving the original vertical field of view.

## Fixes

- **Resolution** — custom widescreen and ultrawide resolutions supported by the game and graphics driver.
- **Aspect Ratio** — gameplay camera proportions matched to the selected resolution.
- **Field of View** — horizontal view correction while preserving the original vertical view.
- **Menu and HUD** — centered original proportions or full-screen stretched scaling.
- **Mouse Alignment** — matching interface coordinates, hit testing and clipping.

## Options and tools

- Interactive resolution and interface mode selection, entirely in English.
- Automatic executable and configuration discovery, with manual selection when needed.
- Preview without writing game files.
- Original-file backups, repeated setting changes, restore and command-line options.

## Quick start

1. Download the ZIP from [Releases](https://github.com/Simorss/OldGamesFixes/releases/tag/emergency4)
   and extract it completely.
2. Close the game and double-click **Emergency4-Fix.cmd**.
3. Choose **Apply**, enter the game folder, enter your resolution, then select
   the menu/HUD mode. All prompts and messages are in English.
4. Start the game normally.

Keep `Emergency4-Fix.ps1` and `EnginePatch.cs` next to the CMD file. The game
executable and configuration are detected by their contents, not by an Italian
edition filename. If several candidates are found, choose the intended files.
Copying these three files into the game folder enables automatic folder
selection. No Python, DLL injection, executable download or replacement game
binary is required. Windows PowerShell 5.1 or PowerShell 7 and write access to
the game folder are required. The included C# scanner is compiled locally by
PowerShell using .NET.

## Menu and HUD modes

- **Original** fits and centers the original 1024x768 interface uniformly inside
  the selected resolution. At 3440x1440, the UI occupies the central 1920x1440
  area while gameplay continues across the full screen. Narrow and portrait
  resolutions are also fitted without distorting the interface.
- **Stretched** keeps the game's original full-screen UI scaling, including its
  distortion on non-4:3 screens. Gameplay remains aspect-corrected.

Both options affect menu and HUD together. Resolution support is also limited
by the game, display and graphics driver. Accepted input ranges are 320–16384
pixels wide and 200–16384 pixels high; valid input does not guarantee the engine
can start in every display mode. Avoid changing resolution through the game's
old options after applying the fix; rerun the patcher to change it.

## Backup and restore

The first application saves the original executable and configuration in
`OldGamesFixes-Emergency4-Backup`, together with a normalized engine baseline and
state metadata. The original files are never replaced in that backup when you
change settings. Keep this folder: it is required for reapplication and restore.

**Restore** returns the exact files that existed before the first application,
including earlier fixes if they were already installed. The configuration
present immediately before restoration is retained separately. Version 1.0
backup metadata is upgraded while preserving the original backup files.

SHA-256 remains in use for backup integrity and detection of edits made after
installation. These checks are not a list of permitted game versions or
languages. If you install a different game build or another binary patch, first
restore this fix or preserve/move its existing backup before creating a new one.
