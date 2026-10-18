# Emergency 4 Resolution & Aspect Ratio Fix

**2.0.0-beta.1 · ASI**

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

- Automatic desktop resolution or a custom `ResX` / `ResY` in the INI.
- `KeepUIAspectRatio` selects centered original proportions or full-screen stretched menu/HUD scaling.
- Automatic FOV correction, preserving the game's configured focal parameter; optional `BaseFOV` override.
- In-memory engine recognition and corrections, without writing the game EXE or CFG.
- English INI comments and an initialization log beside the ASI.

## Installation

1. Download **Emergency4.WidescreenFix-v2.0.0-beta.1.zip** from
   [Releases](https://github.com/Simorss/OldGamesFixes/releases/tag/emergency4).
2. Close the game. If you used the old script patcher, see **Upgrading from the previous fix** below.
3. Extract the complete archive into the folder containing the **actual game executable**, preserving the `scripts` folder.
4. Optionally edit `scripts/Emergency4.WidescreenFix.ini`.
5. Launch the game normally.

The required files are `dinput8.dll`, `scripts/Emergency4.WidescreenFix.asi`
and `scripts/Emergency4.WidescreenFix.ini`. No PowerShell setup, Python or
compiler is needed for normal ASI use. The bundled x86
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) loads
the plugin automatically. Its license is included in the archive.

If your installation already contains `dinput8.dll`, preserve it and use a
compatible existing ASI loader rather than overwriting another mod's DLL.
This plugin requires a loader that calls `InitializeASI` outside `DllMain`,
as the included Ultimate ASI Loader does by default.

## Configuration

```ini
[MAIN]
Enabled = 1
ResX = 3440
ResY = 1440
KeepUIAspectRatio = 1
BaseFOV = 0
```

| Option | Values and behavior |
|---|---|
| `Enabled` | `1` enables the plugin; `0` leaves game memory unchanged. |
| `ResX`, `ResY` | Set both dimensions, or set both to `0` for the desktop resolution of the monitor under the mouse pointer at startup. |
| `KeepUIAspectRatio` | `1` keeps original menu/HUD proportions; `0` stretches them across the display. |
| `BaseFOV` | `0` uses the game's configured focal parameter with automatic aspect correction. A positive integer overrides the original 4:3 baseline; this is not an angle in degrees. |

Close and restart the game after editing the INI. The plugin controls these
settings while the game runs; avoid changing display resolution in the old
in-game options. For diagnostics, read `scripts/Emergency4.WidescreenFix.log`.

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
can start in every display mode.

## Upgrading from the previous fix

For a clean migration, run `tools/Restore-LegacyFix.cmd` with the game closed,
then enter the game folder. This optional tool restores the files preserved by
the earlier script patcher, using its existing backup and integrity checks.
Keep the backup folder until restoration is complete. No restore tool is
needed for a fresh game installation.

Restoration returns the state before the first use of that earlier patcher;
older patches installed before it may still remain. The ASI can normalize
recognized earlier project patches in memory and account for their previous
camera aspect. It does not remove those changes from disk. Removing the ASI
later returns to whatever executable/configuration was already on disk.

## Uninstallation

Close the game and remove `scripts/Emergency4.WidescreenFix.asi` and its INI.
Remove the bundled `dinput8.dll` only if no other mods use it. You can also
remove this plugin's log and the optional tools/docs. No ASI backup restore
step is needed: the plugin itself does not write the game executable or CFG.
The game can still save its own normal settings.

## Compatibility

Intended for **Emergency 4, Emergency 4 Deluxe and 911: First Responders**,
regardless of language. Recognition uses Vision 7.1 / MSVC x86 engine
instructions; executable names and whole-file hashes are not allowlisted.
Unrecognized or ambiguous code is left unpatched, with details in the log.

**Beta:** one actual executable, synthetic variants, native hook tests and
24 resolution/interface emulation cases were validated. Full engine startup,
visual appearance and additional regional releases still need in-game testing.
Movies, embedded 3D views and custom mod interfaces may require separate work.

[Source](asi/src) · [Changelog](CHANGELOG.md) ·
[Technical details](docs/TECHNICAL-ASI.md) · [Validation report](docs/validation-asi.json)
