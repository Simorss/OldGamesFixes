# Implementation and validation

The patch is tied to the exact executable build identified in the README. PE32
image base is `0x400000`; the affected .text/.rdata RVAs equal their file offsets.
Every mutation is declared in `src/patches.json`. No executable is distributed.

## Gameplay

The main camera passes a fixed `4/3` aspect to
`VisRenderer_cl::PerspectiveToAngles`. Replace that float at file offset
`0x78917` with `width / height` and set the config's focal parameter to:

```text
new_r_fov = original_r_fov * (4/3) / (width/height)
```

`r_fov` is a focal parameter, not an angle in degrees. It is also consumed by
world ray/picking calculations. The initial focal parameter is retained in the
backup metadata so repeated applications do not compound the scale. Migration
from the two known earlier patches uses their original value of 350.

## Menu and HUD

The original UI uses a virtual 1024x768 canvas. In Original mode:

```text
scale = min(width / 1024, height / 768)
canvas_width  = width / scale
canvas_height = height / scale
root_x = (canvas_width  - 1024) / 2
root_y = (canvas_height - 768) / 2
```

Forward and inverse mapping constants, mouse bounds and the outer clipping
rectangle are updated together. The existing root GUI object's translation
initializer is routed through two 13-byte trampolines in 15-byte alignment gaps
at VA `0x938401` and `0x938421`. They replace its two stack arguments, then tail
jump to the original initializer. A private float at `0x938441` supplies the UI
vertical reciprocal; only GUI references to the old shared reciprocal are
redirected, preserving unrelated consumers of that constant.

The original root content size remains 1024x768. Its inherited translation
centers all child dialogs, including dynamically positioned elements, without
rewriting XML files. Stretched mode restores every UI-controlled byte to the
canonical build and only applies the gameplay changes.

## Integrity and restore

Only the canonical original executable, the two known earlier patch hashes,
and the last installed/backup hashes in validated local state are accepted.
All controlled byte ranges are normalized in memory and the complete original
SHA-256 must match before generating new output. Changes outside those ranges
are rejected. Backup files have separate SHA-256 checks. Each file is replaced
atomically; ordinary write failures trigger rollback to the pre-command bytes.
The two-file/state update is not a cross-file filesystem transaction; a power
loss can still require restoration from the retained backups.

No in-memory injection, game launch, UI automation, elevation, network request,
registry change or executable download is performed by the released patcher.

## Running developer tests

Install `pefile` and `unicorn` into a developer Python environment. Provide your
own original game files; never commit these fixtures. For example:

```powershell
python tests/verify_fix.py --original-exe 'D:\PrivateFixtures\Em4Deluxe.exe' --original-config 'D:\PrivateFixtures\em4deluxe.cfg' --work-dir tests/.work/run-001 --report docs/validation.json
```

The work directory must not exist. Tests create private fixture copies there
and never launch the game. Optional `--legacy-exe` and `--legacy-config` paths
enable migration tests. `--shell pwsh.exe` selects PowerShell 7.

Tests execute the actual patched GUI initializer, recursive parent coordinate
composition, real SetRect callback, pixel conversion, inverse conversion and
hit testing in Unicorn. Installation, repeated parameter changes, restoration,
preview, build rejection and config encodings are checked through the actual
PowerShell script. The report distinguishes these checks from pending visual
gameplay validation; emulation does not reproduce the complete renderer.
