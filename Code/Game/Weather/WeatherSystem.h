#pragma once
#include "Engine/Math/Vec2.hpp"
#include "MediumTypes.h"

class World;

enum class WeatherType : uint8_t
{
    CLEAR = 0,
    CLOUDY,
    OVERCAST,
    LIGHT_RAIN,
    RAIN,
    HEAVY_RAIN,
    THUNDERSTORM,
    LIGHT_SNOW,
    SNOW,
    HEAVY_SNOW,
    BLIZZARD,
    FOG,
    NUM_WEATHER_TYPES
};

struct WeatherState
{
    WeatherType currentType         = WeatherType::CLEAR;
    WeatherType m_targetType          = WeatherType::CLEAR;
    float       intensity           = 0.0f;
    float       m_targetIntensity     = 0.0f;
    float       m_transitionTimer     = 0.0f;
    float       m_transitionDuration  = 10.0f;
    float       m_weatherDuration     = 0.0f;
    float       windStrength        = 0.0f;
    Vec2        windDirection       = Vec2(1.0f, 0.0f);
    
    bool IsTransitioning() const 
    { 
        return m_transitionTimer < m_transitionDuration; 
    }
    
    float GetTransitionProgress() const 
    { 
        return (m_transitionDuration > 0.0f) ? (m_transitionTimer / m_transitionDuration) : 1.0f; 
    }
};

class WeatherSystem
{
public:
    WeatherSystem(World* world);
    ~WeatherSystem();
    
    void Update(float deltaSeconds);
    
    WeatherState    GetWeatherState() const { return m_weatherState; }
    WeatherType     GetCurrentWeather() const { return m_weatherState.currentType; }
    float           GetWeatherIntensity() const { return m_weatherState.intensity; }
    float           GetWindStrength() const { return m_weatherState.windStrength; }
    Vec2            GetWindDirection() const { return m_weatherState.windDirection; }
    
    void SetWeather(WeatherType type, float transitionTime = 10.0f);
    void SetWeatherIntensity(float intensity);
    void SetAutoWeather(bool enabled) { m_autoWeatherEnabled = enabled; }
    
    bool IsRaining() const;
    bool IsSnowing() const;
    bool IsStormy() const;
    bool IsFoggy() const;
    bool IsClear() const;
    
    float GetSkyDarkening() const;           
    float GetOutdoorLightReduction() const;  
    float GetLightningStrength() const;      
    float GetCloudCoverage() const;          
    float GetFogDensity() const;             
    
    MediumProperties GetWeatherMediumProperties() const;
    
private:
    void UpdateWeatherTransition(float deltaSeconds);
    void UpdateAutomaticWeatherChange(float deltaSeconds);
    void UpdateWind(float deltaSeconds);
    void UpdateLightning(float deltaSeconds);
    
    WeatherType DetermineNextWeather() const;
    float CalculateWeatherDuration(WeatherType type) const;
    float GetBaseIntensityForWeather(WeatherType type) const;
    
    World*          m_world = nullptr;
    WeatherState    m_weatherState;
    
    float           m_lightningTimer = 0.0f;
    float           m_lightningStrength = 0.0f;
    float           m_nextLightningTime = 0.0f;
    
    bool            m_autoWeatherEnabled = true;
    float           m_noiseTime = 0.0f;
};