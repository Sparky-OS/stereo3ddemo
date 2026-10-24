// Where each eye lands in the 3D window's buffer, how big that buffer is for a render resolution per eye, and how a
// pointer position over the view shown at its display size maps to the render resolution.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <shared/stereo_layout.h>

using namespace all;

TEST_CASE("the buffer holds the views side by side only on the declared pair path")
{
    CHECK(bufferSize(StereoPath::SideBySide, 1280, 720) == ViewSize{ 2560, 720 });
    CHECK(bufferSize(StereoPath::QuadBuffer, 1280, 720) == ViewSize{ 1280, 720 });
    CHECK(bufferSize(StereoPath::Off, 1280, 720) == ViewSize{ 1280, 720 });
    CHECK(bufferSize(StereoPath::Unavailable, 1280, 720) == ViewSize{ 1280, 720 });
}

TEST_CASE("each eye's rectangle: the right eye is the right half of a pair, the same rectangle otherwise")
{
    CHECK(eyeRect(StereoPath::SideBySide, Eye::Left, 1280, 720) == ViewRect{ 0, 0, 1280, 720 });
    CHECK(eyeRect(StereoPath::SideBySide, Eye::Right, 1280, 720) == ViewRect{ 1280, 0, 1280, 720 });
    CHECK(eyeRect(StereoPath::QuadBuffer, Eye::Right, 1280, 720) == ViewRect{ 0, 0, 1280, 720 });
    CHECK(eyeRect(StereoPath::Off, Eye::Left, 1544, 1080) == ViewRect{ 0, 0, 1544, 1080 });
}

TEST_CASE("a pointer over the view shown at its display size maps to the render resolution")
{
    // The view is shown at 1544x1080 and rendered at 1280x720: the pointer's share of the view is kept
    const auto p = displayToRender(772.0f, 540.0f, ViewSize{ 1544, 1080 }, ViewSize{ 1280, 720 });
    CHECK(p.x == doctest::Approx(640.0f));
    CHECK(p.y == doctest::Approx(360.0f));
    // Same sizes: the identity
    const auto q = displayToRender(100.0f, 50.0f, ViewSize{ 1544, 1080 }, ViewSize{ 1544, 1080 });
    CHECK(q.x == doctest::Approx(100.0f));
    CHECK(q.y == doctest::Approx(50.0f));
    // A render resolution that is not set follows the display size
    const auto r = displayToRender(100.0f, 50.0f, ViewSize{ 1544, 1080 }, ViewSize{ 0, 0 });
    CHECK(r.x == doctest::Approx(100.0f));
    CHECK(r.y == doctest::Approx(50.0f));
}

TEST_CASE("the render resolution per eye: the display size when none is set")
{
    CHECK(renderSize(ViewSize{ 0, 0 }, ViewSize{ 1544, 1080 }) == ViewSize{ 1544, 1080 });
    CHECK(renderSize(ViewSize{ 1280, 720 }, ViewSize{ 1544, 1080 }) == ViewSize{ 1280, 720 });
}
