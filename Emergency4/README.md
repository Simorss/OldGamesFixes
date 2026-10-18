# Emergency 4 Resolution & Aspect Ratio Fix

**1.1.0-beta.1 · Windows · English interface and documentation**

Choose a custom resolution and whether the menu and HUD should retain their
original proportions or stretch across the entire screen. Gameplay adjusts its
horizontal field of view while preserving the original vertical field of view.

## Language and edition compatibility

This version has **no game-language, executable-name or whole-file checksum
allowlist**. It discovers the camera and GUI routines from machine-code
signatures, follows their pointers and branches, and maps virtual addresses to
the executable's actual PE file offsets. Renamed executables, different resource
data and changed file offsets are supported when the required engine routines
are present.

The patcher is intended for the Emergency 4 family, including Deluxe and
911: First Responders, regardless of localization. Compatibility is determined
by the **Vision 7.1 / MSVC x86 instruction family**, rather than the language or
the label on the game box. It does not claim that every retail, digital, packed,
protected or previously modified build has been tested. A build whose required
instructions cannot be identified uniquely is left unchanged. Removing this
structural check would not make different machine code compatible.

Only one actual game executable was available for validation. Additional
renamed, resource-modified, repacked and rebased fixtures are explicitly
synthetic tests, not independently acquired English or regional game releases.
See [validation results](docs/validation.json).

## Quick start

1. Download the ZIP from [Releases](https://github.com/Simorss/OldGamesFixes/releases)
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

## Command-line examples

```powershell
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 3440 -Height 1440 -InterfaceMode Original -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 1920 -Height 1080 -InterfaceMode Stretched -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Executable 'em4.exe' -Config 'em4.cfg' -Width 2560 -Height 1440 -InterfaceMode Original -Preview -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Restore -NonInteractive
```

`-Executable` and `-Config` are optional filenames inside the game folder.
`-Preview` reports the detected engine and proposed settings without writing
files. Non-interactive use requires explicit filenames if detection is ambiguous.

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

## Validation status

The earlier 3440x1440 gameplay correction was confirmed in-game. Automated
tests execute the actual GUI coordinate and hit-testing routines in x86
emulation, exercise multiple resolutions, restore exact files, and test dynamic
engine recognition. Generalized UI appearance and other real executable builds
still need in-game validation, so this release remains **beta**.

Movies, embedded 3D views and custom mod interfaces may need separate work.
Game assets and user configuration files are not distributed. No game launch
or UI automation is performed by the patcher or developer tests.

## Project files

- `src/`: English interactive patcher, CMD entry point and C# engine scanner.
- `tests/`: integration and x86 emulation tests using private local fixtures.
- `docs/`: implementation notes, release notes and validation results.
- `build-release.ps1`: builds the distributable ZIP and checksum.
- `release/`: generated release packages, excluded from source control.
