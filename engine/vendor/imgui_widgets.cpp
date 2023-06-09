#include "imgui_widgets.h"

void DrawBoundingBox(
    const float* _View,
    const float* _Projection,
    const float* _Matrix,
    const float* _Min,
    const float* _Max)
    {
       matrix_t viewProjection = *(matrix_t*)_View * *(matrix_t*)_Projection;
       vec_t frustum[6];
       ComputeFrustumPlanes(frustum, viewProjection.m16);
       matrix_t res = *(matrix_t*)_Matrix * viewProjection;

       float thickness = 2.f;
       ImU32 col = IM_COL32(0xE2, 0x52, 0x52, 0xFF);

       vec_t camSpacePosition;
       camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gContext.mMVP);
       if (!gContext.mIsOrthographic && camSpacePosition.z < 1.5f)
           return;

       {
           vec_t ptA = makeVect(_Min[0], _Min[1], _Min[2]);
           vec_t ptB = makeVect(_Max[0], _Min[1], _Min[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Min[1], _Min[2]);
           vec_t ptB = makeVect(_Min[0], _Max[1], _Min[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Min[1], _Min[2]);
           vec_t ptB = makeVect(_Min[0], _Min[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Max[1], _Min[2]);
           vec_t ptB = makeVect(_Max[0], _Max[1], _Min[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Max[0], _Max[1], _Min[2]);
           vec_t ptB = makeVect(_Max[0], _Min[1], _Min[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Max[0], _Min[1], _Min[2]);
           vec_t ptB = makeVect(_Max[0], _Min[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Max[0], _Max[1], _Min[2]);
           vec_t ptB = makeVect(_Max[0], _Max[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Max[1], _Min[2]);
           vec_t ptB = makeVect(_Min[0], _Max[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Max[1], _Max[2]);
           vec_t ptB = makeVect(_Max[0], _Max[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Max[1], _Max[2]);
           vec_t ptB = makeVect(_Min[0], _Min[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Min[0], _Min[1], _Max[2]);
           vec_t ptB = makeVect(_Max[0], _Min[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Max[0], _Max[1], _Max[2]);
           vec_t ptB = makeVect(_Max[0], _Min[1], _Max[2]);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }
   }

void DrawPerspectiveFrustum(
    const float* _View,
    const float* _Projection,
    const float* _Matrix,
    const float _Near,
    const float _Far,
    const float _Width,
    const float _Height,
    const float _Fov)
    {
       matrix_t viewProjection = *(matrix_t*)_View * *(matrix_t*)_Projection;
       vec_t frustum[6];
       ComputeFrustumPlanes(frustum, viewProjection.m16);
       matrix_t res = *(matrix_t*)_Matrix * viewProjection;

       float thickness = 2.f;
       ImU32 col = IM_COL32(0xE2, 0x52, 0x52, 0xFF);

       float Hnear = 2 * (float)tan((_Fov * DEG2RAD) / 2) * _Near;
       float Wnear = Hnear * _Width / _Height;
       float Hfar = 2 * (float)tan((_Fov * DEG2RAD) / 2) * _Far;
       float Wfar = Hfar * _Width / _Height;

       vec_t camSpacePosition;
       camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gContext.mMVP);
       if (!gContext.mIsOrthographic && camSpacePosition.z < 1.5f)
           return;

       {
           vec_t ptA = makeVect(-Wnear, Hnear, _Near);
           vec_t ptB = makeVect(Wnear, Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(Wnear, Hnear, _Near);
           vec_t ptB = makeVect(Wnear, -Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(Wnear, -Hnear, _Near);
           vec_t ptB = makeVect(-Wnear, -Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-Wnear, -Hnear, _Near);
           vec_t ptB = makeVect(-Wnear, Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-Wnear, Hnear, _Near);
           vec_t ptB = makeVect(-Wfar, Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(Wnear, Hnear, _Near);
           vec_t ptB = makeVect(Wfar, Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(Wnear, -Hnear, _Near);
           vec_t ptB = makeVect(Wfar, -Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-Wnear, -Hnear, _Near);
           vec_t ptB = makeVect(-Wfar, -Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-Wfar, Hfar, _Far);
           vec_t ptB = makeVect(Wfar, Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(Wfar, Hfar, _Far);
           vec_t ptB = makeVect(Wfar, -Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(Wfar, -Hfar, _Far);
           vec_t ptB = makeVect(-Wfar, -Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-Wfar, -Hfar, _Far);
           vec_t ptB = makeVect(-Wfar, Hfar, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
           vec_t ptB = makeVect(-Wnear, Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
           vec_t ptB = makeVect(Wnear, Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
           vec_t ptB = makeVect(Wnear, -Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
           vec_t ptB = makeVect(-Wnear, -Hnear, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }
   }

void DrawOrthographicFrustum(
    const float* _View,
    const float* _Projection,
    const float* _Matrix,
    const float _Near,
    const float _Far,
    const float _Width,
    const float _Height)
    {
       matrix_t viewProjection = *(matrix_t*)_View * *(matrix_t*)_Projection;
       vec_t frustum[6];
       ComputeFrustumPlanes(frustum, viewProjection.m16);
       matrix_t res = *(matrix_t*)_Matrix * viewProjection;

       float thickness = 2.f;
       ImU32 col = IM_COL32(0xE2, 0x52, 0x52, 0xFF);

       vec_t camSpacePosition;
       camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gContext.mMVP);
       if (!gContext.mIsOrthographic && camSpacePosition.z < 1.5f)
           return;

       if (_Near != 0.0f)
       {
           {
               vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
               vec_t ptB = makeVect(_Width, _Height, _Near);
               gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
           }

           {
               vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
               vec_t ptB = makeVect(_Width, -_Height, _Near);
               gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
           }

           {
               vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
               vec_t ptB = makeVect(-_Width, -_Height, _Near);
               gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
           }

           {
               vec_t ptA = makeVect(0.0f, 0.0f, 0.0f);
               vec_t ptB = makeVect(-_Width, _Height, _Near);
               gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
           }
       }

       {
           vec_t ptA = makeVect(-_Width, _Height, _Near);
           vec_t ptB = makeVect(_Width, _Height, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Width, _Height, _Near);
           vec_t ptB = makeVect(_Width, -_Height, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Width, -_Height, _Near);
           vec_t ptB = makeVect(-_Width, -_Height, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-_Width, -_Height, _Near);
           vec_t ptB = makeVect(-_Width, _Height, _Near);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-_Width, _Height, _Far);
           vec_t ptB = makeVect(_Width, _Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Width, _Height, _Far);
           vec_t ptB = makeVect(_Width, -_Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Width, -_Height, _Far);
           vec_t ptB = makeVect(-_Width, -_Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-_Width, -_Height, _Far);
           vec_t ptB = makeVect(-_Width, _Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-_Width, _Height, _Near);
           vec_t ptB = makeVect(-_Width, _Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Width, _Height, _Near);
           vec_t ptB = makeVect(_Width, _Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(_Width, -_Height, _Near);
           vec_t ptB = makeVect(_Width, -_Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }

       {
           vec_t ptA = makeVect(-_Width, -_Height, _Near);
           vec_t ptB = makeVect(-_Width, -_Height, _Far);
           gContext.mDrawList->AddLine(worldToPos(ptA, res), worldToPos(ptB, res), col, thickness);
       }
   }