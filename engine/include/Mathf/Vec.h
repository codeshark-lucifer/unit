#pragma once

// Vec.h - Vec2 / Vec3 / Vec4 and their free functions.
//
// Note: Vec3 is 12 bytes. In GLSL std140 uniform blocks a vec3 is padded to 16 bytes,
// so prefer Vec4 (or explicit padding) for vec3 members of uniform/storage buffers.

#include <cmath>

// ============================================================================
// Vec2
// ============================================================================
struct Vec2
{
    float x, y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float v) : x(v), y(v) {}
    Vec2(float i, float j) : x(i), y(j) {}

    float &operator[](int i) { return (&x)[i]; }
    const float &operator[](int i) const { return (&x)[i]; }
    const float *data() const { return &x; }

    Vec2 operator-() const { return Vec2(-x, -y); }

    Vec2 &operator+=(const Vec2 &o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }
    Vec2 &operator-=(const Vec2 &o)
    {
        x -= o.x;
        y -= o.y;
        return *this;
    }
    Vec2 &operator*=(const Vec2 &o)
    {
        x *= o.x;
        y *= o.y;
        return *this;
    }
    Vec2 &operator/=(const Vec2 &o)
    {
        x /= o.x;
        y /= o.y;
        return *this;
    }
    Vec2 &operator*=(float s)
    {
        x *= s;
        y *= s;
        return *this;
    }
    Vec2 &operator/=(float s)
    {
        x /= s;
        y /= s;
        return *this;
    }
};

inline Vec2 operator+(Vec2 a, const Vec2 &b) { return a += b; }
inline Vec2 operator-(Vec2 a, const Vec2 &b) { return a -= b; }
inline Vec2 operator*(Vec2 a, const Vec2 &b) { return a *= b; }
inline Vec2 operator/(Vec2 a, const Vec2 &b) { return a /= b; }
inline Vec2 operator*(Vec2 a, float s) { return a *= s; }
inline Vec2 operator*(float s, Vec2 a) { return a *= s; }
inline Vec2 operator/(Vec2 a, float s) { return a /= s; }
inline bool operator==(const Vec2 &a, const Vec2 &b) { return a.x == b.x && a.y == b.y; }
inline bool operator!=(const Vec2 &a, const Vec2 &b) { return !(a == b); }

inline float dot(const Vec2 &a, const Vec2 &b) { return a.x * b.x + a.y * b.y; }
inline float lengthSq(const Vec2 &v) { return dot(v, v); }
inline float length(const Vec2 &v) { return std::sqrt(dot(v, v)); }
inline float distance(const Vec2 &a, const Vec2 &b) { return length(a - b); }
inline Vec2 normalize(const Vec2 &v)
{
    float len = length(v);
    return len > 0.0f ? v / len : Vec2(0.0f);
}
inline Vec2 lerp(const Vec2 &a, const Vec2 &b, float t) { return a + (b - a) * t; }
// 2D "cross product": the z component of the 3D cross product.
inline float cross(const Vec2 &a, const Vec2 &b) { return a.x * b.y - a.y * b.x; }

// ============================================================================
// Vec3
// ============================================================================
struct Vec3
{
    float x, y, z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float v) : x(v), y(v), z(v) {}
    Vec3(float i, float j, float k) : x(i), y(j), z(k) {}
    Vec3(const Vec2 &xy, float k = 0.0f) : x(xy.x), y(xy.y), z(k) {}

    Vec2 xy() const { return Vec2(x, y); }

    float &operator[](int i) { return (&x)[i]; }
    const float &operator[](int i) const { return (&x)[i]; }
    const float *data() const { return &x; }

    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    Vec3 &operator+=(const Vec3 &o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    Vec3 &operator-=(const Vec3 &o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }
    Vec3 &operator*=(const Vec3 &o)
    {
        x *= o.x;
        y *= o.y;
        z *= o.z;
        return *this;
    }
    Vec3 &operator/=(const Vec3 &o)
    {
        x /= o.x;
        y /= o.y;
        z /= o.z;
        return *this;
    }
    Vec3 &operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    Vec3 &operator/=(float s)
    {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }
};

inline Vec3 operator+(Vec3 a, const Vec3 &b) { return a += b; }
inline Vec3 operator-(Vec3 a, const Vec3 &b) { return a -= b; }
inline Vec3 operator*(Vec3 a, const Vec3 &b) { return a *= b; }
inline Vec3 operator/(Vec3 a, const Vec3 &b) { return a /= b; }
inline Vec3 operator*(Vec3 a, float s) { return a *= s; }
inline Vec3 operator*(float s, Vec3 a) { return a *= s; }
inline Vec3 operator/(Vec3 a, float s) { return a /= s; }
inline bool operator==(const Vec3 &a, const Vec3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(const Vec3 &a, const Vec3 &b) { return !(a == b); }

inline float dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float lengthSq(const Vec3 &v) { return dot(v, v); }
inline float length(const Vec3 &v) { return std::sqrt(dot(v, v)); }
inline float distance(const Vec3 &a, const Vec3 &b) { return length(a - b); }
inline Vec3 normalize(const Vec3 &v)
{
    float len = length(v);
    return len > 0.0f ? v / len : Vec3(0.0f);
}
inline Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}
inline Vec3 lerp(const Vec3 &a, const Vec3 &b, float t) { return a + (b - a) * t; }
// Reflects incident vector i around normal n (n must be normalized).
inline Vec3 reflect(const Vec3 &i, const Vec3 &n) { return i - n * (2.0f * dot(n, i)); }

// ============================================================================
// Vec4
// ============================================================================
struct Vec4
{
    float x, y, z, w;

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float v) : x(v), y(v), z(v), w(v) {}
    Vec4(float i, float j, float k, float l) : x(i), y(j), z(k), w(l) {}
    Vec4(const Vec3 &xyz, float l = 1.0f) : x(xyz.x), y(xyz.y), z(xyz.z), w(l) {}
    Vec4(const Vec2 &xy, float k, float l) : x(xy.x), y(xy.y), z(k), w(l) {}

    Vec3 xyz() const { return Vec3(x, y, z); }
    Vec2 xy() const { return Vec2(x, y); }

    float &operator[](int i) { return (&x)[i]; }
    const float &operator[](int i) const { return (&x)[i]; }
    const float *data() const { return &x; }

    Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }

    Vec4 &operator+=(const Vec4 &o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        w += o.w;
        return *this;
    }
    Vec4 &operator-=(const Vec4 &o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        w -= o.w;
        return *this;
    }
    Vec4 &operator*=(const Vec4 &o)
    {
        x *= o.x;
        y *= o.y;
        z *= o.z;
        w *= o.w;
        return *this;
    }
    Vec4 &operator/=(const Vec4 &o)
    {
        x /= o.x;
        y /= o.y;
        z /= o.z;
        w /= o.w;
        return *this;
    }
    Vec4 &operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }
    Vec4 &operator/=(float s)
    {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }
};

inline Vec4 operator+(Vec4 a, const Vec4 &b) { return a += b; }
inline Vec4 operator-(Vec4 a, const Vec4 &b) { return a -= b; }
inline Vec4 operator*(Vec4 a, const Vec4 &b) { return a *= b; }
inline Vec4 operator/(Vec4 a, const Vec4 &b) { return a /= b; }
inline Vec4 operator*(Vec4 a, float s) { return a *= s; }
inline Vec4 operator*(float s, Vec4 a) { return a *= s; }
inline Vec4 operator/(Vec4 a, float s) { return a /= s; }
inline bool operator==(const Vec4 &a, const Vec4 &b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
inline bool operator!=(const Vec4 &a, const Vec4 &b) { return !(a == b); }

inline float dot(const Vec4 &a, const Vec4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline float lengthSq(const Vec4 &v) { return dot(v, v); }
inline float length(const Vec4 &v) { return std::sqrt(dot(v, v)); }
inline float distance(const Vec4 &a, const Vec4 &b) { return length(a - b); }
inline Vec4 normalize(const Vec4 &v)
{
    float len = length(v);
    return len > 0.0f ? v / len : Vec4(0.0f);
}
inline Vec4 lerp(const Vec4 &a, const Vec4 &b, float t) { return a + (b - a) * t; }