#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <shared/stereo_frustum.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/epsilon.hpp>

#include <cmath>

namespace {

// Projects a point given in the centre camera's eye space (the camera looks along -z) with one eye's
// frustum: the eye sits at eyeOffset along +x, so the point moves the other way before the projection.
glm::vec3 projectWithEye(const all::EyeFrustum& eye, const glm::vec3& centreEyeSpacePoint)
{
    const glm::vec4 clip = all::frustumMatrix(eye) * glm::vec4(centreEyeSpacePoint - glm::vec3(eye.eyeOffset, 0.0f, 0.0f), 1.0f);
    return glm::vec3(clip) / clip.w;
}

const all::StereoFrustumParameters params{
    .fovY = 40.0f,
    .aspectRatio = 16.0f / 9.0f,
    .nearPlane = 0.1f,
    .farPlane = 100.0f,
    .interocularDistance = 0.2f,
    .convergenceDistance = 6.0f,
};

} // namespace

TEST_CASE("field of view from the physical screen: 2 atan(H / 2D)")
{
    // A 0.6 m high screen seen from 0.8 m subtends 41.1 degrees, not 73.7 (2 atan(H / D)).
    CHECK(all::fovFromPhysical(0.6f, 0.8f) == doctest::Approx(41.112f).epsilon(0.001));
    CHECK(all::fovFromPhysical(1.0f, 1.0f) == doctest::Approx(53.130f).epsilon(0.001));
    // At a distance of half the height the screen fills a right angle.
    CHECK(all::fovFromPhysical(1.0f, 0.5f) == doctest::Approx(90.0f).epsilon(0.001));
}

TEST_CASE("asymmetric frusta: the eyes sit e/2 apart and the windows shift by (e/2) n/d")
{
    const all::StereoFrustum f = all::stereoFrustum(params);
    const float halfHeight = std::tan(glm::radians(params.fovY) * 0.5f) * params.nearPlane;
    const float halfWidth = halfHeight * params.aspectRatio;
    const float shift = 0.5f * params.interocularDistance * params.nearPlane / params.convergenceDistance;

    CHECK(f.left.eyeOffset == doctest::Approx(-0.1f));
    CHECK(f.right.eyeOffset == doctest::Approx(0.1f));
    CHECK(f.left.left == doctest::Approx(-halfWidth + shift));
    CHECK(f.left.right == doctest::Approx(halfWidth + shift));
    CHECK(f.right.left == doctest::Approx(-halfWidth - shift));
    CHECK(f.right.right == doctest::Approx(halfWidth - shift));
    for (const all::EyeFrustum* eye : { &f.left, &f.right }) {
        CHECK(eye->bottom == doctest::Approx(-halfHeight));
        CHECK(eye->top == doctest::Approx(halfHeight));
        CHECK(eye->nearPlane == doctest::Approx(params.nearPlane));
        CHECK(eye->farPlane == doctest::Approx(params.farPlane));
    }
}

TEST_CASE("the projection matrix is glFrustum's")
{
    const all::StereoFrustum f = all::stereoFrustum(params);
    for (const all::EyeFrustum* eye : { &f.left, &f.right }) {
        const glm::mat4 ours = all::frustumMatrix(*eye);
        const glm::mat4 reference = glm::frustum(eye->left, eye->right, eye->bottom, eye->top, eye->nearPlane, eye->farPlane);
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                CHECK(ours[c][r] == doctest::Approx(reference[c][r]).epsilon(1e-5));
    }

    // With the eyes together the frustum is the symmetric perspective one.
    all::StereoFrustumParameters mono = params;
    mono.interocularDistance = 0.0f;
    const glm::mat4 symmetric = all::frustumMatrix(all::stereoFrustum(mono).left);
    const glm::mat4 perspective = glm::perspective(glm::radians(params.fovY), params.aspectRatio, params.nearPlane, params.farPlane);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            CHECK(symmetric[c][r] == doctest::Approx(perspective[c][r]).epsilon(1e-5));
}

TEST_CASE("a point on the convergence plane has no disparity, nearer points are crossed, vertical disparity is zero")
{
    const all::StereoFrustum f = all::stereoFrustum(params);
    const float d = params.convergenceDistance;
    const float tanHalfH = std::tan(glm::radians(params.fovY) * 0.5f) * params.aspectRatio;

    for (float x : { -2.0f, -0.5f, 0.0f, 0.7f, 1.9f }) {
        for (float y : { -1.0f, 0.0f, 0.8f }) {
            // On the convergence plane: the same normalised x in both eyes.
            const glm::vec3 onPlane = projectWithEye(f.left, { x, y, -d });
            const glm::vec3 onPlaneRight = projectWithEye(f.right, { x, y, -d });
            CHECK(onPlane.x == doctest::Approx(onPlaneRight.x).epsilon(1e-5));
            CHECK(onPlane.y == doctest::Approx(onPlaneRight.y).epsilon(1e-5));

            // At d/2 and at 2d: the closed form e (1/z - 1/d) / tan(hfov/2) in normalised x, vertical identical.
            for (float z : { d / 2.0f, 2.0f * d }) {
                const glm::vec3 left = projectWithEye(f.left, { x, y, -z });
                const glm::vec3 right = projectWithEye(f.right, { x, y, -z });
                const float predicted = params.interocularDistance * (1.0f / z - 1.0f / d) / tanHalfH;
                CHECK(left.x - right.x == doctest::Approx(predicted).epsilon(1e-4));
                CHECK(left.y == doctest::Approx(right.y).epsilon(1e-5));
            }
            // Nearer than the convergence plane the left eye sees the point further right: crossed disparity.
            CHECK(projectWithEye(f.left, { x, y, -d / 2.0f }).x > projectWithEye(f.right, { x, y, -d / 2.0f }).x);
            CHECK(projectWithEye(f.left, { x, y, -2.0f * d }).x < projectWithEye(f.right, { x, y, -2.0f * d }).x);
        }
    }
}

TEST_CASE("flipped eyes swap the two frusta")
{
    all::StereoFrustumParameters flipped = params;
    flipped.interocularDistance = -params.interocularDistance;
    const all::StereoFrustum f = all::stereoFrustum(params);
    const all::StereoFrustum g = all::stereoFrustum(flipped);
    CHECK(g.left.eyeOffset == doctest::Approx(f.right.eyeOffset));
    CHECK(g.left.left == doctest::Approx(f.right.left));
    CHECK(g.left.right == doctest::Approx(f.right.right));
    CHECK(g.right.eyeOffset == doctest::Approx(f.left.eyeOffset));
}
