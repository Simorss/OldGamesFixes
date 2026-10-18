"""Actual x86 GUI/hit-testing emulation for the available private fixture."""
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_ESP,
    UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_EAX,
)

RESOLUTIONS = [(640,480),(1024,768),(1280,1024),(1280,720),(1920,1080),
               (2560,1440),(3440,1440),(3840,1600),(5120,1440),
               (3840,2160),(3840,1080),(1080,1920)]

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
    root_target = 0x934477+5+struct.unpack_from('<i', exe, pe.get_offset_from_rva(0x534478))[0]
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
    assert abs(struct.unpack_from('<f', exe, pe.get_offset_from_rva(0x78917))[0]-w/h) < 1e-6
    return {'resolution': f'{w}x{h}', 'mode': mode, 'gui_scale': [sx,sy],
            'gui_origin_px': [origin_x,origin_y], 'click_tests': 5, 'status': 'PASS'}

