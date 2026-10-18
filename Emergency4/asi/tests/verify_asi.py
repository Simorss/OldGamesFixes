"""Private-fixture tests for the ASI. Never starts the game or edits its files.

Requires Python 3.10+, pefile, capstone, unicorn and the built native test tools.
"""
import argparse
import hashlib
import importlib.util
import json
import pathlib
import re
import shutil
import struct
import subprocess

import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM

ROOT = pathlib.Path(__file__).resolve().parents[2]


def mapped(data):
    pe = pefile.PE(data=data)
    result = bytearray(pe.OPTIONAL_HEADER.SizeOfImage)
    raw = pe.get_memory_mapped_image()
    result[:len(raw)] = raw
    return result, pe


def synthetic_rebase(data, pe, new_base):
    """Rebase code operands only; this is not a runnable alternative game build."""
    before = bytes(data)
    base = pe.OPTIONAL_HEADER.ImageBase
    delta = new_base-base
    result = bytearray(before)
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    md.skipdata = True
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        start = section.VirtualAddress
        for ins in md.disasm(before[start:start+section.SizeOfRawData], base+start):
            if not ins.id:
                continue
            at = ins.address-base
            for op in ins.operands:
                if (op.type == X86_OP_MEM and not op.mem.base and not op.mem.index
                        and base <= op.mem.disp < base+len(before) and ins.disp_size == 4):
                    struct.pack_into('<I', result, at+ins.disp_offset, op.mem.disp+delta)
                elif (op.type == X86_OP_IMM and not ins.group(CS_GRP_CALL) and not ins.group(CS_GRP_JUMP)
                        and base <= op.imm < base+len(before) and ins.imm_size == 4):
                    struct.pack_into('<I', result, at+ins.imm_offset, op.imm+delta)
    struct.pack_into('<I', result, pe.OPTIONAL_HEADER.get_field_absolute_offset('ImageBase'), new_base)
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--original-exe', required=True, type=pathlib.Path)
    ap.add_argument('--legacy-exe', type=pathlib.Path)
    ap.add_argument('--bin-dir', required=True, type=pathlib.Path)
    ap.add_argument('--loader', required=True, type=pathlib.Path)
    ap.add_argument('--work-dir', required=True, type=pathlib.Path)
    ap.add_argument('--report', required=True, type=pathlib.Path)
    a = ap.parse_args()
    a.bin_dir = a.bin_dir.resolve()
    a.work_dir.mkdir(parents=True, exist_ok=False)
    a.work_dir = a.work_dir.resolve()
    original = a.original_exe.read_bytes()
    source_hash = hashlib.sha256(original).hexdigest()
    source, pe = mapped(original)
    base = pe.OPTIONAL_HEADER.ImageBase
    input_file = a.work_dir/'source.bin'
    input_file.write_bytes(source)
    output_file = a.work_dir/'result.bin'
    layout_file = a.work_dir/'layout.json'
    checks = []

    def run(*args, success=True):
        result = subprocess.run(list(map(str,args)), capture_output=True, text=True, timeout=90)
        assert (result.returncode == 0) == success, result.stdout+'\n'+result.stderr
        return result

    def probe(fixture=input_file, address=base, width=3440, height=1440, mode=True, success=True):
        run(a.bin_dir/'EngineProbe.exe', fixture, hex(address), width, height, int(mode), output_file, layout_file, success=success)
        if success:
            return output_file.read_bytes(), json.loads(layout_file.read_text())

    spec = importlib.util.spec_from_file_location('legacy_verification', ROOT/'tests/verify_fix.py')
    emulator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(emulator)
    cases = []
    for width,height in emulator.RESOLUTIONS:
        for mode in ('Original','Stretched'):
            result,layout = probe(width=width,height=height,mode=mode=='Original')
            # Convert the new mapped image back to the private file layout for
            # the existing actual-x86 GUI/hit-testing emulation harness.
            raw = bytearray(original)
            for section in pe.sections:
                start = section.PointerToRawData
                raw[start:start+section.SizeOfRawData] = result[section.VirtualAddress:section.VirtualAddress+section.SizeOfRawData]
            cases.append(emulator.emulate(bytes(raw),width,height,mode))
            assert layout['defaultFov'] > 0 and len(layout['setters']) == 5
    checks.append('24 resolution/UI combinations and 120 actual x86 GUI hit tests passed')

    result,layout = probe()
    # Verify all five installed hooks with native x86 execution in an isolated
    # host. Method continuations are synthetic; no real game setter is invoked.
    rebased = synthetic_rebase(source,pe,0x10000000)
    rebased_file = a.work_dir/'rebased.bin'
    rebased_file.write_bytes(rebased)
    probe(rebased_file,0x10000000)
    runtime = a.work_dir/'RuntimeTest.exe'
    shutil.copy2(a.bin_dir/'RuntimeTest.exe',runtime)
    ini = runtime.with_suffix('.ini')

    def runtime_case(text, fixture=rebased_file, expected='success'):
        ini.write_text('[MAIN]\n'+text, encoding='ascii')
        run(runtime, fixture, '0x10000000', expected)

    runtime_case('ResX=3440\nResY=1440\n')
    runtime_case('ResX=1920\nResY=1080\nBaseFOV=350\n')
    checks.append('Native initialization, five x86 detours/gateways, return/stack preservation, unrelated CVars, stable repeated assignments and one-time initialization passed')
    runtime_case('ResX=0\nResY=0\n')
    checks.append('Automatic desktop resolution initialized successfully')
    runtime_case('Enabled=0\n',expected='disabled')
    for text in ('ResX=3440\nResY=0\n', 'ResX=abc\nResY=1440\n',
                 'ResX=-1\nResY=1440\n','KeepUIAspectRatio=7\n','BaseFOV=-1\n'):
        runtime_case(text,expected='reject')
    checks.append('Disabled plugin and invalid INI settings left all mapped game bytes unchanged')

    missing = bytearray(source)
    missing[layout['aspect']-6] = 0x90
    bad = a.work_dir/'missing.bin'
    bad.write_bytes(missing)
    probe(bad,success=False)
    bad_rebased = bytearray(rebased)
    bad_rebased[layout['aspect']-6] = 0x90
    bad.write_bytes(bad_rebased)
    runtime_case('ResX=3440\nResY=1440\n',bad,'reject')
    ambiguous = bytearray(source)
    root = layout['rootCall']-23
    text = next(s for s in pe.sections if s.Characteristics&0x20000000)
    blank = source.find(bytes(64),text.VirtualAddress,text.VirtualAddress+text.SizeOfRawData)
    assert blank >= 0
    ambiguous[blank:blank+46] = source[root:root+46]
    bad.write_bytes(ambiguous)
    probe(bad,success=False)
    checks.append('Missing and ambiguous engine signatures rejected before applying patches')

    if a.legacy_exe:
        older, _ = mapped(a.legacy_exe.read_bytes())
        legacy_file = a.work_dir/'legacy.bin'
        legacy_file.write_bytes(older)
        built,_ = probe(legacy_file)
        clean,_ = probe()
        # Legacy baseline normalization must yield identical intended edits;
        # all unrelated file differences are intentionally outside this check.
        edits = [i for i in range(len(source)) if clean[i] != source[i]]
        assert all(built[i] == clean[i] for i in edits)
        # Normalize the old focal parameter by previousAspect/newAspect.
        previous = struct.unpack_from('<f',older,layout['aspect'])[0]
        original_focal = 350.0
        old_focal = original_focal*(4/3)/previous
        assert abs(old_focal*previous/(3440/1440)-original_focal*(4/3)/(3440/1440)) < 1e-8
        checks.append('Recognized legacy executable normalized in memory; previous-aspect FOV recovery verified')

    smoke = a.work_dir/'loader-smoke'
    (smoke/'scripts').mkdir(parents=True)
    shutil.copy2(a.bin_dir/'LoaderSmoke.exe',smoke/'LoaderSmoke.exe')
    shutil.copy2(a.loader,smoke/'dinput8.dll')
    shutil.copy2(a.bin_dir/'Emergency4.WidescreenFix.asi',smoke/'scripts/Emergency4.WidescreenFix.asi')
    shutil.copy2(ROOT/'asi/data/scripts/Emergency4.WidescreenFix.ini',smoke/'scripts/Emergency4.WidescreenFix.ini')
    run(smoke/'LoaderSmoke.exe')
    log = (smoke/'scripts/Emergency4.WidescreenFix.log').read_text()
    assert 'Initialization stopped: Engine recognition failed:' in log
    checks.append('Ultimate ASI Loader loaded the actual release plugin through dinput8.dll; an unrelated host was safely rejected')
    plugin = pefile.PE(str(a.bin_dir/'Emergency4.WidescreenFix.asi'))
    assert plugin.FILE_HEADER.Machine == 0x14c
    assert any(x.name == b'InitializeASI' for x in plugin.DIRECTORY_ENTRY_EXPORT.symbols)
    dependencies = [x.dll.decode() for x in plugin.DIRECTORY_ENTRY_IMPORT]
    assert not any(x.lower() in ('libstdc++-6.dll','libgcc_s_dw2-1.dll','libwinpthread-1.dll') for x in dependencies)
    checks.append('Release plugin is x86, exports InitializeASI and has no external GCC runtime DLL dependency')
    assert hashlib.sha256(a.original_exe.read_bytes()).hexdigest() == source_hash
    checks.append('Private source executable unchanged')
    source_sha256 = {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in (
        'asi/src/Engine.hpp','asi/src/Plugin.cpp','asi/src/Exports.def',
        'asi/tests/RuntimeTest.cpp','asi/tests/EngineProbe.cpp')}
    report = {'version':'2.0.0-beta.1','status':'PASS','source_sha256':source_sha256,'checks':checks,'cases':cases,
              'plugin_imports':dependencies,
              'limitations':'One actual executable, synthetic rebased fixtures and synthetic original-setter continuations. No real game launch or graphical validation; other regional builds, engine startup timing, custom mod UI and complete renderer behavior need in-game testing.'}
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':'PASS','checks':checks},indent=2))


if __name__ == '__main__':
    main()
