**Release 1.0 · Windows · ASI**

A wider view for The Regiment on widescreen and ultrawide displays, with
independent camera and weapon FOV controls while playing.

## Features

- **Hor+ FOV correction** for the game's Vert− behavior.
- **Independent weapon / viewmodel FOV**.
- **Mouse acceleration and smoothing removal**, with sensitivity still controlled by the game menu.
- **FOV display** after adjustments and resolution changes.
- **Remappable controls and saved settings**, plus a master enable/disable switch.

| Setting | Decrease | Increase |
| --- | --- | --- |
| Camera FOV | F7 | F8 |
| Weapon FOV | F11 | F12 |

## Before / After

[**Open the interactive comparison ↔**](https://simorss.github.io/OldGamesFixes/theregiment/)

**Without fix**

![Without fix](https://raw.githubusercontent.com/Simorss/OldGamesFixes/main/TheRegiment/screenshots/without-fix.png)

**With fix**

![With fix](https://raw.githubusercontent.com/Simorss/OldGamesFixes/main/TheRegiment/screenshots/with-fix.png)

Original 3440×1440 screenshots, captured at different moments.

## Installation

Close the game and extract **TheRegiment.FOVFix-v1.0.zip** into the main game
folder, preserving `system/scripts`. The ZIP contains only `dinput8.dll`, the
ASI, the INI and `ThirteenAG-LICENSE.txt`. Preserve an existing loader/mod DLL.
Edit the INI if desired and restart the game after changes.

[Instructions and configuration](https://github.com/Simorss/OldGamesFixes/blob/main/TheRegiment/README.md) ·
[Source](https://github.com/Simorss/OldGamesFixes/tree/main/TheRegiment/src) ·
[Report an issue](https://github.com/Simorss/OldGamesFixes/issues)

Validated on one 32-bit executable build; other builds are left unchanged.
Ultimate ASI Loader is a separate component by **ThirteenAG**.
