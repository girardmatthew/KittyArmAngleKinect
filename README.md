# Kitty Azure Speech Recognition and Body Tracking Using Microsoft Simple 3d Viewer Sample

## Introduction

The Kitty Speech Body Tracking with Microsoft Simple 3d Viewer creates a 3d window using Azure Body Tracking SDK that takes vocal commands using Azure speech recognition to determine which body parts to visualize and tracks all the information.

## Usage Info

USAGE: simple_3d_viewer.exe SensorMode[NFOV_UNBINNED, WFOV_BINNED](optional) RuntimeMode[CPU, OFFLINE](optional)
* SensorMode:
  * NFOV_UNBINNED (default) - Narraw Field of View Unbinned Mode [Resolution: 640x576; FOI: 75 degree x 65 degree]
  * WFOV_BINNED             - Wide Field of View Binned Mode [Resolution: 512x512; FOI: 120 degree x 120 degree]
* RuntimeMode:
  * CPU - Use the CPU only mode. It runs on machines without a GPU but it will be much slower
  * OFFLINE - Play a specified file. Does not require Kinect device. Can use with CPU mode

```
e.g.   simple_3d_viewer.exe WFOV_BINNED CPU
                 simple_3d_viewer.exe CPU
                 simple_3d_viewer.exe WFOV_BINNED
                 simple_3d_viewer.exe OFFLINE MyFile.mkv
```

## Instruction
Give vocal command "Hello Kitty!" to wake up from sleep

### Vocal Commands:
* track arm::  Kitty starts tracking arm (returns arm angle in command window)
* track hand:  Kitty starts tracking hand
* track face:  Kitty starts tracking face
* track body:  Kitty starts tracking body
* go back to sleep:  Kitty goes to sleep

### Basic Navigation:
* Rotate: Rotate the camera by moving the mouse while holding mouse left button
* Pan: Translate the scene by holding Ctrl key and drag the scene with mouse left button
* Zoom in/out: Move closer/farther away from the scene center by scrolling the mouse scroll wheel
* Select Center: Center the scene based on a detected joint by right clicking the joint with mouse

### Key Shortcuts
* ESC: quit
* h: help
* b: body visualization mode
* k: 3d window layout
