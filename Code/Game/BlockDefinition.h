#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Gamecommon.hpp"

class BlockDefinition
{
public:
    BlockDefinition() = default; 
    BlockDefinition(XmlElement const& blockDefElement);

public:
    static void InitializeBlockDefs();
    static void ClearDefinitions();

    static BlockDefinition const& GetBlockDef(std::string const& blockDefName);
    static BlockDefinition const& GetBlockDef(uint8_t const& blockUint);

    static std::vector<BlockDefinition> s_blockDefs;
    static BlockDefinition const* s_blockDefsByType[NUM_BLOCK_TYPES];

    std::string m_name = "Air";
    bool m_isVisible = true;
    bool m_isSolid = true;
    bool m_isOpaque = false;
    bool m_isTransparent = false; 

    IntVec2 m_topSpriteCoords;
    IntVec2 m_bottomSpriteCoords;
    IntVec2 m_sideSpriteCoords;

    uint8_t m_indoorLightInfluence = 0;
    uint8_t m_outdoorLightInfluence = 0;

    std::vector<Vertex_PCUTBN> m_verts;
    std::vector<unsigned int> m_indexes;

    bool m_isRedstonePowerable = false;
    bool m_isRedstoneComponent = false;
    bool m_canBeRedstonePowered = false;  // 是否可以被红石充能激活
    uint8_t m_defaultRedstonePower = 0;   // 默认输出功率
    bool m_needsSupport = false;          // 是否需要支撑（红石线、火把等）
    bool m_hasCustomCollision = false;    // 是否有自定义碰撞箱
};