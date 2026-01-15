#pragma once
#include <cstdint>

#include "Engine/Core/Rgba8.hpp"

enum class MediumType : uint8_t
{
    AIR = 0,
    FOG,
    WATER,
    LAVA,
    NUM_MEDIUM_TYPES
};

struct MediumProperties
{
    Rgba8   tintColor           = Rgba8::WHITE;
    float   density             = 0.0f;
    float   fogNearDistance     = 100.0f;
    float   fogFarDistance      = 300.0f;
    float   absorptionRate      = 0.0f;
    float   scatterStrength     = 0.0f;
    bool    enableDistortion    = false;
    float   distortionStrength  = 0.0f;
    float   distortionSpeed     = 1.0f;
    
    static MediumProperties Lerp(const MediumProperties& a, const MediumProperties& b, float t)
    {
        MediumProperties result;
        result.tintColor.r = (unsigned char)((1.0f - t) * a.tintColor.r + t * b.tintColor.r);
        result.tintColor.g = (unsigned char)((1.0f - t) * a.tintColor.g + t * b.tintColor.g);
        result.tintColor.b = (unsigned char)((1.0f - t) * a.tintColor.b + t * b.tintColor.b);
        result.tintColor.a = (unsigned char)((1.0f - t) * a.tintColor.a + t * b.tintColor.a);
        result.density              = (1.0f - t) * a.density + t * b.density;
        result.fogNearDistance      = (1.0f - t) * a.fogNearDistance + t * b.fogNearDistance;
        result.fogFarDistance       = (1.0f - t) * a.fogFarDistance + t * b.fogFarDistance;
        result.absorptionRate       = (1.0f - t) * a.absorptionRate + t * b.absorptionRate;
        result.scatterStrength      = (1.0f - t) * a.scatterStrength + t * b.scatterStrength;
        result.distortionStrength   = (1.0f - t) * a.distortionStrength + t * b.distortionStrength;
        result.distortionSpeed      = (1.0f - t) * a.distortionSpeed + t * b.distortionSpeed;
        result.enableDistortion     = a.enableDistortion || b.enableDistortion;
        return result;
    }
};