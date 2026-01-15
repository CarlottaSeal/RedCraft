#include "PhysicsUtils.h"

Vec3 GetAABBPenetration(const AABB3& a, const AABB3& b)
{
    Vec3 penetration;
    
    // X-axis
    float leftPen = a.m_maxs.x - b.m_mins.x;
    float rightPen = b.m_maxs.x - a.m_mins.x;
    penetration.x = (leftPen < rightPen) ? leftPen : -rightPen;
    
    // Y-axis
    float backPen = a.m_maxs.y - b.m_mins.y;
    float frontPen = b.m_maxs.y - a.m_mins.y;
    penetration.y = (backPen < frontPen) ? backPen : -frontPen;
    
    // Z-axis
    float topPen = a.m_maxs.z - b.m_mins.z;
    float bottomPen = b.m_maxs.z - a.m_mins.z;
    penetration.z = (topPen < bottomPen) ? topPen : -bottomPen;
    
    return penetration;
}

Vec3 ResolveAABBCollision(const AABB3& movingBox, const AABB3& staticBox, const Vec3& velocity)
{
    Vec3 penetration = GetAABBPenetration(movingBox, staticBox);
    
    // Find axis with smallest penetration
    float minPen = fabsf(penetration.x);
    Vec3 pushOut = Vec3(penetration.x, 0, 0);
    
    if (fabsf(penetration.y) < minPen)
    {
        minPen = fabsf(penetration.y);
        pushOut = Vec3(0, penetration.y, 0);
    }
    
    if (fabsf(penetration.z) < minPen)
    {
        pushOut = Vec3(0, 0, penetration.z);
    }
    
    // Push out in direction opposite to velocity
    if (DotProduct3D(pushOut, velocity) > 0)
    {
        pushOut = -pushOut;
    }
    
    return pushOut;
}

StepResult CheckStepUp(const Vec3& desiredPos, float maxStepHeight)
{
    StepResult result;
    
    // Try different step heights
    for (float stepHeight = 0.1f; stepHeight <= maxStepHeight; stepHeight += 0.1f)
    {
        Vec3 stepPos = desiredPos + Vec3(0, 0, stepHeight);
        
        // Check if position is valid at this height
        AABB3 testBox(stepPos - Vec3(PLAYER_WIDTH * 0.5f, PLAYER_WIDTH * 0.5f, PLAYER_HEIGHT * 0.5f),
                     stepPos + Vec3(PLAYER_WIDTH * 0.5f, PLAYER_WIDTH * 0.5f, PLAYER_HEIGHT * 0.5f));
        
        bool collision = false;
        // Would need to check collision with world here
        
        if (!collision)
        {
            result.m_canStep = true;
            result.m_stepHeight = stepHeight;
            result.m_stepPosition = stepPos;
            break;
        }
    }
    
    return result;
}

Vec3 ApplyFriction(const Vec3& velocity, float friction, float deltaSeconds)
{
    Vec3 result = velocity;
    float speed = Vec2(velocity.x, velocity.y).GetLength();
    
    if (speed > 0.01f)
    {
        float drop = speed * friction * deltaSeconds;
        float newSpeed = fmaxf(speed - drop, 0.0f);
        float scale = newSpeed / speed;
        result.x *= scale;
        result.y *= scale;
    }
    
    return result;
}

Vec3 ApplyFriction3D(const Vec3& velocity, float friction, float deltaSeconds)
{
    Vec3 result = velocity;
    float speed = velocity.GetLength();
    
    if (speed > 0.01f)
    {
        float drop = speed * friction * deltaSeconds;
        float newSpeed = fmaxf(speed - drop, 0.0f);
        float scale = newSpeed / speed;
        result = result * scale;
    }
    
    return result;
}

Vec3 Accelerate(const Vec3& velocity, const Vec3& wishDir, float wishSpeed, float accel, float deltaSeconds)
{
    float currentSpeed = DotProduct3D(velocity, wishDir);
    float addSpeed = wishSpeed - currentSpeed;
    
    if (addSpeed <= 0)
        return velocity;
    
    float accelSpeed = accel * deltaSeconds * wishSpeed;
    if (accelSpeed > addSpeed)
        accelSpeed = addSpeed;
    
    return velocity + wishDir * accelSpeed;
}