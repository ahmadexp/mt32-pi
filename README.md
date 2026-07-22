
---

[![mt32-pi CI](https://github.com/dwhinham/mt32-pi/workflows/mt32-pi%20CI/badge.svg)](https://github.com/dwhinham/mt32-pi/actions?query=workflow:"mt32-pi+CI")

<h1 align="center">
    <img width="90%" title="mt32-pi - Baremetal synthesizer system" src="images/mt32pi_logo.svg">
</h1>

- A work-in-progress baremetal MIDI synthesizer for the Raspberry Pi 3 or above, based on [Munt], [FluidSynth] and [Circle].
- Turn your Raspberry Pi into a dedicated emulation of the [famous multi-timbre sound module][Roland MT-32] used by countless classic MS-DOS, PC-98 and Sharp X68000 games!
- Add your favorite [SoundFonts][SoundFont] to expand your synthesizer with [General MIDI], [Roland GS], or even [Yamaha XG] support for endless MIDI possibilities.
- Includes General MIDI and Roland GS support out of the box thanks to [GeneralUser GS] by S. Christian Collins.
- No operating system, no complex Linux audio configuration; just super-low latency audio.
- Automatic idle power management reduces CPU speed and parks inactive cores, then wakes immediately for MIDI activity.
- Easy to configure and ready to play from cold-boot in a matter of seconds.
- The perfect companion for your vintage PC or [MiSTer FPGA] setup.

---

## ✔️ Project status

<img title="mt32-pi running on the Raspberry Pi 3 A+ with the Arananet PI-MIDI HAT." width="280rem" align="right" src="images/mt32pi_pimidi.png">

- Supports Raspberry Pi Zero 2 W, Raspberry Pi 3 Model A+, B, and B+, Raspberry Pi 4 Model B, and CM4 series.
  * Pi 2 works, but only with concessions on playback quality.
  * Pi Zero (original) and Pi 1 are unfortunately too slow (even with an overclock) and unsupported.
- PWM headphone jack audio.
  * Quality is known to be poor (aliasing/distortion on quieter sounds).
  * It is not currently known whether this can be improved or not.
- [I²S Hi-Fi DAC support][I²S Hi-Fi DACs].
  * This is the recommended audio output method for the best quality audio.
- MIDI input via [USB][USB MIDI interfaces], [GPIO][GPIO MIDI interface] MIDI interfaces, or the [serial port].
- [Configuration file] for selecting hardware options and fine tuning.
- [LCD status screen support][LCD and OLED displays] (for MT-32 SysEx messages and status information).
- Simple [physical control surface][control surface] using buttons and rotary encoder.
- [MiSTer FPGA integration via user port][MiSTer FPGA].
- Network MIDI support via [RTP-MIDI] and [raw UDP socket].
- [Embedded FTP server][FTP server] for remote access to files.
- A user interface with menu system is _planned_.
- More advanced MIDI routing is _planned_.

## ✨ Quick-start guide

🆕 If you have a Linux computer or MiSTer FPGA device, you may wish to try the new interactive [mt32-pi installer script](scripts).

Otherwise, for a manual installation:

1. Download the latest release from the [Releases] section.
    * If you are **updating an old version**, read the [Updating mt32-pi] wiki page for the correct procedure.
2. Extract contents to a blank [FAT32-formatted SD card][SD card preparation].
    * Read the [SD card preparation] wiki page for hints on formatting an SD card correctly (especially under Windows).
3. For MT-32 support, add your MT-32 or CM-32L ROM images to the `roms` directory - you have to provide these for copyright reasons.
    * You will need at least one control ROM and one PCM ROM.
    * For information on using multiple ROM sets and switching between them, see the [MT-32 synthesis] wiki page.
    * The file names or extensions don't matter; mt32-pi will scan and detect their types automatically.
4. Optionally add your favorite SoundFonts to the `soundfonts` directory.
    * For information on using multiple SoundFonts and switching between them, see the [SoundFont synthesis] wiki page.
    * Again, file names/extensions don't matter.
5. Edit the `mt32-pi.cfg` file to enable any optional hardware (Hi-Fi DAC, displays, buttons). Refer to [the wiki][mt32-pi wiki] to find supported hardware.
    * **MiSTer users**: Read the [MiSTer setup] section of the wiki for the recommended configuration, and ignore the following two steps.
6. Connect a [USB MIDI interface][USB MIDI interfaces] or [GPIO MIDI circuit][GPIO MIDI interface] to the Pi, and connect some speakers to the headphone jack.
7. Connect your vintage PC's MIDI OUT to the Pi's MIDI IN and (optionally) vice versa.

## ⚡ CPU power management

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

## 🛠️ Building from source

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

## 📚 Documentation

More detailed documentation for mt32-pi can now be found over at the [mt32-pi wiki]. Please read the wiki pages to learn about all of mt32-pi's features and supported hardware, and consider helping us improve it!

## ❓ Help

Take a look at our [FAQ] page for answers to the most common questions about mt32-pi.

If you need some help with mt32-pi and the wiki doesn't answer your questions, head over to the [discussions] area and feel free to start a topic.

> ⚠ **Note**: Please don't use the Issues area to ask for help - Issues are intended for reproducible bug reports and feature requests. Thank you!

## ❤️ Contributing

This project is generally quite stable and very usable, but still considered by its author to be in early stages of development.

Hence, please **DO NOT** work on large features and open pull requests without prior discussion. There is a strong possibility that work-in-progress code for proposed features already exists, but may not yet be public, and your work will have to be rejected.

Trivial changes to the code that fix issues are always welcome, as are improvements to documentation, and hardware/software compatibility reports.

## ⚖️ License

This project's source code is licensed under the [GNU General Public License v3.0][license].

The [mt32-pi logo] was designed by and is © Dale Whinham. The terms of use for the logo are as follows:

- The logo **MAY** be used on open-source community hardware.
- The logo **MAY** be used to link back to this repository or for similar promotional purposes of a strictly **non-commercial nature** (e.g. blog posts, social media, YouTube videos).
- The logo **MUST NOT** be used on or for the marketing of closed-source or commercial hardware (e.g. case designs, PCBs), without express permission.
- The logo **MUST NOT** be used for any other commercial products or purposes without express permission.
- The shape and overall design of the logo **MUST NOT** be modified or distorted. You **MAY** change the colors if required.
- If in any doubt, please ask. Thank you.

## 🙌 Acknowledgments

- Many thanks go out to @rc55 and @nswaldman for their encouragement and testing! ❤️
- A huge thank you to everyone who has donated via Ko-fi, PayPal, or Amazon - your support means a lot! ❤️
- Special thanks to [Edu Arana (Arananet)], [Porkchop Express (MiSTerAddons)], @djhardrich, [Nat (MiSTerFPGA.co.uk)], [Ricardo Saraiva (UltimateMiSTer.com)], [Serge Defever (Serdashop)], and @opjose who have all generously donated hardware to the project.
- The [Munt] team for their incredible work reverse-engineering the Roland MT-32 and producing an excellent emulation and well-structured project.
- The [FluidSynth] team for their excellent and easily-portable SoundFont synthesizer project.
- [S. Christian Collins][GeneralUser GS] for the excellent GeneralUser GS SoundFont and for kindly giving permission to include it in the project.
- The [Circle] and [circle-stdlib] projects for providing the best C++ baremetal framework for the Raspberry Pi.
- The [inih] project for a nice, lightweight config file parser.

[Changelog]: https://github.com/dwhinham/mt32-pi/blob/master/CHANGELOG.md
[Arm GNU Toolchain]: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
[circle-stdlib]: https://github.com/smuehlst/circle-stdlib
[Circle]: https://github.com/rsta2/circle
[Configuration file]: https://github.com/dwhinham/mt32-pi/wiki/Configuration-file
[Control surface]: https://github.com/dwhinham/mt32-pi/wiki/Control-surface
[Discussions]: https://github.com/dwhinham/mt32-pi/discussions
[Edu Arana (Arananet)]: https://www.arananet.net/pedidos
[FAQ]: https://github.com/dwhinham/mt32-pi/wiki/FAQ
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
[mt32-pi logo]: https://github.com/dwhinham/mt32-pi/blob/master/images/mt32pi_logo.svg
[mt32-pi wiki]: https://github.com/dwhinham/mt32-pi/wiki
[Munt]: https://github.com/munt/munt
[Nat (MiSTerFPGA.co.uk)]: https://misterfpga.co.uk
[Porkchop Express (MiSTerAddons)]: https://misteraddons.com
[Releases]: https://github.com/dwhinham/mt32-pi/releases
[Ricardo Saraiva (UltimateMiSTer.com)]: https://ultimatemister.com
[Roland GS]: https://en.wikipedia.org/wiki/Roland_GS
[Roland MT-32]: https://en.wikipedia.org/wiki/Roland_MT-32
[RTP-MIDI]: https://github.com/dwhinham/mt32-pi/wiki/Networking%3A-RTP-MIDI-%28AppleMIDI%29
[Raw UDP socket]: https://github.com/dwhinham/mt32-pi/wiki/Networking%3A-UDP-MIDI
[SD card preparation]: https://github.com/dwhinham/mt32-pi/wiki/SD-card-preparation
[Serge Defever (Serdashop)]: http://serdashop.com
[Serial port]: https://github.com/dwhinham/mt32-pi/wiki/MIDI-via-RS-232-or-USB-to-serial
[SoundFont synthesis]: https://github.com/dwhinham/mt32-pi/wiki/SoundFont-synthesis
[SoundFont]: https://en.wikipedia.org/wiki/SoundFont
[Updating mt32-pi]: https://github.com/dwhinham/mt32-pi/wiki/Updating-mt32-pi
[USB MIDI interfaces]: https://github.com/dwhinham/mt32-pi/wiki/USB-MIDI-interfaces
[Yamaha XG]: https://en.wikipedia.org/wiki/Yamaha_XG
