# ASI implementation and validation

Version 2.0 runs as a native x86 DLL named `Emergency4.WidescreenFix.asi`.
Ultimate ASI Loader v9.7.4 loads it through `dinput8.dll` and calls its exported
`InitializeASI` outside the loader lock. `DllMain` only records the module
handle and disables thread notifications. Initialization is guarded once.

## Engine recognition

The PE-aware recognizer searches the loaded main module, using its actual base
address, section boundaries, import names, instruction signatures and resolved
branch destinations. Camera recognition verifies the imported
`PerspectiveToAngles@VisRenderer_cl` call. GUI drawing, inverse coordinates,
mouse conversion, clipping and the root constructor are correlated through
their data operands. Recognized earlier project patches are normalized only
in the in-memory patch plan.

There is no filename, language or whole-file executable hash allowlist.
Required instructions must still be unique and consistent. Other instruction
families or packed/protected executables need additional recognition profiles.

## Resolution and FOV

Constructors for `r_xres`, `r_yres` and `r_fov` identify their CVar objects.
The camera and GUI signatures must agree with those objects. The integer
constructor identifies the related five typed CVar setters (float, integer,
boolean, narrow string and engine string).

Each setter starts with the verified complete six-byte instruction
`mov eax, fs:[0]`. A gateway copies that instruction and jumps back. The
fastcall wrapper forwards the original thiscall and argument, preserves its
boolean result, and enforces numeric values only after a successful assignment
to one of the three target CVars. Other variables are left alone. The CVar's
string representations are retained, so the engine's normal configuration
writer can preserve the user's original settings.

With `BaseFOV=0`, every newly assigned config focal value is corrected once:

```text
runtime_focal = assigned_focal * previous_executable_aspect / selected_aspect
```

For a clean executable, the previous aspect is 4:3. For a recognized older
project patch, its aspect recovers the earlier focal baseline. A positive
`BaseFOV` instead supplies an explicit 4:3 baseline. This is a focal parameter,
not an angle. Camera rendering and world picking use the same corrected CVar.

Two zero resolution values select the current physical desktop mode of the
monitor under the mouse pointer at startup. Explicit values set the resolution
instead. Resolution overrides remain active for the process; restart after
editing the INI and avoid changing display settings in the old game menu.

## Menu, HUD and mouse

The proportional mode reuses the validated GUI coordinate conversion approach:

```text
scale = min(width / 1024, height / 768)
canvas_width = width / scale
canvas_height = height / scale
root_x = (canvas_width - 1024) / 2
root_y = (canvas_height - 768) / 2
```

Drawing, inverse coordinates, mouse mapping and outer clipping are adjusted
together. Root translation stubs and a private vertical reciprocal occupy
verified alignment gaps. Stretched mode uses the recognized original GUI
structures and retains the gameplay correction.

## Memory writes and diagnostics

The plugin builds the entire plan before modifying game memory. It checks
expected bytes, rejects overlapping writes, acquires all required page
permissions, writes the plan, flushes the instruction cache and restores page
protections. The plugin is pinned before permanent branches target it.
Gateways become executable/read-only before installation. Recognition or
permission failures leave the game patch plan unapplied. Initialization is
intended to occur before normal engine startup through the bundled loader;
hot injection and unloading are not supported.

The plugin writes only its own log beside the ASI. It does not write the game
EXE or CFG. The game itself can still save its normal configuration. Removing
the ASI ends these overrides on the next launch; patches installed on disk by
earlier tools must be restored separately.

## Build and tests

From an x86 Visual Studio build environment:

```powershell
.\Emergency4\build-asi-release.ps1
```

Alternatively pass `-Compiler` with an x86 MinGW g++ path. The build pins the
official loader archive and verifies SHA-256, checks x86 headers, compiles with
warnings treated as errors, and exercises the real loader and plugin in an
unrelated DirectInput host. No game binary is distributed or started by tests.

Private tests require Python 3.10+, `pefile`, `capstone` and `unicorn`:

```powershell
python Emergency4/asi/tests/verify_asi.py --original-exe D:/Fixtures/game.exe --legacy-exe D:/Fixtures/older-patched.exe --bin-dir Emergency4/asi/.work/build --loader Emergency4/asi/.work/build/loader/dinput8.dll --work-dir D:/PrivateTests/new-run --report Emergency4/docs/validation-asi.json
```

The native test host uses a synthetic rebased mapped fixture and substitutes
only the original setter continuations. The actual detours, wrappers, copied
FS-load gateways and return/stack cleanup execute as native x86. Original GUI
methods and hit testing execute in Unicorn across multiple resolutions.

Only one actual executable was available. These tests do not establish support
for every regional release, full engine startup, complete rendering or modded
interfaces. **In-game visual validation is still required; this is a beta.**
