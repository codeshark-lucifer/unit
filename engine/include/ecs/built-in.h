#pragma once
#include "Mathf/Mathf.h"
#include "graphics/mesh.h"
#include "vk/buffer.h"
#include <memory>

struct Transform
{
    Vec3 position{0.0f};
    Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Vec3 scale{1.0f};
    
    // Computes the 4x4 Model matrix (Translation * Rotation * Scale)
    Mat4 GetMatrix() const
    {
        return composeTRS(position, rotation, scale);
    }
};

struct MeshFilter
{
    std::shared_ptr<Mesh> shared_mesh;
};

struct MeshRenderer
{
    Buffer vertex, index;
    uint32_t count = 0; // index count
};

struct Camera
{
    float fovY = 1.04719755f;    // Field of View in radians (~60 degrees)
    float aspect = 16.0f / 9.0f; // Aspect ratio (width / height)
    float zNear = 0.1f;          // Near clipping plane
    float zFar = 100.0f;         // Far clipping plane
    bool isPrimary = true;       // Flag to easily find the active render camera

    // Computes the Vulkan projection matrix (0..1 depth, +Y down)
    Mat4 GetProjectionMatrix() const
    {
        return Mat4::perspective(fovY, aspect, zNear, zFar);
    }

    // Computes the view matrix using world position and orientation (from Transform)
    // In a right-handed system, the camera looks down its local -Z axis.
    Mat4 GetViewMatrix(const Vec3 &position, const Quat &rotation) const
    {
        // Calculate the camera's forward, up, and right vectors from its rotation quaternion
        Vec3 forward = rotation * Vec3(0.0f, 0.0f, -1.0f);
        Vec3 up = rotation * Vec3(0.0f, 1.0f, 0.0f);
        Vec3 center = position + forward;

        return Mat4::lookAt(position, center, up);
    }
};