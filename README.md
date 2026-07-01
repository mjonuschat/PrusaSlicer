
![PrusaSlicer logo](/resources/icons/PrusaSlicer_128px.png)

# PrusaSlicer

PrusaSlicer takes 3D models and converts them into G-code instructions for FDM 3D printers or PNG layers for mSLA 3D printers.
It is developed by [Prusa Research](https://www.prusa3d.com/), but it also supports printers of different manufacturers.

PrusaSlicer is based on [Slic3r](https://github.com/Slic3r/Slic3r) by Alessandro Ranellucci and the RepRap community.

### Distribution

Windows installer and a DMG for macOS can be downloaded from [PrusaSlicer project page](https://www.prusa3d.com/prusaslicer/). Alternatively, you can download them directly from [github releases page](https://github.com/prusa3d/PrusaSlicer/releases) (as well as no-install ZIP package for Windows).

Linux is currently distributed exclusively through [Flatpak](https://flatpak.org/). Visit [our Flathub](https://flathub.org/en/apps/com.prusa3d.PrusaSlicer) page for more information.

You are also more than welcome to build PrusaSlicer yourself from source. See the [documentation directory](doc/) in the source tree to learn how to do it.


### Main features

* **multi-platform** (Win/macOS/Linux)
* **command-line interface** to use it with no GUI
* supports **both FDM and SLA** printers
* per-object and feature-type based invalidation allows to reslice just part of a project when a parameter changes
* ability to set settings on per-object basis
* ability to open **multiple projects**, each containing **multiple beds** (each with potentially **different settings**)
* multiple layer heights in a single print
* advanced 3D preview
* customizable **G-code macros**
* integration with [PrusaConnect](https://connect.prusa3d.com/) and [Printables](https://www.printables.com)


### Technical stack

All of PrusaSlicer is written in C++20, with CMake as the build system.

The slicing backend heavily relies on [Clipper](https://www.angusj.com/clipper2) by Angus Johnson, which handles polygon boolean operations, offsets and similar. [Eigen](https://libeigen.gitlab.io/) library is used for basic types and linear algebra calculations.

[wxWidgets](https://wxwidgets.org) library is used to handle GUI window and events across platforms. Most of the UI is implemented using a custom OpenGL-based UI toolkit based on [Yoga layout engine](https://github.com/facebook/yoga) and [Dear Imgui](https://github.com/ocornut/imgui), which makes it platform-independent.

The application uses many other libraries, you can see [deps/](deps/) and [bundled_deps/](bundled_deps/) folders in the source tree to see the complete list. We are grateful to the authors and maintaners to open-source their work.

### How to get in touch

#### Questions and feature requests

We maintain a [GitHub Discussions](https://www.github.com/prusa3d/discussions) pages in this repository to be used for general discussions, questions and feature requests. You can also find announcements from our side there.

#### Bug reports

Issues can be reported in our [GitHub issue tracker](https://github.com/prusa3d/PrusaSlicer/issues). Make sure that your issue is not already reported and that it indeed is a bug - questions and feature request belong to Discussions. This is to keep the issue tracker in order.

#### Pull requests

Read our [contribution guide](.github/CONTRIBUTING.md) to get more info.
