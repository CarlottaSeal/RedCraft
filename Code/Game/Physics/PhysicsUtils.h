#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

class World;
class Block;

constexpr float PLAYER_WIDTH = 0.6f;           // Player is 0.6 blocks wide
constexpr float PLAYER_HEIGHT = 1.8f;          // Player is 1.8 blocks tall
constexpr float PLAYER_EYE_HEIGHT = 1.62f;     // Eye level from feet
constexpr float PLAYER_CROUCH_HEIGHT = 1.5f;   // Height when crouching

constexpr float GRAVITY = -20.0f;              // Gravity acceleration (blocks/s^2)
constexpr float TERMINAL_VELOCITY = -50.0f;    // Max fall speed (blocks/s)
constexpr float JUMP_VELOCITY = 8.0f;          // Initial jump velocity
constexpr float WALK_SPEED = 4.3f;             // Walking speed (blocks/s)
constexpr float RUN_SPEED = 5.6f;              // Running speed (blocks/s)
constexpr float SWIM_SPEED = 2.0f;             // Swimming speed (blocks/s)
constexpr float FLY_SPEED = 10.9f;             // Creative fly speed (blocks/s)

constexpr float WATER_GRAVITY_MULTIPLIER = 0.2f;  // Gravity reduction in water
constexpr float WATER_DRAG = 0.8f;                // Velocity damping in water
constexpr float AIR_DRAG = 2.f;                   // Air resistance
constexpr float GROUND_DRAG = 15.f;               
constexpr float GROUND_ACCELERATION = 64.f;

constexpr float CAMERA_OVER_SHOULDER_DIST = 4.f;          

Vec3 GetAABBPenetration(const AABB3& a, const AABB3& b);

Vec3 ResolveAABBCollision(const AABB3& movingBox, const AABB3& staticBox, const Vec3& velocity);

struct StepResult
{
    bool m_canStep = false;
    float m_stepHeight = 0.0f;
    Vec3 m_stepPosition;
};

StepResult CheckStepUp(const Vec3& desiredPos,
                       float maxStepHeight);

// Apply friction to horizontal velocity
Vec3 ApplyFriction(const Vec3& velocity, float friction, float deltaSeconds);

// Apply friction to all three axes (for flying/noclip modes)
Vec3 ApplyFriction3D(const Vec3& velocity, float friction, float deltaSeconds);

// Apply acceleration with max speed limit
Vec3 Accelerate(const Vec3& velocity, const Vec3& wishDir, float wishSpeed, 
                       float accel, float deltaSeconds);