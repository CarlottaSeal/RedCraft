#include "WeatherParticleRenderer.h"
#include "WeatherSystem.h"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Game.hpp"
#include "Game/Gamecommon.hpp"

extern RandomNumberGenerator* g_theRNG;
extern Game* g_theGame;

WeatherParticleRenderer::WeatherParticleRenderer(WeatherSystem* weatherSys)
{
    m_weatherSystem = weatherSys;
    Startup();
}

WeatherParticleRenderer::~WeatherParticleRenderer()
{
    Shutdown();
}

void WeatherParticleRenderer::Startup()
{
    m_rainParticles.resize(MAX_RAIN_PARTICLES);
    m_rainVertices.reserve(MAX_RAIN_PARTICLES * 6);
    
    // 初始化所有粒子位置
    for (int i = 0; i < MAX_RAIN_PARTICLES; i++)
    {
        WeatherParticle& p = m_rainParticles[i];
        p.position = Vec3(
            g_theRNG->RollRandomFloatInRange(-PARTICLE_SPAWN_RADIUS, PARTICLE_SPAWN_RADIUS),
            g_theRNG->RollRandomFloatInRange(-PARTICLE_SPAWN_RADIUS, PARTICLE_SPAWN_RADIUS),
            g_theRNG->RollRandomFloatInRange(0.0f, 30.0f)
        );
        p.size = g_theRNG->RollRandomFloatInRange(0.03f, 0.06f);  // ← 更细
        p.alpha = g_theRNG->RollRandomFloatInRange(0.3f, 0.6f);   // ← 更透明
        p.velocity = Vec3(0, 0, -15.0f);
    }
    
    // 初始化雪粒子
    m_snowParticles.resize(MAX_SNOW_PARTICLES);
    m_snowVertices.reserve(MAX_SNOW_PARTICLES * 6);
    
    for (int i = 0; i < MAX_SNOW_PARTICLES; i++)
    {
        WeatherParticle& p = m_snowParticles[i];
        p.position = Vec3(
            g_theRNG->RollRandomFloatInRange(-PARTICLE_SPAWN_RADIUS, PARTICLE_SPAWN_RADIUS),
            g_theRNG->RollRandomFloatInRange(-PARTICLE_SPAWN_RADIUS, PARTICLE_SPAWN_RADIUS),
            g_theRNG->RollRandomFloatInRange(0.0f, 30.0f)
        );
        p.size = g_theRNG->RollRandomFloatInRange(0.1f, 0.3f);
        p.alpha = g_theRNG->RollRandomFloatInRange(0.6f, 1.0f);
        p.velocity = Vec3(0, 0, -2.0f);
    }
}

void WeatherParticleRenderer::Shutdown()
{
}

void WeatherParticleRenderer::Update(float deltaSeconds, const Camera& camera)
{
    if (!m_weatherSystem)
        return;
    
    Vec3 cameraPos = camera.GetPosition();
    WeatherState state = m_weatherSystem->GetWeatherState();
    Vec2 windDir = state.windDirection;
    float windStrength = state.windStrength;
    
    if (m_weatherSystem->IsRaining())
    {
        UpdateRainParticles(deltaSeconds, cameraPos, windDir, windStrength);
        RebuildRainMesh(camera); 
    }
    else if (m_weatherSystem->IsSnowing())
    {
        UpdateSnowParticles(deltaSeconds, cameraPos, windDir, windStrength);
        RebuildSnowMesh(camera);
    }
    else
    {
        m_rainVertices.clear();
        m_snowVertices.clear();
    }
}

void WeatherParticleRenderer::UpdateRainParticles(float deltaSeconds, const Vec3& cameraPos, 
                                                   const Vec2& windDir, float windStrength)
{
    for (WeatherParticle& particle : m_rainParticles)
    {
        particle.velocity.x = windDir.x * windStrength * 3.0f;
        particle.velocity.y = windDir.y * windStrength * 3.0f;
        particle.velocity.z = -15.0f;
        
        particle.position += particle.velocity * deltaSeconds;
        
        float distFromCamera = (particle.position - cameraPos).GetLength();
        if (distFromCamera > PARTICLE_DESPAWN_RADIUS || 
            particle.position.z < cameraPos.z - 20.0f)
        {
            ResetParticle(particle, cameraPos, false);
        }
    }
}

void WeatherParticleRenderer::UpdateSnowParticles(float deltaSeconds, const Vec3& cameraPos,
                                                   const Vec2& windDir, float windStrength)
{
    for (WeatherParticle& particle : m_snowParticles)
    {
        particle.velocity.x = windDir.x * windStrength * 5.0f;
        particle.velocity.y = windDir.y * windStrength * 5.0f;
        particle.velocity.z = -2.0f;
        
        float time = (float)g_theGame->m_gameClock->GetTotalSeconds();
        float swayX = sinf(time * 2.0f + particle.position.y * 0.1f) * 0.5f;
        float swayY = cosf(time * 1.5f + particle.position.x * 0.1f) * 0.5f;
        
        particle.position.x += (particle.velocity.x + swayX) * deltaSeconds;
        particle.position.y += (particle.velocity.y + swayY) * deltaSeconds;
        particle.position.z += particle.velocity.z * deltaSeconds;
        
        float distFromCamera = (particle.position - cameraPos).GetLength();
        if (distFromCamera > PARTICLE_DESPAWN_RADIUS || 
            particle.position.z < cameraPos.z - 20.0f)
        {
            ResetParticle(particle, cameraPos, true);
        }
    }
}

void WeatherParticleRenderer::ResetParticle(WeatherParticle& particle, const Vec3& cameraPos, bool isSnow)
{
    float angle = g_theRNG->RollRandomFloatInRange(0.0f, 360.0f);
    float radius = g_theRNG->RollRandomFloatInRange(0.0f, PARTICLE_SPAWN_RADIUS);
    
    particle.position.x = cameraPos.x + radius * CosDegrees(angle);
    particle.position.y = cameraPos.y + radius * SinDegrees(angle);
    particle.position.z = cameraPos.z + g_theRNG->RollRandomFloatInRange(10.0f, 30.0f);
    
    if (isSnow)
    {
        particle.size = g_theRNG->RollRandomFloatInRange(0.1f, 0.3f);
        particle.alpha = g_theRNG->RollRandomFloatInRange(0.6f, 1.0f);
    }
    else
    {
        particle.size = g_theRNG->RollRandomFloatInRange(0.03f, 0.06f);
        particle.alpha = g_theRNG->RollRandomFloatInRange(0.3f, 0.6f);
    }
}

void WeatherParticleRenderer::RebuildRainMesh(const Camera& camera)
{
    m_rainVertices.clear();
    
    Vec3 camForward, camLeft, camUp;
    camera.GetOrientation().GetAsVectors_IFwd_JLeft_KUp(camForward, camLeft, camUp);
    Vec3 camRight = -camLeft;
    Vec3 camPos = camera.GetPosition();
    
    for (const WeatherParticle& particle : m_rainParticles)
    {
        Vec3 top = particle.position;
        Vec3 bottom = particle.position + Vec3(0, 0, -0.5f);
        
        float distToCamera = (particle.position - camPos).GetLength();
        float sizeScale = 1.0f + (distToCamera * 0.02f);  // 2% per meter
        float halfWidth = (particle.size * 0.5f) / sizeScale;
        
        // R = 255 (full outdoor light)
        // G = 0 (no indoor light)
        // B = 200 (0.78 shade, 稍微变暗)
        // A = alpha
        Rgba8 color(255, 0, 200, (unsigned char)(particle.alpha * 255));
        
        Vec3 bl = bottom - camRight * halfWidth;
        Vec3 br = bottom + camRight * halfWidth;
        Vec3 tr = top + camRight * halfWidth;
        Vec3 tl = top - camRight * halfWidth;
        
        Vec3 normal = (camPos - particle.position).GetNormalized();
        
        AddVertsForQuad3D(m_rainVertices, bl, br, tr, tl, color, AABB2::ZERO_TO_ONE, normal);
    }
}

void WeatherParticleRenderer::RebuildSnowMesh(const Camera& camera)
{
    m_snowVertices.clear();
    
    Vec3 camForward, camLeft, camUp;
    camera.GetOrientation().GetAsVectors_IFwd_JLeft_KUp(camForward, camLeft, camUp);
    Vec3 camRight = -camLeft;
    Vec3 camPos = camera.GetPosition();
    
    for (const WeatherParticle& particle : m_snowParticles)
    {
        float halfSize = particle.size * 0.5f;
        
        Rgba8 color(255, 0, 255, (unsigned char)(particle.alpha * 255));
        
        Vec3 center = particle.position;
        
        Vec3 bl = center - camRight * halfSize - camUp * halfSize;
        Vec3 br = center + camRight * halfSize - camUp * halfSize;
        Vec3 tr = center + camRight * halfSize + camUp * halfSize;
        Vec3 tl = center - camRight * halfSize + camUp * halfSize;
        
        Vec3 normal = (camPos - particle.position).GetNormalized();
        
        AddVertsForQuad3D(m_snowVertices, bl, br, tr, tl, color, AABB2::ZERO_TO_ONE, normal);
    }
}

void WeatherParticleRenderer::Render() const
{
    if (!m_rainVertices.empty())
    {
        g_theRenderer->SetBlendMode(BlendMode::ALPHA);
        g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
        g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
        g_theRenderer->BindTexture(nullptr);
        g_theRenderer->DrawVertexArray(m_rainVertices);
    }
    
    if (!m_snowVertices.empty())
    {
        g_theRenderer->SetBlendMode(BlendMode::ALPHA);
        g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
        g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
        g_theRenderer->BindTexture(nullptr);
        g_theRenderer->DrawVertexArray(m_snowVertices);
    }
}