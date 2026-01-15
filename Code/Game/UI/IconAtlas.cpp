#include "IconAtlas.h"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Game/BlockDefinition.h"

IconAtlas::IconAtlas(Renderer* renderer, 
                     std::string const& texturePath,
                     IntVec2 const& gridLayout,
                     int gridRegionHeight)
    : UISpriteAtlas(*renderer->CreateOrGetTextureFromFile(texturePath.c_str()), gridLayout)
    , m_gridRegionHeight(gridRegionHeight)
{
    m_texture = renderer->CreateOrGetTextureFromFile(texturePath.c_str());
    if (!m_texture)
    {
        ERROR_AND_DIE("GameUIAtlas: Failed to load texture: " + texturePath);
    }
    
    m_textureDimensions = m_texture->GetDimensions();
    
    // if (m_gridLayout.x > 0 && m_gridLayout.y > 0) //what is this doin
    // {
    //     // 如果指定了网格区域高度，用它；否则用整张纹理
    //     int gridHeight = (m_gridRegionHeight > 0) ? m_gridRegionHeight : m_textureDimensions.y;
    //     
    //     m_cellWidth = m_textureDimensions.x / m_gridLayout.x;
    //     m_cellHeight = gridHeight / m_gridLayout.y;
    // }

    InitializeHardCode();   
}

IconAtlas::~IconAtlas()
{
    m_texture = nullptr;
}

void IconAtlas::InitializeHardCode()
{
    DefineSpriteUV("HotbarPanel", Vec2(), Vec2(182.f/256.f, 22.f/256.f));
    DefineSpriteUV("SelectionFrame", Vec2(182.f/256.f,0.f),
        Vec2((182.f+24.f)/256.f, 25.f/256.f));
}

AABB2 IconAtlas::GetGridSpriteUVs(int col, int row) const
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

AABB2 IconAtlas::GetGridSpriteUVs(IntVec2 const& coords) const
{
    //return GetGridSpriteUVs(coords.x, coords.y);

    return GetSpriteUVs(coords);
}

AABB2 IconAtlas::GetGridSpriteUVsByIndex(int index) const
{
    if (m_gridLayout.x <= 0)
    {
        return AABB2::ZERO_TO_ONE;
    }
    
    int col = index % m_gridLayout.x;
    int row = index / m_gridLayout.x;
    
    return GetGridSpriteUVs(col, row);
}

AABB2 IconAtlas::GetItemIconUVs(std::string const& itemName) const
{
    if (itemName.empty())
        return GetGridSpriteUVsByIndex(0);
    
    // 直接查找，O(1)
    IntVec2 iconCoords = BlockDefinition::GetItemIconCoords(itemName);
    return GetGridSpriteUVs(iconCoords);
}

AABB2 IconAtlas::GetBlockIconUVs(std::string const& blockName) const
{
    BlockDefinition def = BlockDefinition::GetBlockDef(blockName);
    if ( !def.m_droppedItemName.empty())
    {
        return GetItemIconUVs(def.m_droppedItemName);
    }
    return GetGridSpriteUVsByIndex(0);
}

void IconAtlas::DefineSprite(std::string const& name, int pixelX, int pixelY, int width, int height)
{
    AABB2 uvs = PixelToUV(pixelX, pixelY, width, height);
    m_namedSprites[name] = uvs;
}

void IconAtlas::DefineSpriteUV(std::string const& name, Vec2 const& uvMins, Vec2 const& uvMaxs)
{
    m_namedSprites[name] = AABB2(uvMins, uvMaxs);
}

AABB2 IconAtlas::GetSpriteUVsByName(std::string const& name) const
{
    auto iter = m_namedSprites.find(name);
    if (iter != m_namedSprites.end())
    {
        return iter->second;
    }
    
    // 未找到，返回默认值
    return AABB2::ZERO_TO_ONE;
}

bool IconAtlas::HasSprite(std::string const& name) const
{
    return m_namedSprites.find(name) != m_namedSprites.end();
}

AABB2 IconAtlas::GetBlockIconUVs(uint8_t blockType) const
{
    BlockDefinition def = BlockDefinition::GetBlockDef(blockType);
    
    // 使用方块的 itemIconCoords
    return GetGridSpriteUVs(def.m_itemIconCoords);
}

AABB2 IconAtlas::GetItemIconUVs(ItemType itemType, uint8_t blockType) const
{
    if (itemType != ItemType::NONE && itemType != ItemType::BONEMEAL)
    {
        // 大多数物品对应方块，使用方块的图标
        if (blockType != 0)
        {
            return GetBlockIconUVs(blockType);
        }
    }
    return AABB2::ZERO_TO_ONE;
}

AABB2 IconAtlas::PixelToUV(int pixelX, int pixelY, int width, int height) const
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
