#include "GameUIAtlas.h"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

GameUIAtlas::GameUIAtlas(Renderer* renderer, 
                         std::string const& texturePath,
                         IntVec2 const& gridLayout,
                         int gridRegionHeight)
    : m_gridLayout(gridLayout)
    , m_gridRegionHeight(gridRegionHeight)
{
    m_texture = renderer->CreateOrGetTextureFromFile(texturePath.c_str());
    if (!m_texture)
    {
        ERROR_AND_DIE("GameUIAtlas: Failed to load texture: " + texturePath);
    }
    
    // 获取纹理尺寸
    m_textureDimensions = m_texture->GetDimensions();
    
    // 计算网格单元格尺寸
    if (m_gridLayout.x > 0 && m_gridLayout.y > 0)
    {
        // 如果指定了网格区域高度，用它；否则用整张纹理
        int gridHeight = (m_gridRegionHeight > 0) ? m_gridRegionHeight : m_textureDimensions.y;
        
        m_cellWidth = m_textureDimensions.x / m_gridLayout.x;
        m_cellHeight = gridHeight / m_gridLayout.y;
    }
}

GameUIAtlas::~GameUIAtlas()
{
    m_texture = nullptr;
}

AABB2 GameUIAtlas::GetGridSpriteUVs(int col, int row) const
{
    if (col < 0 || col >= m_gridLayout.x || row < 0 || row >= m_gridLayout.y)
    {
        return AABB2::ZERO_TO_ONE;  // 返回默认值
    }
    
    // 像素坐标（纹理左上角为原点）
    int pixelX = col * m_cellWidth;
    int pixelY = row * m_cellHeight;
    
    return PixelToUV(pixelX, pixelY, m_cellWidth, m_cellHeight);
}

AABB2 GameUIAtlas::GetGridSpriteUVs(IntVec2 const& coords) const
{
    return GetGridSpriteUVs(coords.x, coords.y);
}

AABB2 GameUIAtlas::GetGridSpriteUVsByIndex(int index) const
{
    if (m_gridLayout.x <= 0)
    {
        return AABB2::ZERO_TO_ONE;
    }
    
    int col = index % m_gridLayout.x;
    int row = index / m_gridLayout.x;
    
    return GetGridSpriteUVs(col, row);
}

void GameUIAtlas::DefineSprite(std::string const& name, int pixelX, int pixelY, int width, int height)
{
    AABB2 uvs = PixelToUV(pixelX, pixelY, width, height);
    m_namedSprites[name] = uvs;
}

void GameUIAtlas::DefineSpriteUV(std::string const& name, Vec2 const& uvMins, Vec2 const& uvMaxs)
{
    m_namedSprites[name] = AABB2(uvMins, uvMaxs);
}

AABB2 GameUIAtlas::GetSpriteUVs(std::string const& name) const
{
    auto iter = m_namedSprites.find(name);
    if (iter != m_namedSprites.end())
    {
        return iter->second;
    }
    
    // 未找到，返回默认值
    return AABB2::ZERO_TO_ONE;
}

bool GameUIAtlas::HasSprite(std::string const& name) const
{
    return m_namedSprites.find(name) != m_namedSprites.end();
}

AABB2 GameUIAtlas::PixelToUV(int pixelX, int pixelY, int width, int height) const
{
    float texWidth = (float)m_textureDimensions.x;
    float texHeight = (float)m_textureDimensions.y;
    
    // 注意：纹理 UV 的 Y 轴通常是反的（0 在底部，1 在顶部）
    // 但像素坐标 Y=0 在顶部
    
    float uMin = (float)pixelX / texWidth;
    float uMax = (float)(pixelX + width) / texWidth;
    
    // 翻转 Y 轴：像素 Y=0 对应 UV V=1
    float vMax = 1.0f - (float)pixelY / texHeight;
    float vMin = 1.0f - (float)(pixelY + height) / texHeight;
    
    return AABB2(uMin, vMin, uMax, vMax);
}