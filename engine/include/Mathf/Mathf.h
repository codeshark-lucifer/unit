#pragma once

// Mathf.h - umbrella header for the small float math library.
//
//   Mathf/Mathf.h   scalar helpers (this file) + includes everything below
//   Mathf/Vec.h     Vec2, Vec3, Vec4
//   Mathf/Matrix.h  Mat2, Mat3, Mat4 (column-major, Vulkan clip space projections)
//   Mathf/Quat.h    Quat
//
// Include this one, or include just the piece you need.

#include <cmath>

namespace mathf
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TAU = 2.0f * PI;
    constexpr float EPSILON = 1e-6f;

    inline float radians(float degrees) { return degrees * (PI / 180.0f); }
    inline float degrees(float radians) { return radians * (180.0f / PI); }
    inline float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    inline float saturate(float v) { return clamp(v, 0.0f, 1.0f); }
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    inline float inverseLerp(float a, float b, float v) { return a == b ? 0.0f : (v - a) / (b - a); }
    inline float smoothstep(float a, float b, float v)
    {
        float t = saturate(inverseLerp(a, b, v));
        return t * t * (3.0f - 2.0f * t);
    }
    inline bool approx(float a, float b, float eps = EPSILON) { return std::fabs(a - b) <= eps; }
}

#include "Mathf/Vec.h"
#include "Mathf/Matrix.h"
#include "Mathf/Quat.h"