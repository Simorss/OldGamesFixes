# Engine recognition and validation

## Recognition rather than a build allowlist

`EnginePatch.cs` parses the x86 PE headers, section mappings and imports.
Executable sections are searched for camera and GUI instruction signatures.
Absolute operands and relative branch targets are resolved from the actual
image base and section layout. No game-language, filename, edition label or
whole-executable hash is used to admit a fresh installation.

The current recognizer covers the Vision 7.1 / MSVC x86 instruction family found
in the available Emergency 4 fixture. Other builds with those routines can be
recognized even if addresses, file offsets, localized resources or filenames
differ. Different instruction encodings, packed/protected images and other
engine revisions need separate recognition profiles. Unique and consistent
matches are required; unrecognized or ambiguous code is not patched blindly.

## Camera

The main camera call is identified by its instruction sequence and verified
against the imported `PerspectiveToAngles@VisRenderer_cl` method. Its aspect
argument is replaced with `width / height`.

```text
new_r_fov = original_r_fov * (4/3) / (width/height)
```

`r_fov` is a focal parameter rather than an angle. It also feeds world picking.
The original focal value is retained in backup state so subsequent applications
do not compound the scale. Earlier patches are normalized from their engine
structures; an existing matching original backup can recover the exact focal
value, otherwise the previous aspect and config value are used to infer it.

## Menu and HUD

Four coordinate-conversion methods are identified by their instruction shape,
shared width/height globals and consistent scale constants. A separate mouse
conversion signature must use the same globals and dimensions. The root GUI
constructor and translation initializer are located and verified independently.

```text
scale = min(width / 1024, height / 768)
canvas_width = width / scale
canvas_height = height / scale
root_x = (canvas_width - 1024) / 2
root_y = (canvas_height - 768) / 2
```

Drawing, inverse coordinates, mouse bounds and outer clipping are adjusted
together. Two 13-byte trampolines occupy recognized 15-byte alignment gaps after
the forward conversion methods; another gap stores a private vertical
reciprocal. Existing GUI pointer instructions are redirected to it, avoiding
changes to unrelated users of the shared vertical reciprocal.

The original root content remains 1024x768. Its inherited translation centers
child dialogs without rewriting XML. Stretched mode restores the recognized
original UI structures and applies only the gameplay correction.

All patch offsets, cave addresses, data operands and branch destinations come
from the detected image. The implementation has no table of Italian executable
offsets. Recognized earlier trampolines are normalized before repatching.

## Backup metadata

Schema 2 stores executable/configuration filenames, original snapshot hashes,
a normalized engine baseline and the last installed hash. Version 1.0 state is
upgraded without replacing its original snapshots. Hashes protect backup
integrity and prevent silently overwriting unrelated edits after installation;
they do not restrict fresh installations to particular releases or languages.

Files are replaced atomically and ordinary write failures roll back their
contents. This is not a cross-file filesystem transaction; retained backups are
still required for recovery after interruption or power loss.

## Developer tests

Requirements: Python 3.10+, `pefile`, `capstone`, `unicorn`, and Windows
PowerShell 5.1. PowerShell 7 is also smoke-tested when installed.

```powershell
python tests/verify_fix.py --original-exe 'D:\PrivateFixtures\Em4Deluxe.exe' --original-config 'D:\PrivateFixtures\em4deluxe.cfg' --work-dir tests/.work/run-001 --report docs/validation.json
```

The work directory must not already exist. It contains private fixture copies;
never commit those game files. Optional legacy fixture arguments enable
migration tests. No game is launched and no UI automation is used.

Tests execute actual x86 GUI methods in Unicorn, not just mathematical copies.
They check multiple resolutions and modes, hit testing, interactive English
input, byte-exact restore, config encodings, engine ambiguity and missing
signatures. Resource-modified/renamed fixtures demonstrate removal of the hash
allowlist. Raw-section repacking tests file-offset independence with GUI
emulation. A synthetic rebased-image test verifies scanner address handling;
it is not presented as a fully runnable alternative game release.

Only one actual game executable was available. Emulation and synthetic variants
do not validate every regional release, DRM wrapper, mod interface or complete
renderer. Additional real builds and visual in-game checks remain necessary.
