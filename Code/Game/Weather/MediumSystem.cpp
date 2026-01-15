#include "MediumSystem.h"
#include "Game/World.h"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "WeatherSystem.h"
#include "Game/ChunkUtils.h"

MediumSystem::MediumSystem(World* world)
    : m_world(world)
{
}

MediumSystem::~MediumSystem()
{
}

void MediumSystem::Update(float deltaSeconds)
{
    m_previousMedium = m_currentMedium;
    UpdateCurrentMedium();
    
    if (m_currentMedium != m_previousMedium)
        m_mediumTransitionTimer = 0.0f;
    else if (m_mediumTransitionTimer < m_mediumTransitionDuration)
        m_mediumTransitionTimer += deltaSeconds;
    
    if (m_currentMedium == MediumType::WATER)
        UpdateUnderwaterState(deltaSeconds);
    else
        m_underwaterDepth = 0.0f;
    
    UpdateDistortionAnimation(deltaSeconds);
    m_currentProperties = CalculateCombinedProperties();
}

void MediumSystem::UpdateCurrentMedium()
{
    Vec3 eyePos = m_world->m_owner->m_player->GetEyePosition();
    m_currentMedium = GetMediumAtPosition(eyePos);
}

void MediumSystem::UpdateUnderwaterState(float deltaSeconds)
{
    UNUSED(deltaSeconds);
    Vec3 eyePos = m_world->m_owner->m_player->GetEyePosition();
    
    int searchZ = (int)eyePos.z;
    m_waterSurfaceZ = eyePos.z;
    
    for (int z = searchZ; z < CHUNK_SIZE_Z; z++)
    {
        Block block = m_world->GetBlockAtWorldCoords((int)eyePos.x, (int)eyePos.y, z);
        if (block.m_typeIndex != BLOCK_TYPE_WATER)
        {
            m_waterSurfaceZ = (float)z;
            break;
        }
    }
    
    m_underwaterDepth = m_waterSurfaceZ - eyePos.z;
    if (m_underwaterDepth < 0.0f)
        m_underwaterDepth = 0.0f;
}

void MediumSystem::UpdateDistortionAnimation(float deltaSeconds)
{
    float speed = 1.0f;
    if (m_currentMedium == MediumType::WATER) speed = 1.5f;
    else if (m_currentMedium == MediumType::LAVA) speed = 0.8f;
    m_distortionTime += deltaSeconds * speed;
}

MediumType MediumSystem::GetMediumAtPosition(const Vec3& position) const
{
    if (position.z < 0 || position.z >= CHUNK_SIZE_Z)
        return MediumType::AIR;
    
    Block block = m_world->GetBlockAtWorldCoords(
        (int)floorf(position.x),
        (int)floorf(position.y),
        (int)floorf(position.z)
    );
    
    if (block.m_typeIndex == BLOCK_TYPE_WATER || IsUnderwaterProduct(block.m_typeIndex))
        return MediumType::WATER;
    if (block.m_typeIndex == BLOCK_TYPE_LAVA)
        return MediumType::LAVA;
    
    if (m_world->m_weatherSystem && m_world->m_weatherSystem->IsFoggy())
        return MediumType::FOG;
    
    return MediumType::AIR;
}

MediumProperties MediumSystem::GetPropertiesForMedium(MediumType type) const
{
    MediumProperties props;
    
    switch (type)
    {
    case MediumType::WATER:
        {
            float depthFactor = GetClamped(m_underwaterDepth / 50.0f, 0.0f, 1.0f);
            Rgba8 shallow(60, 140, 180, 255);
            Rgba8 deep(10, 40, 80, 255);
            props.tintColor.r = (unsigned char)((1.0f - depthFactor) * shallow.r + depthFactor * deep.r);
            props.tintColor.g = (unsigned char)((1.0f - depthFactor) * shallow.g + depthFactor * deep.g);
            props.tintColor.b = (unsigned char)((1.0f - depthFactor) * shallow.b + depthFactor * deep.b);
            props.tintColor.a = 255;
            props.density = 0.7f + 0.25f * depthFactor;
            props.fogNearDistance = 3.0f - 2.0f * depthFactor;
            props.fogFarDistance = 25.0f - 15.0f * depthFactor;
            props.enableDistortion = true;
            props.distortionStrength = 0.015f;
        }
        break;
    case MediumType::LAVA:
        props.tintColor = Rgba8(255, 80, 0, 255);
        props.density = 0.95f;
        props.fogNearDistance = 0.5f;
        props.fogFarDistance = 3.0f;
        props.enableDistortion = true;
        props.distortionStrength = 0.03f;
        break;
    case MediumType::FOG:
        props.tintColor = Rgba8(200, 210, 220, 255);
        props.density = 0.5f;
        props.fogNearDistance = 10.0f;
        props.fogFarDistance = 60.0f;
        break;
    default:
        props.tintColor = Rgba8(255, 255, 255, 255);
        props.density = 0.0f;
        props.fogNearDistance = 200.0f;
        props.fogFarDistance = 400.0f;
        break;
    }
    
    return props;
}

MediumProperties MediumSystem::CalculateCombinedProperties() const
{
    MediumProperties props = GetPropertiesForMedium(m_currentMedium);
    
    if (m_currentMedium == MediumType::AIR && m_world->m_weatherSystem)
        props = m_world->m_weatherSystem->GetWeatherMediumProperties();
    
    if (m_mediumTransitionTimer < m_mediumTransitionDuration && m_previousMedium != m_currentMedium)
    {
        MediumProperties prev = GetPropertiesForMedium(m_previousMedium);
        float t = m_mediumTransitionTimer / m_mediumTransitionDuration;
        t = t * t * (3.0f - 2.0f * t);
        props = MediumProperties::Lerp(prev, props, t);
    }
    
    return props;
}