#include "WeatherSystem.h"

#include "Engine/Core/EngineCommon.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Gamecommon.hpp"

extern RandomNumberGenerator* g_theRNG;

WeatherSystem::WeatherSystem(World* world)
    : m_world(world)
{
    m_weatherState.m_weatherDuration = CalculateWeatherDuration(WeatherType::CLEAR);
    m_nextLightningTime = g_theRNG->RollRandomFloatInRange(10.0f, 40.0f);
}

WeatherSystem::~WeatherSystem() {}

void WeatherSystem::Update(float deltaSeconds)
{
    m_noiseTime += deltaSeconds;
    UpdateWeatherTransition(deltaSeconds);
    
    if (m_autoWeatherEnabled)
        UpdateAutomaticWeatherChange(deltaSeconds);
    
    UpdateWind(deltaSeconds);
    
    if (IsStormy())
        UpdateLightning(deltaSeconds);
    else
        m_lightningStrength = 0.0f;
}

void WeatherSystem::UpdateWeatherTransition(float deltaSeconds)
{
    if (m_weatherState.m_transitionTimer >= m_weatherState.m_transitionDuration)
        return;
    
    m_weatherState.m_transitionTimer += deltaSeconds;
    float t = m_weatherState.m_transitionTimer / m_weatherState.m_transitionDuration;
    t = t * t * (3.0f - 2.0f * t);  // SmoothStep
    
    m_weatherState.intensity = Interpolate(
        m_weatherState.intensity, 
        m_weatherState.m_targetIntensity, 
        t * 0.1f  
    );
    
    if (m_weatherState.m_transitionTimer >= m_weatherState.m_transitionDuration)
    {
        m_weatherState.currentType = m_weatherState.m_targetType;
        m_weatherState.intensity = m_weatherState.m_targetIntensity;
    }
}

void WeatherSystem::UpdateAutomaticWeatherChange(float deltaSeconds)
{
    m_weatherState.m_weatherDuration -= deltaSeconds;
    if (m_weatherState.m_weatherDuration <= 0.0f)
    {
        SetWeather(DetermineNextWeather(), 15.0f);
    }
}

void WeatherSystem::UpdateWind(float deltaSeconds)
{
    UNUSED(deltaSeconds);
    
    float windNoiseX = Compute1dPerlinNoise(m_noiseTime * 0.1f, 1.0f, 3, 0.5f, 2.0f, true, GAME_SEED + 300);
    float windNoiseY = Compute1dPerlinNoise(m_noiseTime * 0.1f + 100.0f, 1.0f, 3, 0.5f, 2.0f, true, GAME_SEED + 301);
    
    Vec2 targetWindDir = Vec2(windNoiseX, windNoiseY).GetNormalized();
    m_weatherState.windDirection = (m_weatherState.windDirection * 0.99f + targetWindDir * 0.01f).GetNormalized();
    
    float baseWind = 0.1f;
    switch (m_weatherState.currentType)
    {
    case WeatherType::CLEAR:            baseWind = 0.1f;  break;
    case WeatherType::CLOUDY:           baseWind = 0.2f;  break;
    case WeatherType::OVERCAST:         baseWind = 0.3f;  break;
    case WeatherType::LIGHT_RAIN:
    case WeatherType::LIGHT_SNOW:       baseWind = 0.4f;  break;
    case WeatherType::RAIN:
    case WeatherType::SNOW:
    case WeatherType::FOG:              baseWind = 0.5f;  break;
    case WeatherType::HEAVY_RAIN:
    case WeatherType::HEAVY_SNOW:       baseWind = 0.7f;  break;
    case WeatherType::THUNDERSTORM:     baseWind = 0.8f;  break;
    case WeatherType::BLIZZARD:         baseWind = 1.0f;  break;
    default:                            baseWind = 0.1f;  break;
    }
    
    m_weatherState.windStrength = baseWind * m_weatherState.intensity;
}

void WeatherSystem::UpdateLightning(float deltaSeconds)
{
    m_lightningTimer += deltaSeconds;
    
    // 闪电衰减
    if (m_lightningStrength > 0.0f)
    {
        m_lightningStrength -= deltaSeconds * 8.0f;
        m_lightningStrength = GetClamped(m_lightningStrength, 0.0f, 1.0f);
    }
    
    // 触发新闪电
    if (m_lightningTimer >= m_nextLightningTime)
    {
        m_lightningStrength = 1.0f;
        
        // 下次闪电间隔（强度越高，闪电越频繁）
        float minInterval = 3.0f / m_weatherState.intensity;
        float maxInterval = 15.0f / m_weatherState.intensity;
        m_nextLightningTime = m_lightningTimer + g_theRNG->RollRandomFloatInRange(minInterval, maxInterval);
    }
}

void WeatherSystem::SetWeather(WeatherType type, float transitionTime)
{
    if (m_weatherState.m_transitionDuration <= 0.01f)
    {
        m_weatherState.currentType = m_weatherState.m_targetType;
        m_weatherState.intensity = m_weatherState.m_targetIntensity;
        m_weatherState.m_transitionTimer = 0.0f;
        return;
    }
    
    if (type == m_weatherState.currentType && !m_weatherState.IsTransitioning())
        return;
    
    m_weatherState.m_targetType = type;
    m_weatherState.m_transitionDuration = transitionTime;
    m_weatherState.m_transitionTimer = 0.0f;
    m_weatherState.m_targetIntensity = GetBaseIntensityForWeather(type);
    m_weatherState.m_weatherDuration = CalculateWeatherDuration(type);
}

void WeatherSystem::SetWeatherIntensity(float intensity)
{
    m_weatherState.m_targetIntensity = GetClamped(intensity, 0.0f, 1.0f);
}

float WeatherSystem::GetBaseIntensityForWeather(WeatherType type) const
{
    switch (type)
    {
    case WeatherType::CLEAR:            return 0.0f;
    case WeatherType::CLOUDY:           return 0.2f;
    case WeatherType::OVERCAST:         return 0.4f;
    case WeatherType::LIGHT_RAIN:
    case WeatherType::LIGHT_SNOW:
    case WeatherType::FOG:              return 0.5f;
    case WeatherType::RAIN:
    case WeatherType::SNOW:             return 0.7f;
    case WeatherType::HEAVY_RAIN:
    case WeatherType::HEAVY_SNOW:       return 0.85f;
    case WeatherType::THUNDERSTORM:
    case WeatherType::BLIZZARD:         return 1.0f;
    default:                            return 0.5f;
    }
}

bool WeatherSystem::IsRaining() const
{
    WeatherType t = m_weatherState.currentType;
    return t == WeatherType::LIGHT_RAIN || t == WeatherType::RAIN || 
           t == WeatherType::HEAVY_RAIN || t == WeatherType::THUNDERSTORM;
}

bool WeatherSystem::IsSnowing() const
{
    WeatherType t = m_weatherState.currentType;
    return t == WeatherType::LIGHT_SNOW || t == WeatherType::SNOW || 
           t == WeatherType::HEAVY_SNOW || t == WeatherType::BLIZZARD;
}

bool WeatherSystem::IsStormy() const
{
    return m_weatherState.currentType == WeatherType::THUNDERSTORM;
}

bool WeatherSystem::IsFoggy() const
{
    return m_weatherState.currentType == WeatherType::FOG;
}

bool WeatherSystem::IsClear() const
{
    return m_weatherState.currentType == WeatherType::CLEAR;
}

float WeatherSystem::GetSkyDarkening() const
{
    float base = 0.0f;
    switch (m_weatherState.currentType)
    {
    case WeatherType::CLEAR:            base = 0.0f;  break;
    case WeatherType::CLOUDY:           base = 0.15f; break;
    case WeatherType::OVERCAST:         base = 0.3f;  break;
    case WeatherType::LIGHT_RAIN:
    case WeatherType::LIGHT_SNOW:       base = 0.35f; break;
    case WeatherType::RAIN:
    case WeatherType::SNOW:
    case WeatherType::FOG:              base = 0.45f; break;
    case WeatherType::HEAVY_RAIN:
    case WeatherType::HEAVY_SNOW:       base = 0.55f; break;
    case WeatherType::THUNDERSTORM:     base = 0.7f;  break;
    case WeatherType::BLIZZARD:         base = 0.6f;  break;
    default: break;
    }
    float cloudCoverage = GetCloudCoverage();
    float cloudDarkening = 0.0f;
    if (cloudCoverage > 0.4f)
    {
        cloudDarkening = (cloudCoverage - 0.4f) * 0.5f;  
    }
    
    return (base * m_weatherState.intensity) + cloudDarkening;
}

float WeatherSystem::GetOutdoorLightReduction() const
{
    float base = 0.0f;
    switch (m_weatherState.currentType)
    {
    case WeatherType::CLEAR:            base = 0.0f;  break;
    case WeatherType::CLOUDY:           base = 0.1f;  break;
    case WeatherType::OVERCAST:         base = 0.25f; break;
    case WeatherType::LIGHT_RAIN:
    case WeatherType::LIGHT_SNOW:       base = 0.3f;  break;
    case WeatherType::RAIN:
    case WeatherType::SNOW:
    case WeatherType::FOG:              base = 0.4f;  break;
    case WeatherType::HEAVY_RAIN:
    case WeatherType::HEAVY_SNOW:       base = 0.5f;  break;
    case WeatherType::THUNDERSTORM:     base = 0.6f;  break;
    case WeatherType::BLIZZARD:         base = 0.55f; break;
    default: break;
    }
    float cloudCoverage = GetCloudCoverage();
    float cloudReduction = 0.0f;
    if (cloudCoverage > 0.4f)
    {
        cloudReduction = (cloudCoverage - 0.4f) * 0.6f;  // 最多额外减少 36%
    }
    
    return (base * m_weatherState.intensity) + cloudReduction;
}

float WeatherSystem::GetLightningStrength() const
{
    return m_lightningStrength;
}

float WeatherSystem::GetCloudCoverage() const
{
    switch (m_weatherState.currentType)
    {
    case WeatherType::CLEAR:            return 0.15f;
    case WeatherType::CLOUDY:           return 0.4f;
    case WeatherType::OVERCAST:         return 0.85f;
    case WeatherType::FOG:              return 0.7f;
    case WeatherType::LIGHT_RAIN:
    case WeatherType::LIGHT_SNOW:       return 0.65f;
    case WeatherType::RAIN:
    case WeatherType::SNOW:             return 0.75f;
    case WeatherType::HEAVY_RAIN:
    case WeatherType::HEAVY_SNOW:       return 0.9f;
    case WeatherType::THUNDERSTORM:
    case WeatherType::BLIZZARD:         return 0.95f;
    default:                            return 0.4f;
    }
}

float WeatherSystem::GetFogDensity() const
{
    if (m_weatherState.currentType == WeatherType::FOG)
        return 0.6f * m_weatherState.intensity;
    
    // 其他天气也可能有轻微雾气
    if (IsRaining() || IsSnowing())
        return 0.1f * m_weatherState.intensity;
    
    return 0.0f;
}

WeatherType WeatherSystem::DetermineNextWeather() const
{
    WeatherType cur = m_weatherState.currentType;
    float roll = g_theRNG->RollRandomFloatZeroToOne();
    
    switch (cur)
    {
    case WeatherType::CLEAR:
        if (roll < 0.35f) return WeatherType::CLEAR;      // 35% 保持晴天
        if (roll < 0.75f) return WeatherType::CLOUDY;     // 40% → 多云
        // 10% 直接小雨（降低到10%）
        if (roll < 0.85f) return WeatherType::LIGHT_RAIN; // 10% → 小雨
        return WeatherType::FOG;                           // 15% → 雾
        
    case WeatherType::CLOUDY:
        if (roll < 0.12f) return WeatherType::CLEAR;      // 12% → 晴天
        if (roll < 0.28f) return WeatherType::CLOUDY;     // 16% 保持
        if (roll < 0.50f) return WeatherType::OVERCAST;   // 22% → 阴天
        // 35% 直接进入雨雪（降低到35%）
        if (roll < 0.70f) return WeatherType::LIGHT_RAIN; // 20% → 小雨
        if (roll < 0.85f) return WeatherType::RAIN;       // 15% → 雨
        return WeatherType::LIGHT_SNOW;                    // 15% → 小雪
        
    case WeatherType::OVERCAST:
        if (roll < 0.12f) return WeatherType::CLOUDY;     // 12% 返回多云
        // 70% 进入雨雪（略微降低）
        if (roll < 0.50f) return WeatherType::RAIN;       // 38% → 雨
        if (roll < 0.82f) return WeatherType::SNOW;       // 32% → 雪
        return WeatherType::FOG;                           // 18% → 雾
        
    case WeatherType::LIGHT_RAIN:
        if (roll < 0.18f) return WeatherType::CLOUDY;     // 18% → 多云
        if (roll < 0.28f) return WeatherType::LIGHT_RAIN; // 10% 保持
        return WeatherType::RAIN;                          // 72% → 雨（快速升级）
        
    case WeatherType::RAIN:
        if (roll < 0.20f) return WeatherType::LIGHT_RAIN; // 20% → 小雨
        if (roll < 0.50f) return WeatherType::RAIN;       // 30% 保持
        if (roll < 0.78f) return WeatherType::HEAVY_RAIN; // 28% → 大雨
        return WeatherType::THUNDERSTORM;                  // 22% → 雷暴
        
    case WeatherType::HEAVY_RAIN:
        if (roll < 0.25f) return WeatherType::RAIN;       // 25% → 雨
        if (roll < 0.60f) return WeatherType::HEAVY_RAIN; // 35% 保持
        return WeatherType::THUNDERSTORM;                  // 40% → 雷暴
        
    case WeatherType::THUNDERSTORM:
        if (roll < 0.32f) return WeatherType::HEAVY_RAIN; // 32% → 大雨
        if (roll < 0.62f) return WeatherType::THUNDERSTORM; // 30% 保持
        return WeatherType::RAIN;                          // 38% → 雨
        
    case WeatherType::LIGHT_SNOW:
        if (roll < 0.18f) return WeatherType::CLOUDY;     // 18% → 多云
        if (roll < 0.28f) return WeatherType::LIGHT_SNOW; // 10% 保持
        return WeatherType::SNOW;                          // 72% → 雪
        
    case WeatherType::SNOW:
        if (roll < 0.20f) return WeatherType::LIGHT_SNOW; // 20% → 小雪
        if (roll < 0.50f) return WeatherType::SNOW;       // 30% 保持
        if (roll < 0.78f) return WeatherType::HEAVY_SNOW; // 28% → 大雪
        return WeatherType::BLIZZARD;                      // 22% → 暴风雪
        
    case WeatherType::HEAVY_SNOW:
        if (roll < 0.25f) return WeatherType::SNOW;       // 25% → 雪
        if (roll < 0.60f) return WeatherType::HEAVY_SNOW; // 35% 保持
        return WeatherType::BLIZZARD;                      // 40% → 暴风雪
        
    case WeatherType::BLIZZARD:
        if (roll < 0.32f) return WeatherType::HEAVY_SNOW; // 32% → 大雪
        if (roll < 0.62f) return WeatherType::BLIZZARD;   // 30% 保持
        return WeatherType::SNOW;                          // 38% → 雪
        
    case WeatherType::FOG:
        if (roll < 0.35f) return WeatherType::OVERCAST;   // 35% → 阴天
        if (roll < 0.65f) return WeatherType::FOG;        // 30% 保持
        if (roll < 0.85f) return WeatherType::CLEAR;      // 20% → 晴天
        return WeatherType::LIGHT_RAIN;                    // 15% → 小雨
        
    default:
        return WeatherType::CLEAR;
    }
}

float WeatherSystem::CalculateWeatherDuration(WeatherType type) const
{
    float base = 60.0f;
    switch (type)
    {
    case WeatherType::CLEAR:            base = 120.0f; break;  
    case WeatherType::CLOUDY:           base = 90.0f;  break;
    case WeatherType::OVERCAST:         base = 60.0f;  break;
    case WeatherType::THUNDERSTORM:
    case WeatherType::BLIZZARD:         base = 30.0f;  break;
    case WeatherType::HEAVY_RAIN:
    case WeatherType::HEAVY_SNOW:       base = 40.0f;  break;
    default:                            base = 60.0f;  break;
    }
    
    float result = base * g_theRNG->RollRandomFloatInRange(0.7f, 1.3f);
    if (result < 0.0f) result = 0.0f;
    return result;
}

MediumProperties WeatherSystem::GetWeatherMediumProperties() const
{
    MediumProperties props;
    float intensity = m_weatherState.intensity;
    
    switch (m_weatherState.currentType)
    {
    case WeatherType::CLEAR:
        props.tintColor = Rgba8(255, 255, 255, 255);
        props.density = 0.0f;
        props.fogNearDistance = 250.0f;
        props.fogFarDistance = 400.0f;
        break;
        
    case WeatherType::CLOUDY:
        props.tintColor = Rgba8(240, 245, 250, 255);
        props.density = 0.1f * intensity;
        props.fogNearDistance = 200.0f;
        props.fogFarDistance = 350.0f;
        break;
        
    case WeatherType::OVERCAST:
        props.tintColor = Rgba8(200, 210, 220, 255);
        props.density = 0.2f * intensity;
        props.fogNearDistance = 150.0f;
        props.fogFarDistance = 300.0f;
        break;
        
    case WeatherType::LIGHT_RAIN:
        props.tintColor = Rgba8(180, 190, 210, 255);
        props.density = 0.25f * intensity;
        props.fogNearDistance = 100.0f;
        props.fogFarDistance = 250.0f;
        break;
        
    case WeatherType::RAIN:
        props.tintColor = Rgba8(160, 175, 200, 255);
        props.density = 0.35f * intensity;
        props.fogNearDistance = 60.0f;
        props.fogFarDistance = 180.0f;
        break;
        
    case WeatherType::HEAVY_RAIN:
        props.tintColor = Rgba8(140, 160, 190, 255);
        props.density = 0.5f * intensity;
        props.fogNearDistance = 30.0f;
        props.fogFarDistance = 120.0f;
        break;
        
    case WeatherType::THUNDERSTORM:
        props.tintColor = Rgba8(120, 140, 180, 255);
        props.density = 0.55f * intensity;
        props.fogNearDistance = 25.0f;
        props.fogFarDistance = 100.0f;
        break;
        
    case WeatherType::LIGHT_SNOW:
        props.tintColor = Rgba8(230, 235, 255, 255);
        props.density = 0.2f * intensity;
        props.fogNearDistance = 120.0f;
        props.fogFarDistance = 280.0f;
        break;
        
    case WeatherType::SNOW:
        props.tintColor = Rgba8(220, 230, 250, 255);
        props.density = 0.35f * intensity;
        props.fogNearDistance = 80.0f;
        props.fogFarDistance = 200.0f;
        break;
        
    case WeatherType::HEAVY_SNOW:
        props.tintColor = Rgba8(210, 220, 245, 255);
        props.density = 0.5f * intensity;
        props.fogNearDistance = 40.0f;
        props.fogFarDistance = 140.0f;
        break;
        
    case WeatherType::BLIZZARD:
        props.tintColor = Rgba8(200, 210, 240, 255);
        props.density = 0.7f * intensity;
        props.fogNearDistance = 15.0f;
        props.fogFarDistance = 80.0f;
        break;
        
    case WeatherType::FOG:
        props.tintColor = Rgba8(200, 210, 220, 255);
        props.density = 0.6f * intensity;
        props.fogNearDistance = 10.0f;
        props.fogFarDistance = 60.0f;
        break;
        
    default:
        props.tintColor = Rgba8(255, 255, 255, 255);
        props.density = 0.0f;
        props.fogNearDistance = 250.0f;
        props.fogFarDistance = 400.0f;
        break;
    }
    
    return props;
}