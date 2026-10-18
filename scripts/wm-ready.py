"""Wait for EWMH atoms on the local virtual display, without changing X state."""
import ctypes
import time
x11 = ctypes.CDLL('libX11.so.6')
x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
x11.XInternAtom.restype = ctypes.c_ulong
x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
for attempt in range(100):
    display = x11.XOpenDisplay(None)
    if display:
        ready = all(x11.XInternAtom(display, name, 1) for name in
                    (b'WM_PROTOCOLS', b'_NET_WM_STATE', b'_NET_SUPPORTING_WM_CHECK'))
        x11.XCloseDisplay(display)
        if ready:
            break
    time.sleep(.05)
else:
    raise SystemExit('Virtual desktop did not become ready')
