#pragma once
#include "MediumTypes.h"
#include "Engine/Math/Vec3.hpp"

class World;

class MediumSystem
{
public:
    MediumSystem(World* world);
    ~MediumSystem();
    
    void Update(float deltaSeconds);
    
    MediumType          GetCurrentMediumType() const { return m_currentMedium; }
    MediumProperties    GetCurrentMediumProperties() const { return m_currentProperties; }
    MediumType          GetMediumAtPosition(const Vec3& position) const;
    MediumProperties    GetPropertiesForMedium(MediumType type) const;
    
    bool IsUnderwater() const { return m_currentMedium == MediumType::WATER; }
    bool IsInLava() const { return m_currentMedium == MediumType::LAVA; }
    bool IsInFog() const { return m_currentMedium == MediumType::FOG; }
    
    float GetUnderwaterDepth() const { return m_underwaterDepth; }
    float GetDistortionTime() const { return m_distortionTime; }
    
private:
    void UpdateCurrentMedium();
    void UpdateUnderwaterState(float deltaSeconds);
    void UpdateDistortionAnimation(float deltaSeconds);
    MediumProperties CalculateCombinedProperties() const;
    
    World*              m_world = nullptr;
    MediumType          m_currentMedium = MediumType::AIR;
    MediumType          m_previousMedium = MediumType::AIR;
    MediumProperties    m_currentProperties;
    
    float               m_underwaterDepth = 0.0f;
    float               m_waterSurfaceZ = 0.0f;
    float               m_mediumTransitionTimer = 0.0f;
    float               m_mediumTransitionDuration = 0.5f;
    float               m_distortionTime = 0.0f;
};