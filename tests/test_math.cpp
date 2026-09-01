#include "ZCTestFramework.h"

#include "math/ZCMath.h"

using namespace zocos;

ZC_TEST(math_identity_is_neutral) {
    const Mat4 id = Mat4::identity();
    const Mat4 t = Mat4::translate(3.f, 4.f);
    const Mat4 r = id * t;
    ZC_CHECK_NEAR(r.m[12], 3.f, 1e-5);
    ZC_CHECK_NEAR(r.m[13], 4.f, 1e-5);
    ZC_CHECK_NEAR(r.m[0], 1.f, 1e-5);
}

ZC_TEST(math_translate_then_rotate_order) {
    // translate(10,0) * rotateZ(90): rotate happens first, so (1,0) -> (0,1) -> (10,1).
    const Mat4 m = Mat4::translate(10.f, 0.f) * Mat4::rotateZ(90.f);
    const Vec2 p = m.transformPoint({1.f, 0.f});
    ZC_CHECK_NEAR(p.x, 10.f, 1e-4);
    ZC_CHECK_NEAR(p.y, 1.f, 1e-4);
}

ZC_TEST(math_ortho_maps_corners_to_ndc) {
    const Mat4 m = Mat4::ortho(0.f, 800.f, 0.f, 600.f, -1.f, 1.f);
    const Vec2 bottomLeft = m.transformPoint({0.f, 0.f});
    const Vec2 topRight = m.transformPoint({800.f, 600.f});
    ZC_CHECK_NEAR(bottomLeft.x, -1.f, 1e-5);
    ZC_CHECK_NEAR(bottomLeft.y, -1.f, 1e-5);
    ZC_CHECK_NEAR(topRight.x, 1.f, 1e-5);
    ZC_CHECK_NEAR(topRight.y, 1.f, 1e-5);
}

ZC_TEST(math_node_local_matrix_anchor) {
    // Anchor at the center of a 100x100 node placed at (50,50): the node's own
    // center maps to its position, and the local origin maps to position-anchor.
    const Mat4 m = zcNodeLocalMatrix({50.f, 50.f}, {1.f, 1.f}, 0.f, {0.5f, 0.5f}, {100.f, 100.f});
    const Vec2 origin = m.transformPoint({0.f, 0.f});
    const Vec2 center = m.transformPoint({50.f, 50.f});
    ZC_CHECK_NEAR(origin.x, 0.f, 1e-4);
    ZC_CHECK_NEAR(origin.y, 0.f, 1e-4);
    ZC_CHECK_NEAR(center.x, 50.f, 1e-4);
    ZC_CHECK_NEAR(center.y, 50.f, 1e-4);
}

ZC_TEST(math_node_local_matrix_scale) {
    // Scale 2x around a zero anchor: local (10,10) -> (20,20), then +position.
    const Mat4 m = zcNodeLocalMatrix({5.f, 0.f}, {2.f, 2.f}, 0.f, {0.f, 0.f}, {10.f, 10.f});
    const Vec2 p = m.transformPoint({10.f, 10.f});
    ZC_CHECK_NEAR(p.x, 25.f, 1e-4);
    ZC_CHECK_NEAR(p.y, 20.f, 1e-4);
}

ZC_TEST(math_affine_inverse_2d_roundtrip) {
    const Mat4 transform =
        Mat4::translate(12.f, -4.f) * Mat4::rotateZ(35.f) * Mat4::scale(2.f, 3.f);
    const Vec2 point{7.f, -5.f};
    const Vec2 transformed = transform.transformPoint(point);

    Mat4 inverse;
    ZC_CHECK(transform.getAffineInverse2D(inverse));
    const Vec2 restored = inverse.transformPoint(transformed);
    ZC_CHECK_NEAR(restored.x, point.x, 1e-4);
    ZC_CHECK_NEAR(restored.y, point.y, 1e-4);

    const Mat4 singular = Mat4::scale(0.f, 1.f);
    ZC_CHECK(!singular.getAffineInverse2D(inverse));
}
