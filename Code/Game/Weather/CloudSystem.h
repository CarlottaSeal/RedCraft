#pragma once
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include <vector>

class World;
class VertexBuffer;
class IndexBuffer;

class CloudSystem
{
public:
    CloudSystem(World* world);
    ~CloudSystem();
    
    void Update(float deltaSeconds);
    void Render() const;
    
    void SetCloudHeight(float height) { m_cloudHeight = height; }
    void SetCloudColor(const Rgba8& color) { m_cloudColor = color; }
    void SetCloudSpeed(float speed) { m_cloudSpeed = speed; }
    void SetCloudCoverage(float coverage);
    void SetCloudBlockSize(float size) { m_cloudBlockSize = size; m_needsRebuild = true; }
    void SetRenderRadius(float radius) { m_renderRadius = radius; m_needsRebuild = true; }
    
    float GetCloudHeight() const { return m_cloudHeight; }
    float GetCloudCoverage() const { return m_cloudCoverage; }
    float GetCloudShadowStrength() const { return m_cloudCoverage * 0.25f; }
    
    bool IsUnderCloud(float worldX, float worldY) const;
    
private:
    void RebuildCloudMesh();
    void AddCloudBlock(float worldX, float worldY, float worldZ, const AABB2& uvs, float noiseX, float noiseY);
    
    bool ShouldHaveCloud(float noiseX, float noiseY) const;
    
    bool HasCloudNeighbor(float noiseX, float noiseY, int dx, int dy) const;
    
    Vec3 GetCameraPosition() const;
    
    World* m_world = nullptr;
    
    float   m_cloudHeight       = 180.0f;      
    float   m_cloudThickness    = 0.8f;        
    float   m_cloudBlockSize    = 12.0f;       
    float   m_cloudCoverage     = 0.45f;       
    float   m_cloudSpeed        = 3.0f;        
    Rgba8   m_cloudColor        = Rgba8(255, 255, 255, 220);
    
    Vec2    m_cloudOffset       = Vec2(0.0f, 0.0f);
    Vec2    m_windDirection     = Vec2(1.0f, 0.0f);
    Vec2    m_offsetAtLastRebuild = Vec2(0.0f, 0.0f);
    float   m_coverageAtLastRebuild = 0.45f;
    
    float   m_renderRadius      = 400.0f;
    Vec3    m_lastCameraPos     = Vec3();
    bool    m_needsRebuild      = true;
    
    std::vector<Vertex_PCUTBN>  m_vertices;
    std::vector<unsigned int>   m_indices;
    VertexBuffer*               m_vertexBuffer = nullptr;
    IndexBuffer*                m_indexBuffer = nullptr;
    
    unsigned int m_cloudSeed = 54321;
};