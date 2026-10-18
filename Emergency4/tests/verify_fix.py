"""Integration + x86 emulation tests. Game files are private, caller-supplied fixtures.

Requires Python 3.10+, pefile, unicorn, and Windows PowerShell 5.1.
Run --help for fixture and report paths. No UI automation or game launch.
"""
import argparse
import hashlib
import json
import math
import pathlib
import shutil
import struct
import subprocess
import sys

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_ESP,
    UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_EAX,
)

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'src' / 'Emergency4-Fix.ps1'
CANONICAL = '17924F0E847FB0173DDBD5FDB5D5CBBF68F75CF1DB6FDCADB155F964A350FDC9'
RESOLUTIONS = [(640,480),(1024,768),(1280,1024),(1280,720),(1920,1080),
               (2560,1440),(3440,1440),(3840,1600),(5120,1440),
               (3840,2160),(3840,1080),(1080,1920)]


def sha(data):
    return hashlib.sha256(data).hexdigest().upper()


def config_text(data):
    if data.startswith(b'\xff\xfe'):
        return data.decode('utf-16')
    if data.startswith(b'\xfe\xff'):
        return data.decode('utf-16')
    return data.decode('utf-8-sig' if data.startswith(b'\xef\xbb\xbf') else 'cp1252')


def config_values(data):
    import re
    return dict(re.findall(r'<var\s+name="([^"]+)"\s+value="([^"]*)"\s*/>', config_text(data)))


def emulate(exe, w, h, mode):
    """Execute actual original/modified engine methods, not Python copies of their logic."""
    pe = pefile.PE(data=exe)
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0, 0x1000)
    u.mem_map(0x400000, 0xb00000)
    u.mem_write(0x400000, pe.get_memory_mapped_image())
    u.mem_map(0x10000000, 0x20000)
    u.mem_map(0x20000000, 0x20000)
    u.mem_map(0x30000000, 0x1000)
    u.reg_write(UC_X86_REG_FPCW, 0x37f)
    u.reg_write(UC_X86_REG_FPSW, 0)
    u.mem_write(0x30000100, b'\xd9\x1d'+struct.pack('<I', 0x2001f000)+b'\x90')

    def word(x):
        return struct.pack('<f' if isinstance(x, float) else '<I', x)

    def readf(addr):
        return struct.unpack('<f', u.mem_read(addr, 4))[0]

    def call(va, this, *args, fpu=False):
        sp = 0x10010000
        ret = 0x30000100 if fpu else 0x30000000
        stop = ret+6 if fpu else ret
        u.mem_write(sp, word(ret)+b''.join(word(x) for x in args))
        u.reg_write(UC_X86_REG_ESP, sp)
        u.reg_write(UC_X86_REG_ECX, this)
        u.emu_start(va, stop, count=100000)
        assert u.reg_read(UC_X86_REG_EIP) == stop
        return readf(0x2001f000) if fpu else u.reg_read(UC_X86_REG_EAX)

    u.mem_write(0xbd9314, word(float(w)))
    u.mem_write(0xbd9384, word(float(h)))
    root, dialog, button, merged = 0x20000000, 0x20001000, 0x20002000, 0x20003000
    for obj in [root, dialog, button]:
        call(0x938630, obj+0x30)
    root_target = 0x934477+5+struct.unpack_from('<i', exe, 0x534478)[0]
    call(root_target, root, 0., 0.)
    # Use the real root vtable and SetRect callback, as in the GUI constructor.
    u.mem_write(root, word(0xa30e20))
    call(0x93a120, root, 0., 0., 1024., 768.)
    call(0x939610, dialog+0x30, 0., 0.)
    call(0x938260, dialog+0x30, 0., 0., 1024., 768.)
    u.mem_write(dialog+0x2c, word(root))
    call(0x939610, button+0x30, 259., 251.)
    call(0x938260, button+0x30, 0., 0., 156., 43.)
    u.mem_write(button+0x2c, word(dialog))
    call(0x938630, merged)
    call(0x93ac90, button, merged)
    ox, oy = 0x2001e000, 0x2001e004
    call(0x938520, merged, 0., 0., ox, oy)
    left, top = readf(ox), readf(oy)
    sx, sy = w/1024, h/768
    if mode == 'Original':
        sx = sy = min(sx, sy)
    origin_x, origin_y = (w-1024*sx)/2, (h-768*sy)/2
    assert abs(left-(origin_x+259*sx)) < .01
    assert abs(top-(origin_y+251*sy)) < .01
    call(0x938570, 0, 156., 43., ox, oy)
    assert abs(readf(ox)-156*sx) < .01
    assert abs(readf(oy)-43*sy) < .01
    for dx, dy, expected in [(78.,21.5,True),(-1.,21.5,False),
                             (157.,21.5,False),(78.,-1.,False),(78.,44.,False)]:
        px, py = left+dx*sx, top+dy*sy
        vx = call(0x938430, 0, float(px), fpu=True)
        vy = call(0x938450, 0, float(py), fpu=True)
        hit = bool(call(0x9389a0, merged, float(vx), float(vy)) & 0xff)
        assert hit == expected, (w, h, mode, px, py, vx, vy, hit)
    assert abs(struct.unpack_from('<f', exe, 0x78917)[0]-w/h) < 1e-6
    return {'resolution': f'{w}x{h}', 'mode': mode, 'gui_scale': [sx,sy],
            'gui_origin_px': [origin_x,origin_y], 'click_tests': 5, 'status': 'PASS'}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--original-exe', required=True, type=pathlib.Path)
    ap.add_argument('--original-config', required=True, type=pathlib.Path)
    ap.add_argument('--legacy-exe', type=pathlib.Path)
    ap.add_argument('--legacy-config', type=pathlib.Path)
    ap.add_argument('--work-dir', required=True, type=pathlib.Path)
    ap.add_argument('--report', required=True, type=pathlib.Path)
    ap.add_argument('--shell', default='powershell.exe')
    a = ap.parse_args()
    original, cfg = a.original_exe.read_bytes(), a.original_config.read_bytes()
    assert sha(original) == CANONICAL
    # New dedicated fixture directory only. Never overwrite or remove existing fixtures.
    a.work_dir.mkdir(parents=True, exist_ok=False)

    def fixture(name, data=original, configuration=cfg):
        path = a.work_dir / name
        path.mkdir()
        (path/'Em4Deluxe.exe').write_bytes(data)
        (path/'em4deluxe.cfg').write_bytes(configuration)
        return path

    def run(path, *extra, success=True):
        p = subprocess.run([a.shell,'-NoProfile','-ExecutionPolicy','Bypass','-File',str(SCRIPT),
                            '-GamePath',str(path),'-NonInteractive',*map(str,extra)],
                           capture_output=True, text=True, encoding='utf-8', errors='replace')
        assert (p.returncode == 0) == success, p.stdout+'\n'+p.stderr
        return p

    game = fixture('all resolutions')
    results = []
    baseline = float(config_values(cfg)['r_fov'])
    for w, h in RESOLUTIONS:
        for mode in ['Original','Stretched']:
            run(game,'-Width',w,'-Height',h,'-InterfaceMode',mode)
            data = (game/'Em4Deluxe.exe').read_bytes()
            current_config = (game/'em4deluxe.cfg').read_bytes()
            values = config_values(current_config)
            assert values['r_xres'] == str(w) and values['r_yres'] == str(h)
            assert math.isclose(float(values['r_fov']),baseline*(4/3)/(w/h),rel_tol=1e-12)
            # Other config keys and BOM are preserved.
            before = config_values(cfg)
            assert {k:v for k,v in values.items() if k not in ['r_fov','r_xres','r_yres']} == {
                k:v for k,v in before.items() if k not in ['r_fov','r_xres','r_yres']}
            assert current_config[:2] == cfg[:2]
            results.append(emulate(data,w,h,mode))
    run(game,'-Restore')
    assert (game/'Em4Deluxe.exe').read_bytes() == original
    assert (game/'em4deluxe.cfg').read_bytes() == cfg

    # Preview writes nothing, including no backup folder.
    preview = fixture('preview')
    run(preview,'-Width',1920,'-Height',1080,'-InterfaceMode','Original','-Preview')
    assert sorted(x.name for x in preview.iterdir()) == ['Em4Deluxe.exe','em4deluxe.cfg']
    assert (preview/'Em4Deluxe.exe').read_bytes() == original

    # Unsupported/corrupted builds and invalid dimensions leave input untouched.
    bad = bytearray(original); bad[0x10000] ^= 1
    rejected = fixture('unsupported',bytes(bad))
    run(rejected,'-Width',1920,'-Height',1080,'-InterfaceMode','Original',success=False)
    assert (rejected/'Em4Deluxe.exe').read_bytes() == bad
    invalid = fixture('invalid dimensions')
    run(invalid,'-Width',-1,'-Height',1080,'-InterfaceMode','Original',success=False)
    assert (invalid/'Em4Deluxe.exe').read_bytes() == original

    # Encoding preservation on clean installations.
    for name, encoding in [('utf8bom','utf-8-sig'),('ansi','cp1252'),('utf16be','utf-16-be')]:
        data = config_text(cfg).encode(encoding)
        if name == 'utf16be': data=b'\xfe\xff'+data
        path = fixture(name,configuration=data)
        run(path,'-Width',2560,'-Height',1440,'-InterfaceMode','Original')
        assert config_values((path/'em4deluxe.cfg').read_bytes())['r_xres'] == '2560'
        run(path,'-Restore')
        assert (path/'em4deluxe.cfg').read_bytes() == data

    checks = ['24 resolution/mode combinations','120 geometric hit tests',
              'resolution and mode changes without cumulative FOV',
              'exact executable/config restoration','preview without writes',
              'unknown executable rejection','invalid dimension rejection',
              'UTF-16 LE/BE, UTF-8 BOM and ANSI configs']
    # Exercise the actual interactive path with all answers supplied through stdin.
    interactive = fixture('interactive path with spaces')
    proc = subprocess.run([a.shell,'-NoProfile','-ExecutionPolicy','Bypass','-File',str(SCRIPT)],
                          input=f'1\n{interactive.resolve()}\n1920x1080\n1\n',
                          capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=30)
    assert proc.returncode == 0, proc.stdout+proc.stderr
    interactive_cfg = config_values((interactive/'em4deluxe.cfg').read_bytes())
    assert interactive_cfg['r_xres'] == '1920' and interactive_cfg['r_yres'] == '1080'
    assert 'menu/HUD: Original' in proc.stdout
    checks += ['interactive action, game-path, resolution and UI prompts via stdin']
    if shutil.which('pwsh.exe'):
        smoke = fixture('PowerShell 7 smoke')
        args7 = ['pwsh.exe','-NoProfile','-File',str(SCRIPT),'-GamePath',str(smoke),'-NonInteractive']
        for suffix in [['-Width','1080','-Height','1920','-InterfaceMode','Original'],['-Restore']]:
            proc = subprocess.run(args7+suffix,capture_output=True,text=True,encoding='utf-8',errors='replace')
            assert proc.returncode == 0, proc.stdout+proc.stderr
        assert (smoke/'Em4Deluxe.exe').read_bytes() == original
        assert (smoke/'em4deluxe.cfg').read_bytes() == cfg
        checks += ['PowerShell 7 portrait apply/restore smoke test']
    if a.legacy_exe and a.legacy_config:
        old_exe, old_cfg = a.legacy_exe.read_bytes(), a.legacy_config.read_bytes()
        path = fixture('legacy migration',old_exe,old_cfg)
        run(path,'-Width',3440,'-Height',1440,'-InterfaceMode','Original')
        migrated = (path/'Em4Deluxe.exe').read_bytes()
        emulate(migrated,3440,1440,'Original')
        state_path = path/'OldGamesFixes-Emergency4-Backup'/'state.json'
        assert json.loads(state_path.read_text(encoding='utf-8-sig'))['OriginalFocalLength'] == 350
        first = sha(migrated)
        run(path,'-Width',3440,'-Height',1440,'-InterfaceMode','Original')
        assert sha((path/'Em4Deluxe.exe').read_bytes()) == first
        run(path,'-Restore')
        assert (path/'Em4Deluxe.exe').read_bytes() == old_exe
        assert (path/'em4deluxe.cfg').read_bytes() == old_cfg
        checks += ['legacy fixed-3440 migration','idempotent application','exact legacy restore']
    result = {'version':'1.0.0-beta.1','status':'PASS','shell':a.shell,
              'method':'Private fixtures + actual x86 GUI methods executed in Unicorn; no game launch or UI automation',
              'checks':checks,'cases':results,
              'limitations':'Visual in-game validation is pending for the generalized menu/HUD patch and resolutions other than 3440x1440.'}
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':'PASS','cases':len(results),'checks':checks},indent=2))


if __name__ == '__main__':
    main()
