# This fork

This is a personal fork of [AxisNimble/TheFlightWall_OSS](https://github.com/AxisNimble/TheFlightWall_OSS),
restructured as an overlay on top of upstream so new upstream commits can
still be pulled in cleanly.

## Why

The upstream project drives a 20-panel WS2812B LED-matrix wall. This fork
targets much smaller hardware instead: a single SPI TFT. See
[`firmware/README.md`](firmware/README.md) for what's actually different in
the firmware (display adapter, AeroAPI caching, layout).

The ESP32-S3 Super Mini + 1.54" ST7789 target (`esp32s3_154`) exists because
that hardware was already sitting in a 3D-printed desktop enclosure -
[Simple Print Monitor ST7789 1.54" Display Case](https://makerworld.com/en/models/2501721-simple-print-monitor-st7789-1-54-display-case)
on MakerWorld - originally bought for a different local project
(BambuHelper), whose proven pin configuration for this exact board+panel
combo was reused directly rather than guessed from datasheets.

## Branch structure

- **`main`** — kept as an exact, untouched mirror of upstream `main`. Never
  commit directly here; it exists only so upstream updates have somewhere
  conflict-free to land.
- **`overlay`** (default branch of this fork) — `main` plus all the
  hardware-port customizations, as a single branch you build and flash from.

## Pulling in upstream updates

```sh
git fetch upstream
git checkout main
git merge upstream/main        # fast-forward, main never diverges locally
git push origin main

git checkout overlay
git merge main                 # bring upstream's changes into the overlay
# resolve any conflicts (most likely in firmware/README.md or
# firmware/platformio.ini, since both are edited on both sides), then:
git push origin overlay
```

If `upstream` isn't already configured as a remote:

```sh
git remote add upstream https://github.com/AxisNimble/TheFlightWall_OSS.git
```
