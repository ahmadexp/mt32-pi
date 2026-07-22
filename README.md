[![mt32-pi CI](https://github.com/ahmadexp/mt32-pi/actions/workflows/ci.yml/badge.svg)](https://github.com/ahmadexp/mt32-pi/actions/workflows/ci.yml)

<h1 align="center">
    <img width="90%" title="mt32-pi - Baremetal synthesizer system" src="images/mt32pi_logo.svg">
</h1>

mt32-pi is a bare-metal MIDI synthesizer for Raspberry Pi. It runs without a general-purpose operating system and combines [Munt] MT-32 emulation with [FluidSynth] SoundFont synthesis on the [Circle] framework.

The firmware provides low-latency MIDI synthesis for vintage computers, [MiSTer FPGA], and standalone MIDI controllers. It supports MT-32/CM-32L ROM sets, [General MIDI], [Roland GS], [Yamaha XG], and user-supplied [SoundFonts][SoundFont].

---

## Supported hardware

<img title="mt32-pi running on the Raspberry Pi 3 A+ with the Arananet PI-MIDI HAT." width="280rem" align="right" src="images/mt32pi_pimidi.png">

- Raspberry Pi Zero 2 W, Raspberry Pi 3 Model A+/B/B+, Raspberry Pi 4 Model B, and Compute Module 4.
  * Raspberry Pi 2 is supported with reduced playback quality.
  * Raspberry Pi Zero (original) and Raspberry Pi 1 are not supported.
- PWM headphone-jack audio.
  * PWM output has known aliasing and distortion on quieter sounds; an I²S DAC is recommended for higher-quality output.
- [I²S Hi-Fi DAC support][I²S Hi-Fi DACs].
- MIDI input via [USB][USB MIDI interfaces], [GPIO][GPIO MIDI interface] MIDI interfaces, or the [serial port].
- [LCD status screen support][LCD and OLED displays] (for MT-32 SysEx messages and status information).
- [Physical control surface][control surface] using buttons and a rotary encoder.
- [MiSTer FPGA integration via user port][MiSTer FPGA].
- Network MIDI support via [RTP-MIDI] and [raw UDP socket].
- [Embedded FTP server][FTP server] for remote access to files.
- Automatic CPU power management during idle periods.

## Installation

Linux and MiSTer users can use the interactive installer in [`scripts`](scripts). For a manual installation:

1. Download the latest release from the [Releases] section.
    * If you are **updating an old version**, read the [Updating mt32-pi] wiki page for the correct procedure.
2. Extract contents to a blank [FAT32-formatted SD card][SD card preparation].
    * Read the [SD card preparation] wiki page for hints on formatting an SD card correctly (especially under Windows).
3. For MT-32 support, add legally obtained MT-32 or CM-32L ROM images to the `roms` directory.
    * You will need at least one control ROM and one PCM ROM.
    * For information on using multiple ROM sets and switching between them, see the [MT-32 synthesis] wiki page.
    * The file names or extensions don't matter; mt32-pi will scan and detect their types automatically.
4. Optionally add SoundFonts to the `soundfonts` directory.
    * For information on using multiple SoundFonts and switching between them, see the [SoundFont synthesis] wiki page.
    * Again, file names/extensions don't matter.
5. Edit `mt32-pi.cfg` to enable optional hardware such as I²S DACs, displays, and controls. See the [configuration reference][Configuration file].
    * **MiSTer users**: Read the [MiSTer setup] section of the wiki for the recommended configuration, and ignore the following two steps.
6. Connect a [USB MIDI interface][USB MIDI interfaces] or [GPIO MIDI circuit][GPIO MIDI interface] to the Pi, and connect some speakers to the headphone jack.
7. Connect your vintage PC's MIDI OUT to the Pi's MIDI IN and (optionally) vice versa.

## CPU power management

mt32-pi automatically enters power-saving mode after a configurable period without MIDI activity. The default timeout is 300 seconds and can be changed in `mt32-pi.cfg`:

```ini
power_save_timeout = 300
```

Set the value to `0` to disable automatic power saving. When the timeout expires, mt32-pi:

- Stops the audio device cleanly before changing the CPU clock.
- Parks the audio core and an idle display core using the processor's low-power wait-for-event instruction.
- Reduces the CPU clock through the Raspberry Pi firmware.
- Turns off supported LCD backlights.
- Continues polling the MiSTer control interface when it is enabled, because that interface does not have an interrupt-driven wake path.

Incoming MIDI, control-panel input, USB changes, and other registered activity restore the maximum CPU speed, restart audio, and wake parked cores. Firmware throttling and undervoltage events are also monitored and reported in the log and on an attached display.

## Building from source

The supported compiler release is [Arm GNU Toolchain] 15.2.Rel1, which provides GCC 15.2.1. CI downloads this release automatically and tests Raspberry Pi 2, Raspberry Pi 3 64-bit, and Raspberry Pi 4 64-bit builds with both the normal and HDMI consoles.

### Prerequisites

- Git and the repository submodules.
- GNU Make, CMake, `patch`, and standard Unix build tools.
- The `arm-none-eabi` toolchain for 32-bit targets.
- The `aarch64-none-elf` toolchain for 64-bit targets.

Download both toolchains from the [Arm GNU Toolchain] page and add their `bin` directories to `PATH`. For example:

```bash
export MT32PI_GCC32=/path/to/arm-gnu-toolchain-15.2.rel1-arm-none-eabi
export MT32PI_GCC64=/path/to/arm-gnu-toolchain-15.2.rel1-aarch64-none-elf
export PATH="$MT32PI_GCC32/bin:$MT32PI_GCC64/bin:$PATH"

arm-none-eabi-gcc --version
aarch64-none-elf-gcc --version
```

On macOS, the Circle configuration scripts also require GNU `getopt` and GNU installation tools. With Homebrew:

```bash
brew install cmake coreutils gnu-getopt
export PATH="$(brew --prefix gnu-getopt)/bin:$PATH"
```

### Build a firmware image

Initialize the dependencies once, then select a board:

```bash
make submodules
make -j4 BOARD=pi3-64
```

Available targets and their output image names are:

| `BOARD` | Architecture | Raspberry Pi | Output image |
| --- | --- | --- | --- |
| `pi2` | 32-bit | Pi 2 | `kernel7.img` |
| `pi3` | 32-bit | Pi 3 / Zero 2 W | `kernel8-32.img` |
| `pi3-64` | 64-bit | Pi 3 / Zero 2 W | `kernel8.img` |
| `pi4` | 32-bit | Pi 4 / CM4 | `kernel7l.img` |
| `pi4-64` | 64-bit | Pi 4 / CM4 | `kernel8-rpi4.img` |

The build also produces an ELF file for debugging and an Intel HEX image for serial flashing. Kernel images are gzip-compressed by default; set `GZIP_KERNEL=0` if an uncompressed image is required.

Build the HDMI-console variant by cleaning the application objects first:

```bash
make clean BOARD=pi3-64
make -j4 BOARD=pi3-64 HDMI_CONSOLE=1
```

Use `make clean BOARD=<board>` to rebuild only mt32-pi. Use `make mrproper BOARD=<current-board>` before switching board architectures or when a completely fresh dependency build is needed.

## Repository layout

- `src` and `include`: mt32-pi firmware source and headers.
- `sdcard`: runtime configuration and SD-card filesystem content.
- `scripts`: installation and maintenance utilities.
- `patches`: dependency compatibility patches applied by the build.
- `external`: Git submodules for Circle, synthesizer engines, and supporting libraries.
- `Kernel.mk`, `Config.mk`, and `Makefile`: target configuration and build orchestration.

## Technical references

- [Configuration file]
- [MT-32 synthesis]
- [SoundFont synthesis]
- [Audio DACs][I²S Hi-Fi DACs]
- [MIDI interfaces][USB MIDI interfaces]
- [Displays][LCD and OLED displays]
- [MiSTer setup]
- [Networking][RTP-MIDI]
- [Complete project wiki][mt32-pi wiki]

## Core components

- [Munt] provides Roland MT-32 emulation.
- [FluidSynth] provides SoundFont synthesis.
- [Circle] and [circle-stdlib] provide the bare-metal Raspberry Pi runtime.
- [inih] provides configuration-file parsing.
- [GeneralUser GS] provides the bundled General MIDI/Roland GS SoundFont.

## License

The source code is licensed under the [GNU General Public License v3.0][license]. Third-party components and bundled data retain their respective licenses.

[Arm GNU Toolchain]: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
[circle-stdlib]: https://github.com/smuehlst/circle-stdlib
[Circle]: https://github.com/rsta2/circle
[Configuration file]: https://github.com/dwhinham/mt32-pi/wiki/Configuration-file
[Control surface]: https://github.com/dwhinham/mt32-pi/wiki/Control-surface
[FluidSynth]: http://www.fluidsynth.org
[FTP server]: https://github.com/dwhinham/mt32-pi/wiki/Embedded-FTP-server
[General MIDI]: https://en.wikipedia.org/wiki/General_MIDI
[GeneralUser GS]: http://schristiancollins.com/generaluser.php
[GPIO MIDI interface]: https://github.com/dwhinham/mt32-pi/wiki/GPIO-MIDI-interface
[I²S Hi-Fi DACs]: https://github.com/dwhinham/mt32-pi/wiki/I%C2%B2S-DACs
[inih]: https://github.com/benhoyt/inih
[LCD and OLED displays]: https://github.com/dwhinham/mt32-pi/wiki/LCD-and-OLED-displays
[License]: https://github.com/dwhinham/mt32-pi/blob/master/LICENSE
[MiSTer FPGA]: https://github.com/dwhinham/mt32-pi/wiki/MiSTer-FPGA
[MiSTer setup]: https://github.com/dwhinham/mt32-pi/wiki/MiSTer-FPGA%3A-Setup-and-usage
[MT-32 synthesis]: https://github.com/dwhinham/mt32-pi/wiki/MT-32-synthesis
[mt32-pi wiki]: https://github.com/dwhinham/mt32-pi/wiki
[Munt]: https://github.com/munt/munt
[Releases]: https://github.com/dwhinham/mt32-pi/releases
[Roland GS]: https://en.wikipedia.org/wiki/Roland_GS
[RTP-MIDI]: https://github.com/dwhinham/mt32-pi/wiki/Networking%3A-RTP-MIDI-%28AppleMIDI%29
[Raw UDP socket]: https://github.com/dwhinham/mt32-pi/wiki/Networking%3A-UDP-MIDI
[SD card preparation]: https://github.com/dwhinham/mt32-pi/wiki/SD-card-preparation
[Serial port]: https://github.com/dwhinham/mt32-pi/wiki/MIDI-via-RS-232-or-USB-to-serial
[SoundFont synthesis]: https://github.com/dwhinham/mt32-pi/wiki/SoundFont-synthesis
[SoundFont]: https://en.wikipedia.org/wiki/SoundFont
[Updating mt32-pi]: https://github.com/dwhinham/mt32-pi/wiki/Updating-mt32-pi
[USB MIDI interfaces]: https://github.com/dwhinham/mt32-pi/wiki/USB-MIDI-interfaces
[Yamaha XG]: https://en.wikipedia.org/wiki/Yamaha_XG
