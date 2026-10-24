#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <cmath>

namespace all {

// The vertical field of view, in degrees, of a screen of height screenHeight seen from viewerDistance
// (both in the same unit). Given tan = opposite / adjacent <==> screen height / distance
// we can compute the vfov as 2 * atan(screen height / distance)
inline float fovFromPhysical(float screenHeight, float viewerDistance)
{
    return glm::degrees(2.0f * std::atan(screenHeight / viewerDistance));
}

struct StereoFrustumParameters {
    float fovY; // vertical field of view, degrees
    float aspectRatio; // width / height of one eye's picture
    float nearPlane;
    float farPlane;
    float interocularDistance; // e: the full distance between the eyes; negative swaps them
    float convergenceDistance; // d: the distance at which a point has no disparity
};

// One eye's camera: its offset along the camera's right vector and its near-plane window.
struct EyeFrustum {
    float eyeOffset;
    float left;
    float right;
    float bottom;
    float top;
    float nearPlane;
    float farPlane;
};

struct StereoFrustum {
    EyeFrustum left;
    EyeFrustum right;
};

// Off-axis stereo: the two cameras keep parallel axes, each displaced by e/2, and each one's near-plane
// window is shifted toward the other eye by (e/2) n/d, so that a point at distance d lands on the same
// window x in both eyes. The eye on the left (offset -e/2) uses l + shift and r + shift.
inline StereoFrustum stereoFrustum(const StereoFrustumParameters& p)
{
    const float halfHeight = std::tan(glm::radians(p.fovY) * 0.5f) * p.nearPlane;
    const float halfWidth = p.aspectRatio * halfHeight;
    const float halfInterocular = p.interocularDistance * 0.5f;
    const float shift = halfInterocular * p.nearPlane / p.convergenceDistance;

    return {
        { -halfInterocular, -halfWidth + shift, halfWidth + shift, -halfHeight, halfHeight, p.nearPlane, p.farPlane },
        { halfInterocular, -halfWidth - shift, halfWidth - shift, -halfHeight, halfHeight, p.nearPlane, p.farPlane },
    };
}

// The projection matrix of an eye's window, in glFrustum's form.
inline glm::mat4 frustumMatrix(const EyeFrustum& f)
{
    return glm::frustum(f.left, f.right, f.bottom, f.top, f.nearPlane, f.farPlane);
}

} // namespace all
