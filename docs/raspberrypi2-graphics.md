# Raspberry Pi 2 graphics diagnosis (2026-11-14)

Validated over SSH on the Raspberry Pi 2 running Linux 6.12.93-v7, Xorg
21.1.18 and Mesa 25.1.6. The operator confirmed the visible test colours,
the Eris dashboard and the Qt /graphic-api-test application in a container.
No kernel, firmware, U-Boot binary, boot script or persistent environment
was changed on the board during these tests.

## Two distinct failures

### Black output caused by the ARM32 identity CTM

The modesetting driver initialized its identity colour transformation matrix
with `1UL << 32`. On ARM32, unsigned long has only 32 bits, so this does not
produce the required fixed-point representation of 1.0. A zero colour matrix
can make an otherwise functioning X server display only black.

The state on this board showed an enabled HDMI connector, active CRTC, and
an X-owned 1024x600 framebuffer. X answered xset/xrandr queries, but neither
the dashboard nor a red root background was visible. Disabling UseGammaLUT
(which also disables the driver's CTM path) restored visible output.

The Xorg bbappend backports the identity-constant fix using `1ULL << 32`:
https://gitlab.freedesktop.org/xorg/xserver/-/commit/69beb656539d5f9f82ec501f42245d0f43b709d8

The rebuilt modesetting module was loaded from /tmp with normal colour
management enabled. Its CTM property contained the correct identity matrix,
and the operator confirmed a blue background. The fix is applicable to
Xorg 21.1.18 generally; it is not limited to the Pi kernel or U-Boot.

### Glamor/EGL initialization hang

With the CTM fix applied, enabling Glamor still left X unresponsive after:

    glamor: Ignoring GL < 2.1, falling back to GLES.

GDB with the matching Mesa symbols captured this path:

    futex_wait
    do_futex_fence_wait
    _util_queue_fence_wait
    _mesa_glthread_finish
    st_destroy_context
    dri_destroy_context / driDestroyContext
    dri2_destroy_context
    dri2_make_current
    eglMakeCurrent
    glamor_egl_init

This locates the hang in context teardown during the GL-to-GLES switch.
It does not establish a complete upstream root cause. Disabling mesa_glthread,
preloading libEGL, and experimentally unbinding the first GL context before
destroying it did not resolve the hang. The experimental source change was
removed, and the final Xorg artifacts were rebuilt without it.

The Pi 2-specific X configuration therefore selects modesetting with
`AccelMethod "none"` through an OutputClass matching vc4. KMS display output
and normal colour management remain enabled. This is a workaround for the
tested stack: reassess it when Mesa/Xorg changes. Other board configurations
are not assigned this option.

## Validation and runtime state

* The CTM initializer's original invalid shift was reproduced in a 32-bit
  compile; the corrected constants compiled in both 32-bit and 64-bit modes.
* BitBake builds and package QA passed for xserver-xorg and xserver-nodm-init
  in build-raspberrypi2-graphical. The packaged 99-vc4.conf was checked against
  the source configuration.
* The actual Pi 2 OutputClass configuration was tested with automatic device
  selection, using the corrected module. xset and xrandr responded, and the
  correct CTM was exposed without disabling UseGammaLUT.
* The operator confirmed the normal dashboard including its clock and the
  Qt application in the temporary eris-qt-display-diag container.
* This validates these displayed workloads, not hardware-accelerated client
  OpenGL, Qt Quick, or performance. X server Glamor acceleration is disabled
  by the workaround.

The board's live test uses temporary modules/configuration under /tmp and a
diagnostic X invocation. It does not survive reboot. Rebuild and install the
updated rootfs to make these recipe changes permanent; no bootloader update
is required. No cold-boot test of that regenerated rootfs was performed.

Detailed runtime logs/backtraces were saved locally in
/tmp/eris-rpi2-display-evidence.tar.gz. Tests also found that the kernel boot
arguments still contain loglevel=0 and quiet despite eris-devel; that separate
development-console configuration issue was not changed by the graphics fix.
