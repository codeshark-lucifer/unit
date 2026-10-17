#pragma once

// Matrix.h - Mat2 / Mat3 / Mat4.
//
// Conventions
//  - Matrices are COLUMN-MAJOR: m[col][row]. This is the same memory layout as GLSL,
//    so a Mat4 can be copied straight into a uniform buffer or push constant.
//  - Right-handed coordinates, angles in RADIANS.
//  - Transform order is the usual column-vector form:  p' = Projection * View * Model * p
//  - Mat4::perspective() and Mat4::ortho() target VULKAN clip space (depth 0..1),
//    perspective() already flips Y, so no extra Y flip is needed.

#include <cmath>
#include "Mathf/Vec.h"

// ============================================================================
// Mat2  (column-major: c[col][row])
// ============================================================================
struct Mat2
{
    Vec2 c[2];

    Mat2() : Mat2(1.0f) {}
    explicit Mat2(float diagonal)
    {
        c[0] = Vec2(diagonal, 0.0f);
        c[1] = Vec2(0.0f, diagonal);
    }
    Mat2(const Vec2 &col0, const Vec2 &col1)
    {
        c[0] = col0;
        c[1] = col1;
    }

    static Mat2 identity() { return Mat2(1.0f); }

    Vec2 &operator[](int col) { return c[col]; }
    const Vec2 &operator[](int col) const { return c[col]; }
    const float *data() const { return &c[0].x; }

    Mat2 transpose() const
    {
        return Mat2(Vec2(c[0].x, c[1].x),
                    Vec2(c[0].y, c[1].y));
    }

    float determinant() const { return c[0].x * c[1].y - c[1].x * c[0].y; }

    // Returns identity if the matrix is singular.
    Mat2 inverse() const
    {
        float det = determinant();
        if (det == 0.0f)
            return Mat2();
        float inv = 1.0f / det;
        return Mat2(Vec2(c[1].y, -c[0].y) * inv,
                    Vec2(-c[1].x, c[0].x) * inv);
    }
};

inline Vec2 operator*(const Mat2 &m, const Vec2 &v) { return m.c[0] * v.x + m.c[1] * v.y; }
inline Mat2 operator*(const Mat2 &a, const Mat2 &b) { return Mat2(a * b.c[0], a * b.c[1]); }
inline Mat2 operator*(const Mat2 &m, float s) { return Mat2(m.c[0] * s, m.c[1] * s); }
inline Mat2 operator*(float s, const Mat2 &m) { return m * s; }
inline Mat2 operator+(const Mat2 &a, const Mat2 &b) { return Mat2(a.c[0] + b.c[0], a.c[1] + b.c[1]); }
inline Mat2 operator-(const Mat2 &a, const Mat2 &b) { return Mat2(a.c[0] - b.c[0], a.c[1] - b.c[1]); }
inline bool operator==(const Mat2 &a, const Mat2 &b) { return a.c[0] == b.c[0] && a.c[1] == b.c[1]; }
inline bool operator!=(const Mat2 &a, const Mat2 &b) { return !(a == b); }

// ============================================================================
// Mat3  (column-major: c[col][row])
// ============================================================================
struct Mat3
{
    Vec3 c[3];

    Mat3() : Mat3(1.0f) {}
    explicit Mat3(float diagonal)
    {
        c[0] = Vec3(diagonal, 0.0f, 0.0f);
        c[1] = Vec3(0.0f, diagonal, 0.0f);
        c[2] = Vec3(0.0f, 0.0f, diagonal);
    }
    Mat3(const Vec3 &col0, const Vec3 &col1, const Vec3 &col2)
    {
        c[0] = col0;
        c[1] = col1;
        c[2] = col2;
    }

    static Mat3 identity() { return Mat3(1.0f); }

    // 2D transforms in homogeneous coordinates (apply to Vec3(x, y, 1)).
    static Mat3 translate(const Vec2 &t)
    {
        Mat3 m;
        m.c[2] = Vec3(t.x, t.y, 1.0f);
        return m;
    }
    static Mat3 scale(const Vec2 &s)
    {
        Mat3 m;
        m.c[0].x = s.x;
        m.c[1].y = s.y;
        return m;
    }
    static Mat3 rotate(float angle)
    {
        float cs = std::cos(angle), sn = std::sin(angle);
        return Mat3(Vec3(cs, sn, 0.0f),
                    Vec3(-sn, cs, 0.0f),
                    Vec3(0.0f, 0.0f, 1.0f));
    }

    Vec3 &operator[](int col) { return c[col]; }
    const Vec3 &operator[](int col) const { return c[col]; }
    const float *data() const { return &c[0].x; }

    Mat3 transpose() const
    {
        return Mat3(Vec3(c[0].x, c[1].x, c[2].x),
                    Vec3(c[0].y, c[1].y, c[2].y),
                    Vec3(c[0].z, c[1].z, c[2].z));
    }

    float determinant() const { return dot(c[0], cross(c[1], c[2])); }

    // Returns identity if the matrix is singular.
    Mat3 inverse() const
    {
        Vec3 r0 = cross(c[1], c[2]);
        Vec3 r1 = cross(c[2], c[0]);
        Vec3 r2 = cross(c[0], c[1]);
        float det = dot(c[0], r0);
        if (det == 0.0f)
            return Mat3();
        float inv = 1.0f / det;
        return Mat3(Vec3(r0.x, r1.x, r2.x) * inv,
                    Vec3(r0.y, r1.y, r2.y) * inv,
                    Vec3(r0.z, r1.z, r2.z) * inv);
    }
};

inline Vec3 operator*(const Mat3 &m, const Vec3 &v) { return m.c[0] * v.x + m.c[1] * v.y + m.c[2] * v.z; }
inline Mat3 operator*(const Mat3 &a, const Mat3 &b) { return Mat3(a * b.c[0], a * b.c[1], a * b.c[2]); }
inline Mat3 operator*(const Mat3 &m, float s) { return Mat3(m.c[0] * s, m.c[1] * s, m.c[2] * s); }
inline Mat3 operator*(float s, const Mat3 &m) { return m * s; }
inline Mat3 operator+(const Mat3 &a, const Mat3 &b) { return Mat3(a.c[0] + b.c[0], a.c[1] + b.c[1], a.c[2] + b.c[2]); }
inline Mat3 operator-(const Mat3 &a, const Mat3 &b) { return Mat3(a.c[0] - b.c[0], a.c[1] - b.c[1], a.c[2] - b.c[2]); }
inline bool operator==(const Mat3 &a, const Mat3 &b) { return a.c[0] == b.c[0] && a.c[1] == b.c[1] && a.c[2] == b.c[2]; }
inline bool operator!=(const Mat3 &a, const Mat3 &b) { return !(a == b); }

// ============================================================================
// Mat4  (column-major: c[col][row])
// ============================================================================
struct Mat4
{
    Vec4 c[4];

    Mat4() : Mat4(1.0f) {}
    explicit Mat4(float diagonal)
    {
        c[0] = Vec4(diagonal, 0.0f, 0.0f, 0.0f);
        c[1] = Vec4(0.0f, diagonal, 0.0f, 0.0f);
        c[2] = Vec4(0.0f, 0.0f, diagonal, 0.0f);
        c[3] = Vec4(0.0f, 0.0f, 0.0f, diagonal);
    }
    Mat4(const Vec4 &col0, const Vec4 &col1, const Vec4 &col2, const Vec4 &col3)
    {
        c[0] = col0;
        c[1] = col1;
        c[2] = col2;
        c[3] = col3;
    }
    // Embeds a 3x3 matrix in the upper-left corner.
    explicit Mat4(const Mat3 &m)
    {
        c[0] = Vec4(m.c[0], 0.0f);
        c[1] = Vec4(m.c[1], 0.0f);
        c[2] = Vec4(m.c[2], 0.0f);
        c[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    static Mat4 identity() { return Mat4(1.0f); }

    Vec4 &operator[](int col) { return c[col]; }
    const Vec4 &operator[](int col) const { return c[col]; }
    const float *data() const { return &c[0].x; }

    // Upper-left 3x3 (rotation/scale part).
    Mat3 toMat3() const
    {
        return Mat3(c[0].xyz(), c[1].xyz(), c[2].xyz());
    }

    // ---- Transform factories -------------------------------------------------

    static Mat4 translate(const Vec3 &t)
    {
        Mat4 m;
        m.c[3] = Vec4(t, 1.0f);
        return m;
    }

    static Mat4 scale(const Vec3 &s)
    {
        Mat4 m;
        m.c[0].x = s.x;
        m.c[1].y = s.y;
        m.c[2].z = s.z;
        return m;
    }

    // Rotation of 'angle' radians around an arbitrary axis (right-handed).
    static Mat4 rotate(float angle, const Vec3 &axis)
    {
        Vec3 a = normalize(axis);
        float cs = std::cos(angle);
        float sn = std::sin(angle);
        float t = 1.0f - cs;

        Mat4 m;
        m.c[0] = Vec4(t * a.x * a.x + cs, t * a.x * a.y + sn * a.z, t * a.x * a.z - sn * a.y, 0.0f);
        m.c[1] = Vec4(t * a.x * a.y - sn * a.z, t * a.y * a.y + cs, t * a.y * a.z + sn * a.x, 0.0f);
        m.c[2] = Vec4(t * a.x * a.z + sn * a.y, t * a.y * a.z - sn * a.x, t * a.z * a.z + cs, 0.0f);
        return m;
    }

    static Mat4 rotateX(float angle)
    {
        float cs = std::cos(angle), sn = std::sin(angle);
        Mat4 m;
        m.c[1] = Vec4(0.0f, cs, sn, 0.0f);
        m.c[2] = Vec4(0.0f, -sn, cs, 0.0f);
        return m;
    }

    static Mat4 rotateY(float angle)
    {
        float cs = std::cos(angle), sn = std::sin(angle);
        Mat4 m;
        m.c[0] = Vec4(cs, 0.0f, -sn, 0.0f);
        m.c[2] = Vec4(sn, 0.0f, cs, 0.0f);
        return m;
    }

    static Mat4 rotateZ(float angle)
    {
        float cs = std::cos(angle), sn = std::sin(angle);
        Mat4 m;
        m.c[0] = Vec4(cs, sn, 0.0f, 0.0f);
        m.c[1] = Vec4(-sn, cs, 0.0f, 0.0f);
        return m;
    }

    // ---- Camera / projection -------------------------------------------------

    // Right-handed view matrix (camera looks down its -Z axis).
    static Mat4 lookAt(const Vec3 &eye, const Vec3 &center, const Vec3 &up)
    {
        Vec3 f = normalize(center - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);

        Mat4 m;
        m.c[0] = Vec4(s.x, u.x, -f.x, 0.0f);
        m.c[1] = Vec4(s.y, u.y, -f.y, 0.0f);
        m.c[2] = Vec4(s.z, u.z, -f.z, 0.0f);
        m.c[3] = Vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f);
        return m;
    }

    // Perspective projection for VULKAN clip space: depth range 0..1, +Y down.
    // fovY is the vertical field of view in radians.
    static Mat4 perspective(float fovY, float aspect, float zNear, float zFar)
    {
        float tanHalf = std::tan(fovY * 0.5f);

        Mat4 m(0.0f);
        m.c[0].x = 1.0f / (aspect * tanHalf);
        m.c[1].y = -1.0f / tanHalf; // negative: Vulkan's Y axis points down
        m.c[2].z = zFar / (zNear - zFar);
        m.c[2].w = -1.0f;
        m.c[3].z = -(zFar * zNear) / (zFar - zNear);
        return m;
    }

    // Orthographic projection for VULKAN clip space (depth 0..1).
    // ortho(0, width, 0, height, ...) puts (0,0) at the TOP-left of the screen with +Y down.
    static Mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        Mat4 m;
        m.c[0].x = 2.0f / (right - left);
        m.c[1].y = 2.0f / (top - bottom);
        m.c[2].z = -1.0f / (zFar - zNear);
        m.c[3] = Vec4(-(right + left) / (right - left),
                      -(top + bottom) / (top - bottom),
                      -zNear / (zFar - zNear),
                      1.0f);
        return m;
    }

    // ---- Operations ----------------------------------------------------------

    Mat4 transpose() const
    {
        return Mat4(Vec4(c[0].x, c[1].x, c[2].x, c[3].x),
                    Vec4(c[0].y, c[1].y, c[2].y, c[3].y),
                    Vec4(c[0].z, c[1].z, c[2].z, c[3].z),
                    Vec4(c[0].w, c[1].w, c[2].w, c[3].w));
    }

    float determinant() const
    {
        const float *m = data();
        float c0 = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
                   m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        float c1 = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
                   m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        float c2 = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
                   m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        float c3 = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
                   m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        return m[0] * c0 + m[1] * c1 + m[2] * c2 + m[3] * c3;
    }

    // General 4x4 inverse. Returns identity if the matrix is singular.
    Mat4 inverse() const
    {
        const float *m = data();
        float inv[16];

        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
                 m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
                 m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
                 m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
                  m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
                 m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
                 m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
                 m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
                  m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
                 m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
                 m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
                  m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
                  m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
                 m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
                 m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
                  m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
                  m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if (det == 0.0f)
            return Mat4();

        float invDet = 1.0f / det;
        Mat4 result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.c[i][j] = inv[i * 4 + j] * invDet;
        return result;
    }
};

inline Vec4 operator*(const Mat4 &m, const Vec4 &v)
{
    return m.c[0] * v.x + m.c[1] * v.y + m.c[2] * v.z + m.c[3] * v.w;
}
inline Mat4 operator*(const Mat4 &a, const Mat4 &b)
{
    return Mat4(a * b.c[0], a * b.c[1], a * b.c[2], a * b.c[3]);
}
inline Mat4 operator*(const Mat4 &m, float s)
{
    return Mat4(m.c[0] * s, m.c[1] * s, m.c[2] * s, m.c[3] * s);
}
inline Mat4 operator*(float s, const Mat4 &m) { return m * s; }
inline Mat4 operator+(const Mat4 &a, const Mat4 &b)
{
    return Mat4(a.c[0] + b.c[0], a.c[1] + b.c[1], a.c[2] + b.c[2], a.c[3] + b.c[3]);
}
inline Mat4 operator-(const Mat4 &a, const Mat4 &b)
{
    return Mat4(a.c[0] - b.c[0], a.c[1] - b.c[1], a.c[2] - b.c[2], a.c[3] - b.c[3]);
}
inline bool operator==(const Mat4 &a, const Mat4 &b)
{
    return a.c[0] == b.c[0] && a.c[1] == b.c[1] && a.c[2] == b.c[2] && a.c[3] == b.c[3];
}
inline bool operator!=(const Mat4 &a, const Mat4 &b) { return !(a == b); }

// Transforms a point (w = 1): applies translation.
inline Vec3 transformPoint(const Mat4 &m, const Vec3 &p) { return (m * Vec4(p, 1.0f)).xyz(); }
// Transforms a direction (w = 0): ignores translation.
inline Vec3 transformDirection(const Mat4 &m, const Vec3 &d) { return (m * Vec4(d, 0.0f)).xyz(); }