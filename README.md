# TegraExplorer

[![TegraExplorer builder](https://github.com/sthetix/TegraExplorer-Ext/workflows/TegraExplorer%20builder/badge.svg)](https://github.com/sthetix/TegraExplorer-Ext/actions)
[![Downloads](https://img.shields.io/github/downloads/sthetix/TegraExplorer-Ext/total)](https://github.com/sthetix/TegraExplorer-Ext/releases)
[![Version](https://img.shields.io/github/v/release/sthetix/TegraExplorer-Ext)](https://github.com/sthetix/TegraExplorer-Ext/releases)

<p align="center">
  <img src="https://raw.githubusercontent.com/sthetix/TegraExplorer-Ext/main/images/preview.jpg">
</p>

A payload-based file explorer for your switch!

## Usage
1. Get your favorite payload injector
2. Inject TegraExplorer as a payload

Navigate around the menus using the joycons.
- A: Select
- B: Back
- Left Joystick up/down (Dpad or joystick): navigate menus up/down
- Right Joystick up/down: fast menu navigation up/down
- Capture (Minerva only): Take a screenshot
- L3/R3 (Press joysticks in): Recalibrate centerpoint

If you do not have your joycons connected:
- Power -> A
- Vol+ -> Left Joystick up
- Vol- -> Left Joystick down

## Functions
- Navigate the SD card
- Navigate the System partition of your sysmmc and emummc
- Interact with files
	- Deleting, copying, renaming and moving files
	- Launching payloads files
	- Viewing the hex data of a file
	- Launching special [TegraScript](https://github.com/suchmememanyskill/TegraScript) files
	- Renaming files
- Interacting with folders
	- Deleting, copying or renaming folders
	- Creating folders
- Dumping your current firmware to sd
- Formatting the sd card
- **Launch Lockpick_RCM** directly from the menu

*and more*

## Menu Structure

```
-- Explore --
  Browse SD / Mount SD
  Browse EMMC / Browse EMUMMC

-- Tools --
  Partition the sd
  View dumped keys
  Credits

-- Load --
  Load Lockpick_RCM

-- Reboot --
  Reboot to RCM
  Reboot normally
  Reboot to Hekate

-- Scripts --
  [Dynamic script files from SD card]

-- Exit --
  Power off
```

## Support

For general CFW support, go to the [Nintendo Homebrew](https://discord.gg/C29hYvh) discord

## Changes in This Fork

This is a modified version of TegraExplorer with custom enhancements:

### UI/UX Improvements
- **Updated color scheme** to match Lockpick_RCM Pro theme
- **Reorganized main menu** structure for better navigation
- **Turquoise headers** with soft white text for clean, minimal aesthetic

### New Features
- **Load Lockpick_RCM** - Launch Lockpick_RCM payloads directly from the menu
  - Supports `Lockpick_RCM_Pro.bin` (HATS pack)
  - Falls back to `Lockpick_RCM.bin`

### Menu Reorganization
- **Explore**: SD/MMC/EMUMMC browsing
- **Tools**: Partition, keys, credits
- **Load**: Lockpick_RCM launcher
- **Reboot**: RCM, normal, Hekate
- **Scripts**: Dynamic script loading from SD
- **Exit**: Power off (red, at bottom)

### Script Modifications
- **SystemWipe.te**: Enhanced UI with flashing border animation, detailed warning screen, improved safety checks
- **FirmwareDump.te**: Streamlined workflow with cleaner user interaction flow

### Repository Independence
- Completely separated from upstream repository
- All embedded scripts regenerated from local source
- No automatic syncing with original project

## Credits

Original project by suchmememanyskill.

Based on [Lockpick_RCM](https://github.com/shchmue/Lockpick_RCM), and thus also based on [Hekate](https://github.com/CTCaer/hekate)

Awesome people who helped with this project:
- [shchmue](https://github.com/shchmue)
- [maddiethecafebabe](https://github.com/maddiethecafebabe/)
- [bleck9999](https://github.com/bleck9999)

Other awesome people:
- PhazonicRidley
- Dax
- Huhen
- Exelix
- Emmo
- Gengar
- Einso
- JeffV

## Screenshots

![Screenshot 1](https://raw.githubusercontent.com/sthetix/TegraExplorer-Ext/main/images/screenshot1.jpg)
![Screenshot 2](https://raw.githubusercontent.com/sthetix/TegraExplorer-Ext/main/images/screenshot2.jpg)
![Screenshot 3](https://raw.githubusercontent.com/sthetix/TegraExplorer-Ext/main/images/screenshot3.jpg)
