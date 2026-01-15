#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"  
#include <vector>

class Renderer;
class Camera;
class WeatherSystem;
class VertexBuffer;

struct WeatherParticle
{
    Vec3 position;
    Vec3 velocity;
    float size;
    float alpha;
};

class WeatherParticleRenderer
{
public:
    WeatherParticleRenderer(WeatherSystem* weatherSys);
    ~WeatherParticleRenderer();
    
    void Startup();
    void Shutdown();
    
    void Update(float deltaSeconds, const Camera& camera);
    void Render() const;
    
private:
    void UpdateRainParticles(float deltaSeconds, const Vec3& cameraPos, const Vec2& windDir, float windStrength);
    void UpdateSnowParticles(float deltaSeconds, const Vec3& cameraPos, const Vec2& windDir, float windStrength);
    void RebuildRainMesh(const Camera& camera);   
    void RebuildSnowMesh(const Camera& camera);   
    
    void ResetParticle(WeatherParticle& particle, const Vec3& cameraPos, bool isSnow);
    
private:
    WeatherSystem* m_weatherSystem;
    std::vector<WeatherParticle> m_rainParticles;
    std::vector<Vertex_PCUTBN> m_rainVertices;  
    
    std::vector<WeatherParticle> m_snowParticles;
    std::vector<Vertex_PCUTBN> m_snowVertices;  
    
    const int MAX_RAIN_PARTICLES = 2000;
    const int MAX_SNOW_PARTICLES = 1000;
    const float PARTICLE_SPAWN_RADIUS = 50.0f;
    const float PARTICLE_DESPAWN_RADIUS = 60.0f;
};