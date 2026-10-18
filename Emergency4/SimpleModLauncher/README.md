# Emergency 4 Simple Mod Launcher

**EM4SMD · Version 1.0 · Bonus tool**

A small, standalone CMD launcher that lists the folders inside `Mods` with a
number beside each one. Enter a number to start Emergency 4 directly with that
mod, without loading it manually from the game menu.

## How to use

1. Download **EM4SMD.cmd** from the [bonus release](https://github.com/Simorss/OldGamesFixes/releases/tag/emergency4-smd).
2. Place it beside **Em4Deluxe.exe** in the game folder. A direct subfolder, such as `Lancia`, also works.
3. Double-click **EM4SMD.cmd**, enter the mod number and press Enter.
4. Enter **0** to exit without starting the game.

The launcher scans `Mods` each time it runs, so newly added mod folders appear
automatically. It starts the selected mod with the game folder as the working
directory:

```text
Em4Deluxe.exe -game -mod "Selected mod folder"
```

**Requirements:** Windows PowerShell and a game installation containing
`Em4Deluxe.exe` and `Mods`. No separate PowerShell file or installation step is
needed. The launcher does not edit game files, install mods or require the
widescreen ASI. The displayed names are folder names; the selected folder must
contain a valid, compatible Emergency 4 mod.

## Development credits

100% of the work on this launcher was done through **GPT-6.1 Sol**.
