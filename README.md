# Stereo Demonstrator

This is the stereo version of Stereo Demonstrator by Sparky Stereo OS, forked from [KDABLabs/stereo3ddemo](https://github.com/KDABLabs/stereo3ddemo).

The Stereo Demonstrator provides demo applications to experiment with
stereo rendering on stereo capable displays from [Schneider Digital](https://www.schneider-digital.com/en/)

![alt text](https://github.com/KDABLabs/stereo3ddemo/blob/master/screenshot.png)

This project builds a Qt3D based application and (optionally) a Vulkan based application.

Both applications allow loading 3D files *(.obj, .fbx, .gltf ...)* to view them in stereo.
A 3D cursor is drawn at the intersection between the mouse and the 3D model (fallsback to being place on focus plane if no intersection)

***Note:** that the Vulkan based application relies on KDAB's Vulkan engine which is not open sourced at this time.*

## Features

### Stereo Camera Controls:
- Projection Type (Asymmetric Frustums, Toe In)
- Focus Distance
- Eye Separation
- Flip Eyes

### Focus Controls
- AutoFocus + Focus Area
- Manual Focus Distance

### Field of View Controls:
- Manually Specified
- Deduced from Physical Dimensions

### Cursor Controls:
- Appearance
- Color
- Size

### Misc Controls:
- Wireframe mode
- Frustum Viewer

## Requirements
- OpenGL support (compatible GPU and drivers installed)
- GPU that supports quad buffers / multilayered swapchains (Nvidia Quadro & RTX, AMD Pro & FirePro)
  - **Note:** content will be displayed side by side on GPUs that do not support that feature
- Qt 6.8 with the Qt3D optional module

## Building

### Using VSCode (recommended)
- Copy CMakePresets.json.example to CMakePresets.json
- Edit CMAKE_PREFIX_PATH variable in CMakePresets.json to point to the Qt install path
- Build with VSCode
- Launch KDAB_Qt_Qt3D_OpenGL

### Using Command Line
- mkdir build; cd build
- cmake -DCMAKE_PREFIX_PATH=/path/to/Qt -DCMAKE_BUILD_TYPE=Release ..
- cmake --build .
- cmake --build . -t package
- ./KDAB_Qt_Qt3D_OpenGL


## Contact

- Visit us on GitHub: <https://github.com/KDAB/KDGpu>
- Email info@kdab.com for questions about copyright, licensing or commercial support.

Stay up-to-date with KDAB product announcements:

- [KDAB Newsletter](https://news.kdab.com)
- [KDAB Blogs](https://www.kdab.com/category/blogs)
- [KDAB on Twitter](https://twitter.com/KDABQt)

## Licensing

The Stereo Demonstrator is available under the terms of
the [GPL](https://github.com/KDABLabs/stereo3ddemo/blob/master/LICENSES/gpl-3.0.txt) license.

Contact KDAB at <info@kdab.com> if you need different licensing options.

## Get Involved

Please submit your contributions or issue reports from our GitHub space at <https://github.com/KDAB/KDGpu>.

Contact <info@kdab.com> for more information.

## About KDAB

The Stereo Demonstrator is supported and maintained by Klarälvdalens Datakonsult AB (KDAB).

The KDAB Group is the global No.1 software consultancy for Qt, C++ and
OpenGL applications across desktop, embedded and mobile platforms.

The KDAB Group provides consulting and mentoring for developing Qt applications
from scratch and in porting from all popular and legacy frameworks to Qt.
We continue to help develop parts of Qt and are one of the major contributors
to the Qt Project. We can give advanced or standard trainings anywhere
around the globe on Qt as well as C++, OpenGL, 3D and more.

Please visit <https://www.kdab.com> to meet the people who write code like this.

## Sparky Stereo OS

This fork is the Stereo Demonstrator as [Sparky Stereo OS](https://github.com/Sparky-OS/sparky-stereo-os) ships it, on its `stereo3d` branch.
It keeps everything the demonstrator does and changes how the picture reaches the screen, because on that desktop the compositor, Stereo KWin, does the display work:

- The 3D area is the one stereo surface. The stereo surface format is set on the Qt3D window only, never as the application's default format, so the side menu and the dialogs stay ordinary 2D surfaces, identical in both eyes.
- Full side by side, declared once. With a quad-buffer OpenGL context the edition's Mesa packs the two eyes and declares the surface at the swap, as before. Without one, the demonstrator no longer squeezes each eye into a quarter of the window: as a user option it draws the two eyes side by side, each at the 3D area's resolution, into a window twice as wide and declares it through [libstereo-declare](https://github.com/Sparky-OS/libstereo-declare); Stereo KWin shows it at one view's size and makes every screen's format from it (3D televisions, anaglyph on any monitor, interleaved and frame-sequential displays, two-output pairs, headsets). Otherwise it runs mono.
- The program's 3D options are those of every program on the edition: Enable 3D, the render resolution per eye and a shortcut to Display Settings. The demonstrator's own teaching controls stay, under Advanced.
- Two fixes found while reading the code: the field of view from the physical screen is `2·atan(H/(2D))`, and the off-axis frustum is a pure function with a unit test.
- Builds on Debian's Qt 6.11 and Qt 3D with the system's Assimp, glm, libspnav and doctest; KDBindings 1.1.0 is vendored under `3rdparty/KDBindings` (MIT). The closed-engine parts (Serenity, KDGui, Flutter) are off by default on Linux.
- Strings added by the edition come in English, Polish, Brazilian Portuguese, French, German, Japanese, Korean and Chinese (Simplified and Traditional).

The edition's documentation explains the design: [how it works](https://github.com/Sparky-OS/sparky-stereo-os/blob/main/docs/how-it-works.md).

The Stereo Demonstrator was written by KDAB for the stereo displays of Schneider Digital, and their logos in the About page credit them.
The licence is unchanged, GPL-3.0.

Sparky Stereo OS, Daniel Ramos's edition of SparkyLinux (by Paweł "pavroo" Pijanowski).
