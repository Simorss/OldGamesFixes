# Emergency 4: Resolution & Aspect Ratio Fix

**1.0.0-beta.1 · Windows · [Istruzioni in italiano](README.it.md)**

An interactive PowerShell patcher for a specific Emergency 4 Deluxe / Edizione
Oro build. Enter a resolution and choose whether the menu and HUD should keep
their original 4:3 proportions or stretch across the entire screen. Gameplay
keeps the original vertical FOV and adjusts horizontal FOV for the new aspect.

## Quick start

1. Download and extract the ZIP from **Releases**.
2. Close the game and double-click **Emergency4-Fix.cmd**.
3. Choose Apply, enter the game directory, resolution, and interface mode.
4. Start the game normally. Run the patcher again to change settings or restore.

The interactive prompts are in Italian. The command-line interface is available
for automation:

```powershell
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 3440 -Height 1440 -InterfaceMode Original -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Width 1920 -Height 1080 -InterfaceMode Stretched -NonInteractive
.\Emergency4-Fix.ps1 -GamePath 'D:\Games\Emergency4' -Restore -NonInteractive
```

Add `-Preview` to an apply command to inspect its result without writing files.
Keep `patches.json` next to the PowerShell script. Windows PowerShell 5.1 is
sufficient; Python is needed only to run the developer tests.

## Interface options

**Original** fits and centers the 1024x768 interface uniformly within the chosen
resolution, leaving gameplay at full screen width. At 3440x1440 the interface
occupies the central 1920x1440 area. Narrow/portrait modes also preserve 4:3.

**Stretched** retains the game's original full-screen UI scaling, including its
distortion on non-4:3 screens. Both options apply to the menu and HUD together.

## Supported executable

```text
Em4Deluxe.exe — original SHA-256:
17924F0E847FB0173DDBD5FDB5D5CBBF68F75CF1DB6FDCADB155F964A350FDC9
```

This is not a universal patch for all Emergency 4 releases. Unknown executables
are rejected. Existing fixed-3440 patches from this project can be migrated.
Resolutions must also be supported by the game, graphics driver and display.
Input bounds are 320–16384 pixels wide and 200–16384 pixels high.

Backups are kept in `OldGamesFixes-Emergency4-Backup` inside the game directory.
Restore returns the exact executable and configuration present before the first
use of this patcher, including older fixes if they were already installed.
Do not remove that folder or change resolution through the game's old settings.

## Validation and limitations

The earlier 3440x1440 gameplay fix has been confirmed in-game by the user.
This configurable version is tested on private fixtures and by executing the
actual GUI coordinate and hit-testing routines in x86 emulation. Full visual
validation of the generalized interface patch and other resolutions is pending.
Special mod interfaces, video playback and embedded 3D views may need separate
work. See [validation results](docs/validation.json) and
[technical notes](docs/TECHNICAL.md).

Only scripts and patch metadata are distributed. No game executable, DLL, asset
or user configuration is included. Build a release with `build-release.ps1`.
