#pragma once
#include <cstdint>

#include "BlockDefinition.h"
#include "Gamecommon.hpp"

class Chunk;

class Block
{
public:
    uint8_t m_typeIndex = 0;
    uint8_t m_lightData = 0;  // 高4位=室外光，低4位=室内光
    
    uint8_t m_flags = 0;

    uint8_t m_redstoneData = 0;

    inline void SetType(uint8_t typeIndex)
    {
        m_typeIndex = typeIndex;
        
        const BlockDefinition& def = BlockDefinition::GetBlockDef(typeIndex);
        
        m_flags &= (BLOCK_BIT_IS_SKY | BLOCK_BIT_IS_LIGHT_DIRTY);
        
        // 设置新的flags
        if (def.m_isOpaque)
            m_flags |= BLOCK_BIT_IS_OPAQUE;
        if (def.m_isSolid)
            m_flags |= BLOCK_BIT_IS_SOLID;
        if (def.m_isVisible)
            m_flags |= BLOCK_BIT_IS_VISIBLE;
        
        // 重置红石数据（保留朝向如果是有方向的方块）
        if (!IsRedstoneComponent(typeIndex))
        {
            m_redstoneData = 0;
        }
    }
    
    //uint8_t GetOutdoorLight() const;
    void SetOutdoorLight(uint8_t level);
    //uint8_t GetIndoorLight() const;
    void SetIndoorLight(uint8_t level);

    inline bool IsOpaque() const
    {
        return (m_flags & BLOCK_BIT_IS_OPAQUE) != 0;
    }
    inline uint8_t GetOutdoorLight() const
    {
        return (m_lightData >> 4) & 0x0F;
    }
    inline uint8_t GetIndoorLight() const
    {
        return m_lightData & 0x0F;
    }
    
    bool IsSky() const;
    void SetIsSky(bool isSky);
    bool IsLightDirty() const;
    void SetLightDirty(bool isDirty);
    //bool IsOpaque() const;
    void SetIsOpaque(bool isOpaque);
    bool IsSolid() const;
    void SetIsSolid(bool isSolid);
    bool IsVisible() const;
    void SetIsVisible(bool isVisible);

    inline uint8_t GetRedstonePower() const { return m_redstoneData & 0x0F; }
    inline void SetRedstonePower(uint8_t power) 
    { 
        power = (power > 15) ? 15 : power;
        m_redstoneData = (m_redstoneData & 0xF0) | power; 
    }
    
    // 红石脏标记
    inline bool IsRedstoneDirty() const { return (m_flags & BLOCK_BIT_IS_REDSTONE_DIRTY) != 0; }
    inline void SetRedstoneDirty(bool isDirty)
    {
        if (isDirty) m_flags |= BLOCK_BIT_IS_REDSTONE_DIRTY;
        else m_flags &= ~BLOCK_BIT_IS_REDSTONE_DIRTY;
    }
    
    // 方块朝向 (0-5)，存储在 m_redstoneData 高4位的低3位
    // 用于中继器、活塞、侦测器等有方向的方块
    inline uint8_t GetBlockFacing() const { return (m_redstoneData >> 4) & 0x07; }
    inline void SetBlockFacing(uint8_t facing)
    {
        facing = (facing > 5) ? 0 : facing;
        m_redstoneData = (m_redstoneData & 0x8F) | ((facing & 0x07) << 4);
    }
    
    // 特殊状态位（最高位），用于：拉杆开/关、活塞伸出/收回
    inline bool GetSpecialState() const { return (m_redstoneData & 0x80) != 0; }

    uint8_t GetRepeaterDelay();
    void SetRepeaterDelay(uint8_t delay);
    bool IsRepeaterLocked();
    void SetRepeaterLocked(bool locked);
    
    void SetSpecialState(bool state);
    Direction GetPistonFacing();
    bool IsPistonExtended();
    void SetPistonExtended(bool extended);
    bool IsStickyPistonHead();
};

static_assert(sizeof(Block) == 4, "Block should be exactly 4 bytes!");
