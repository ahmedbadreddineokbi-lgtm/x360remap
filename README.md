# X360Remap

A fork of [hiddriver360](https://github.com/EinTim23/hiddriver360) by EinTim23, adding **mouse and keyboard support** to Xbox 360 consoles running RGH/JTAG (DashLaunch), merged with native controller input.

## What this is

X360Remap is made of two pieces:

- **X360Remap Core** (`hiddriver.xex`) — a DashLaunch plugin that reads USB HID mouse/keyboard input and turns it into Xbox 360 controller input, system-wide.
- **X360Remap Studio** (`application.xex`) — a small on-console configuration app (Dear ImGui) to bind keys, tune mouse sensitivity, and manage per-game profiles, so you don't have to hand-edit a config file.

**Must have** Xbdm.exe need to be activated, without it X360Remap cause the console to freeze, I tried everything to avoid that dependence but I couldn't find a permanent solution every log stop at the moment of the freez with no clear cause.

Built for players who want to use mouse and keyboard on original Xbox 360 hardware, no dongle or adapter involved.

## Status

This is a solo, spare-time project, and it's honestly not yet where I'd like it to be. Core input remapping, per-game profiles, and mouse calibration all work on real hardware, but the interface is still early and rough around the edges, and not every mouse/keyboard combination has been tested. Expect bugs, and please don't expect a finished product. Feedback and issue reports are genuinely welcome — that's how this gets better over time.

## Getting started

- Copy `hiddriver.xex` to `Hdd1:\` and load it as a DashLaunch plugin.
- Build `application/` into `X360RemapStudio.xex`, copy it to `Hdd1:\X360RemapStudio\`, and launch it from Aurora content as an application add it then access it to configure your devices.
- The app has a built-in "How it works" tab covering setup and the in-game quick-bind shortcut.


## Credits

Built on top of [hiddriver360](https://github.com/EinTim23/hiddriver360) by EinTim23 — thanks for the original USB HID groundwork this fork builds on.

## Support


If this project is useful to you, a bit of support goes a long way for a solo developer working on this in his free time:

- PayPal: x360remap@gmail.com
- Ko-fi: https://ko-fi.com/x360remap


## License

GPLv3 — see [LICENSE](LICENSE).
