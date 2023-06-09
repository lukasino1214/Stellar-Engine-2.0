#include <imgui.h>

IMGUI_API void DrawBoundingBox(
    const float* _View,
    const float* _Projection,
    const float* _Matrix,
    const float* _Min,
    const float* _Max);

IMGUI_API void DrawPerspectiveFrustum(
    const float* _View,
    const float* _Projection,
    const float* _Matrix,
    const float _Near,
    const float _Far,
    const float _Width,
    const float _Height,
    const float _Fov);

IMGUI_API void DrawOrthographicFrustum(
    const float* _View,
    const float* _Projection,
    const float* _Matrix,
    const float _Near,
    const float _Far,
    const float _Width,
    const float _Height);