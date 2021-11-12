#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>
#include <mono/jit/jit.h>

namespace SmolEngine
{
    struct TransformComponentCSharp
    {
        glm::vec3  WorldPos;
        glm::vec3  Rotation;
        glm::vec3  Scale;
        uint32_t*  Handler;
    };

    struct MeshComponentCSharp
    {
        bool      IsVisible;
        bool      IsActive;
        size_t    MeshID;
        uint32_t* Handler;
    };

    struct RigidBodyComponentCSharp
    {
        bool      Created;
        uint16_t  Shape;
        uint16_t  Type;
        uint32_t* Handler;
    };

    struct CameraComponentCSharp
    {
        float     FOV;
        float     zNear;
        float     zFar;
        float     Zoom;
        bool      IsPrimary;
        uint32_t* Handler;
    };

    struct RigidBodyCreateInfoCSharp
    {
        uint16_t    eShape;
        uint16_t    eType;
        float       Mass;
        float       Density;
        float       Friction;
        float       Restitution;
        float       LinearDamping;
        float       AngularDamping;
        float       RollingFriction;
        float       SpinningFriction;
        glm::vec3   Size;
        void*       Path; // for custom geometry
    };

    struct PointLightComponentCSharp
    {
        glm::vec3   Color;
        float       Intensity;
        float       Raduis;
        bool        IsActive;
        uint32_t*   Handler;
    };

    struct DirectionalLightComponentCSharp
    {
        glm::vec3   Color;
        glm::vec3   Dir;
        uint16_t    ShadowType;
        float       Intensity;
        bool        IsActive;
        uint32_t*   Handler;
    };
}