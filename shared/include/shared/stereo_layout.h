#pragma once
#include <shared/stereo_path.h>

namespace all {

enum class Eye {
    Left = 0,
    Right = 1
};

struct ViewSize {
    int width = 0;
    int height = 0;
    bool operator==(const ViewSize&) const = default;
    bool isValid() const { return width > 0 && height > 0; }
};

struct ViewRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool operator==(const ViewRect&) const = default;
};

struct ViewPoint {
    float x = 0.0f;
    float y = 0.0f;
};

// The render resolution per eye: the size chosen by the user, or the size the view is shown at when none is
inline ViewSize renderSize(ViewSize chosen, ViewSize display)
{
    return chosen.isValid() ? chosen : display;
}

// The 3D window's buffer for a path: the views side by side when the program draws the pair itself, one view
// otherwise (the display stack packs a quad-buffer window's two eyes itself)
inline ViewSize bufferSize(StereoPath path, int viewWidth, int viewHeight)
{
    return ViewSize{ viewWidth * stereoViews(path), viewHeight };
}

// Where an eye lands in that buffer
inline ViewRect eyeRect(StereoPath path, Eye eye, int viewWidth, int viewHeight)
{
    const int x = (path == StereoPath::SideBySide && eye == Eye::Right) ? viewWidth : 0;
    return ViewRect{ x, 0, viewWidth, viewHeight };
}

// A pointer position over the view, shown at its display size, in the render resolution's pixels: the pointer
// keeps its share of the view (an unset render resolution follows the display size)
inline ViewPoint displayToRender(float x, float y, ViewSize display, ViewSize render)
{
    const ViewSize r = renderSize(render, display);
    if (!display.isValid())
        return ViewPoint{ x, y };
    return ViewPoint{ x * float(r.width) / float(display.width), y * float(r.height) / float(display.height) };
}

} // namespace all
