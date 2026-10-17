#pragma once

// Quat.h - unit quaternion for 3D rotation.
//
// Layout is (x, y, z, w) with w the scalar part. Identity is (0, 0, 0, 1).
// Same handedness as Mat4::rotate: a positive angle rotates counter-clockwise
// when looking down the axis toward the origin (right-handed).
//
// Euler convention used by fromEuler / toEuler:
//   euler = (pitch about X, yaw about Y, roll about Z), radians,
//   applied in the order Z (roll), then X (pitch), then Y (yaw)  ->  q = qY * qX * qZ

#include <cmath>
#include "Mathf/Vec.h"
#include "Mathf/Matrix.h"

struct Quat
{
    float x, y, z, w;

    Quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Quat(float i, float j, float k, float l) : x(i), y(j), z(k), w(l) {}

    static Quat identity() { return Quat(); }

    // Rotation of 'angle' radians around 'axis' (axis does not need to be normalized).
    static Quat fromAxisAngle(const Vec3 &axis, float angle)
    {
        Vec3 a = normalize(axis);
        float half = angle * 0.5f;
        float s = std::sin(half);
        return Quat(a.x * s, a.y * s, a.z * s, std::cos(half));
    }

    inline static Quat fromEuler(const Vec3 &euler); // defined below (needs operator*)

    Vec3 xyz() const { return Vec3(x, y, z); }
    const float *data() const { return &x; }

    Quat operator-() const { return Quat(-x, -y, -z, -w); }

    Quat conjugate() const { return Quat(-x, -y, -z, w); }

    // Inverse of a unit quaternion is its conjugate; this also handles non-unit ones.
    Quat inverse() const
    {
        float n = x * x + y * y + z * z + w * w;
        if (n == 0.0f)
            return Quat();
        float inv = 1.0f / n;
        return Quat(-x * inv, -y * inv, -z * inv, w * inv);
    }

    Quat normalized() const
    {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len == 0.0f)
            return Quat();
        float inv = 1.0f / len;
        return Quat(x * inv, y * inv, z * inv, w * inv);
    }

    // Rotates a vector by this (unit) quaternion.
    Vec3 rotate(const Vec3 &v) const
    {
        Vec3 u(x, y, z);
        Vec3 t = cross(u, v) * 2.0f;
        return v + t * w + cross(u, t);
    }

    Mat3 toMat3() const
    {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        return Mat3(Vec3(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy)),
                    Vec3(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx)),
                    Vec3(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy)));
    }

    Mat4 toMat4() const { return Mat4(toMat3()); }

    // Returns (pitch X, yaw Y, roll Z) in radians, inverse of fromEuler.
    Vec3 toEuler() const
    {
        Mat3 m = toMat3();
        float m12 = m.c[2].y; // row 1, col 2
        float pitch = std::asin(m12 < -1.0f ? 1.0f : (m12 > 1.0f ? -1.0f : -m12));
        float yaw, roll;

        if (std::fabs(m12) < 0.9999999f)
        {
            yaw = std::atan2(m.c[2].x, m.c[2].z);
            roll = std::atan2(m.c[0].y, m.c[1].y);
        }
        else // gimbal lock
        {
            yaw = std::atan2(-m.c[0].z, m.c[0].x);
            roll = 0.0f;
        }
        return Vec3(pitch, yaw, roll);
    }
};

inline Quat operator+(const Quat &a, const Quat &b) { return Quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
inline Quat operator-(const Quat &a, const Quat &b) { return Quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
inline Quat operator*(const Quat &q, float s) { return Quat(q.x * s, q.y * s, q.z * s, q.w * s); }
inline Quat operator*(float s, const Quat &q) { return q * s; }
inline bool operator==(const Quat &a, const Quat &b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
inline bool operator!=(const Quat &a, const Quat &b) { return !(a == b); }

// Hamilton product: (a * b) applies b first, then a.
inline Quat operator*(const Quat &a, const Quat &b)
{
    return Quat(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

// Rotates a vector: q * v.
inline Vec3 operator*(const Quat &q, const Vec3 &v) { return q.rotate(v); }

inline float dot(const Quat &a, const Quat &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline float length(const Quat &q) { return std::sqrt(dot(q, q)); }
inline Quat normalize(const Quat &q) { return q.normalized(); }

// Normalized linear interpolation: cheap, good for small angles.
inline Quat nlerp(const Quat &a, const Quat &b, float t)
{
    Quat bb = dot(a, b) < 0.0f ? -b : b; // take the shortest path
    return (a * (1.0f - t) + bb * t).normalized();
}

// Spherical linear interpolation: constant angular speed.
inline Quat slerp(const Quat &a, const Quat &b, float t)
{
    float d = dot(a, b);
    Quat bb = b;
    if (d < 0.0f)
    {
        bb = -b;
        d = -d;
    }

    if (d > 0.9995f) // nearly parallel: fall back to nlerp to avoid dividing by ~0
        return nlerp(a, bb, t);

    float theta = std::acos(d);
    float sinTheta = std::sin(theta);
    float wa = std::sin((1.0f - t) * theta) / sinTheta;
    float wb = std::sin(t * theta) / sinTheta;
    return a * wa + bb * wb;
}

inline Quat Quat::fromEuler(const Vec3 &e)
{
    Quat qx = fromAxisAngle(Vec3(1.0f, 0.0f, 0.0f), e.x);
    Quat qy = fromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), e.y);
    Quat qz = fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), e.z);
    return qy * qx * qz;
}

// Builds a model matrix: translate * rotate * scale.
inline Mat4 composeTRS(const Vec3 &position, const Quat &rotation, const Vec3 &scale)
{
    return Mat4::translate(position) * rotation.toMat4() * Mat4::scale(scale);
}