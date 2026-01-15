#include "CloudSystem.h"
#include "WeatherSystem.h"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/World.h"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <math.h>

extern Renderer* g_theRenderer;
extern Game* g_theGame;

CloudSystem::CloudSystem(World* world)
    : m_world(world)
    , m_cloudSeed(GAME_SEED + 7777)
{
}

CloudSystem::~CloudSystem()
{
    if (m_vertexBuffer) { delete m_vertexBuffer; m_vertexBuffer = nullptr; }
    if (m_indexBuffer)  { delete m_indexBuffer;  m_indexBuffer = nullptr; }
}

Vec3 CloudSystem::GetCameraPosition() const
{
    if (m_world->m_owner->m_player->m_gameCamera)
    {
        return m_world->m_owner->m_player->m_gameCamera->GetPosition();
    }
    return m_world->m_owner->m_player->GetEyePosition();
}

void CloudSystem::Update(float deltaSeconds)
{
    Vec2 windDir = m_windDirection;
    float speed = m_cloudSpeed;
    float targetCoverage = m_cloudCoverage;
    
    if (m_world->m_weatherSystem)
    {
        WeatherState state = m_world->m_weatherSystem->GetWeatherState();
        windDir = state.windDirection;
        speed = m_cloudSpeed + state.windStrength * 5.0f;
        targetCoverage = m_world->m_weatherSystem->GetCloudCoverage();
        targetCoverage = GetClamped(targetCoverage, 0.2f, 0.9f);
        
        float coverageDiff = targetCoverage - m_cloudCoverage;
        if (fabsf(coverageDiff) > 0.01f)
        {
            m_cloudCoverage += coverageDiff * deltaSeconds * 0.5f;
        }
    }
    
    m_windDirection = windDir;
    m_cloudOffset += windDir * speed * deltaSeconds;
    
    float wrapDistance = m_cloudBlockSize * 1000.0f;
    if (fabsf(m_cloudOffset.x) > wrapDistance)
        m_cloudOffset.x = fmodf(m_cloudOffset.x, wrapDistance);
    if (fabsf(m_cloudOffset.y) > wrapDistance)
        m_cloudOffset.y = fmodf(m_cloudOffset.y, wrapDistance);
    
    Vec3 cameraPos = GetCameraPosition();
    float cameraMoved = (cameraPos - m_lastCameraPos).GetLength();
    Vec2 offsetDelta = m_cloudOffset - m_offsetAtLastRebuild;
    float offsetMoved = offsetDelta.GetLength();
    float coverageChanged = fabsf(m_cloudCoverage - m_coverageAtLastRebuild);
    
    if (cameraMoved > m_cloudBlockSize * 3.0f || 
        offsetMoved > m_cloudBlockSize * 0.5f ||
        coverageChanged > 0.05f ||
        m_needsRebuild)
    {
        m_lastCameraPos = cameraPos;
        m_offsetAtLastRebuild = m_cloudOffset;
        m_coverageAtLastRebuild = m_cloudCoverage;
        m_needsRebuild = false;
        RebuildCloudMesh();
    }
}

bool CloudSystem::ShouldHaveCloud(float noiseX, float noiseY) const
{
    float noise = Compute2dPerlinNoise(
         noiseX, noiseY,
         200.0f, 8, 0.5f, 2.0f, true, m_cloudSeed
     );
    float adjustedCoverage = powf(m_cloudCoverage, 0.7f);
    float threshold = 1.0f - (adjustedCoverage * 2.0f);
    return noise > threshold;
}

bool CloudSystem::HasCloudNeighbor(float noiseX, float noiseY, int dx, int dy) const
{
    float neighborX = noiseX + dx * m_cloudBlockSize;
    float neighborY = noiseY + dy * m_cloudBlockSize;
    return ShouldHaveCloud(neighborX, neighborY);
}

void CloudSystem::RebuildCloudMesh()
{
    m_vertices.clear();
    m_indices.clear();
    
    AABB2 cloudUV = g_theGame->m_spriteSheet->GetSpriteUVs(IntVec2(2,0));
    Vec3 cameraPos = m_lastCameraPos;
    int blockRadius = (int)(m_renderRadius / m_cloudBlockSize);
    
    int centerBlockX = (int)floorf(cameraPos.x / m_cloudBlockSize);
    int centerBlockY = (int)floorf(cameraPos.y / m_cloudBlockSize);
    
    int cloudsGenerated = 0;
    int positionsChecked = 0;
    
    for (int by = -blockRadius; by <= blockRadius; by++)
    {
        for (int bx = -blockRadius; bx <= blockRadius; bx++)
        {
            float dist = sqrtf((float)(bx * bx + by * by)) * m_cloudBlockSize;
            if (dist > m_renderRadius)
                continue;
            
            positionsChecked++;
            
            float worldX = (centerBlockX + bx) * m_cloudBlockSize;
            float worldY = (centerBlockY + by) * m_cloudBlockSize;
            
            float noiseX = worldX + m_cloudOffset.x;
            float noiseY = worldY + m_cloudOffset.y;
            
            if (ShouldHaveCloud(noiseX, noiseY))
            {
                AddCloudBlock(worldX, worldY, m_cloudHeight, cloudUV, noiseX, noiseY);
                cloudsGenerated++;
            }
        }
    }
    
    // static int rebuildCount = 0;
    // rebuildCount++;
    // DebuggerPrintf("[CloudSystem] Rebuild #%d: %d/%d clouds (%.1f%%), coverage=%.2f\n",
    //                rebuildCount, cloudsGenerated, positionsChecked,
    //                (float)cloudsGenerated / positionsChecked * 100.0f,
    //                m_cloudCoverage);
    
    if (m_vertexBuffer) { delete m_vertexBuffer; m_vertexBuffer = nullptr; }
    if (m_indexBuffer)  { delete m_indexBuffer;  m_indexBuffer = nullptr; }
    
    if (!m_vertices.empty())
    {
        m_vertexBuffer = g_theRenderer->CreateVertexBuffer(
            (unsigned int)(m_vertices.size() * sizeof(Vertex_PCUTBN)), sizeof(Vertex_PCUTBN));
        m_indexBuffer = g_theRenderer->CreateIndexBuffer(
            (unsigned int)(m_indices.size() * sizeof(unsigned int)), sizeof(unsigned int));
        
        g_theRenderer->CopyCPUToGPU(m_vertices.data(),
            (unsigned int)(m_vertices.size() * sizeof(Vertex_PCUTBN)), m_vertexBuffer);
        g_theRenderer->CopyCPUToGPU(m_indices.data(),
            (unsigned int)(m_indices.size() * sizeof(unsigned int)), m_indexBuffer);
    }
}

void CloudSystem::AddCloudBlock(float worldX, float worldY, float worldZ, const AABB2& uvs, float noiseX, float noiseY)
{
    float size = m_cloudBlockSize;
    float height = m_cloudThickness;
    
    Vec3 nnn(worldX,        worldY,        worldZ);
    Vec3 xnn(worldX + size, worldY,        worldZ);
    Vec3 nxn(worldX,        worldY + size, worldZ);
    Vec3 xxn(worldX + size, worldY + size, worldZ);
    Vec3 nnx(worldX,        worldY,        worldZ + height);
    Vec3 xnx(worldX + size, worldY,        worldZ + height);
    Vec3 nxx(worldX,        worldY + size, worldZ + height);
    Vec3 xxx(worldX + size, worldY + size, worldZ + height);
    
    bool hasEast  = HasCloudNeighbor(noiseX, noiseY, 1, 0);
    bool hasWest  = HasCloudNeighbor(noiseX, noiseY, -1, 0);
    bool hasNorth = HasCloudNeighbor(noiseX, noiseY, 0, 1);
    bool hasSouth = HasCloudNeighbor(noiseX, noiseY, 0, -1);
    
    Rgba8 topColor(255, 0, 255, m_cloudColor.a);
    Rgba8 bottomColor(255, 0, 178, m_cloudColor.a);
    Rgba8 eastWestColor(255, 0, 217, m_cloudColor.a);
    Rgba8 northSouthColor(255, 0, 204, m_cloudColor.a);
    
    AddVertsForIndexQuad3D(m_vertices, m_indices, nnx, xnx, xxx, nxx, topColor, uvs);
    AddVertsForIndexQuad3D(m_vertices, m_indices, nnn, nxn, xxn, xnn, bottomColor, uvs);
    
    if (!hasSouth)
        AddVertsForIndexQuad3D(m_vertices, m_indices, nnn, xnn, xnx, nnx, northSouthColor, uvs);
    if (!hasNorth)
        AddVertsForIndexQuad3D(m_vertices, m_indices, xxn, nxn, nxx, xxx, northSouthColor, uvs);
    if (!hasEast)
        AddVertsForIndexQuad3D(m_vertices, m_indices, xnn, xxn, xxx, xnx, eastWestColor, uvs);
    if (!hasWest)
        AddVertsForIndexQuad3D(m_vertices, m_indices, nxn, nnn, nnx, nxx, eastWestColor, uvs);
}

void CloudSystem::Render() const
{
    if (!m_vertexBuffer || !m_indexBuffer || m_vertices.empty())
        return;
    
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    
    g_theRenderer->DrawIndexBuffer(m_vertexBuffer, m_indexBuffer, (unsigned int)m_indices.size());
}

void CloudSystem::SetCloudCoverage(float coverage)
{
    float oldCoverage = m_cloudCoverage;
    m_cloudCoverage = GetClamped(coverage, 0.0f, 0.8f);
    
    if (fabsf(oldCoverage - m_cloudCoverage) > 0.05f)
    {
        m_needsRebuild = true;
    }
}

bool CloudSystem::IsUnderCloud(float worldX, float worldY) const
{
    float noiseX = worldX + m_cloudOffset.x;
    float noiseY = worldY + m_cloudOffset.y;
    return ShouldHaveCloud(noiseX, noiseY);
}