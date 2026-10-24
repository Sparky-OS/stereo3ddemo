#pragma once

namespace all {

// How the 3D window's picture reaches the screen.
enum class StereoPath {
    Off, // 3D disabled by the user: one picture from the centre camera, nothing declared
    Unavailable, // 3D wanted, no route on this stack: one picture from the left eye, nothing declared
    QuadBuffer, // a stereo OpenGL context: each eye in its back buffer, the display stack packs and declares them
    SideBySide // both eyes side by side in a window twice as wide, declared full side by side to the desktop
};

inline int stereoViews(StereoPath path)
{
    return path == StereoPath::SideBySide ? 2 : 1;
}

inline bool stereoDraws(StereoPath path)
{
    return path == StereoPath::QuadBuffer || path == StereoPath::SideBySide;
}

} // namespace all
