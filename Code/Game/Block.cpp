#include "Block.h"

// uint8_t Block::GetOutdoorLight() const
// {
//     return (m_lightData & LIGHT_MASK_OUTDOOR) >> 4;
// }

void Block::SetOutdoorLight(uint8_t level)
{
    level = (level > 15) ? 15 : level;  // 限制在0-15
    m_lightData = (m_lightData & LIGHT_MASK_INDOOR) | (level << 4);
}

// uint8_t Block::GetIndoorLight() const
// {
//     return m_lightData & LIGHT_MASK_INDOOR;
// }

void Block::SetIndoorLight(uint8_t level)
{
    level = (level > 15) ? 15 : level;
    m_lightData = (m_lightData & LIGHT_MASK_OUTDOOR) | level;
}

bool Block::IsSky() const

{
    return (m_flags & BLOCK_BIT_IS_SKY) != 0;
}

void Block::SetIsSky(bool isSky)
{
    if (isSky)
        m_flags |= BLOCK_BIT_IS_SKY;
    else
        m_flags &= ~BLOCK_BIT_IS_SKY;
}

bool Block::IsLightDirty() const
{
    return (m_flags & BLOCK_BIT_IS_LIGHT_DIRTY) != 0;
}

void Block::SetLightDirty(bool isDirty)
{
    if (isDirty)
        m_flags |= BLOCK_BIT_IS_LIGHT_DIRTY;
    else
        m_flags &= ~BLOCK_BIT_IS_LIGHT_DIRTY;
}

// bool Block::IsOpaque() const
// {
//     return (m_flags & BLOCK_BIT_IS_OPAQUE) != 0;
// }

void Block::SetIsOpaque(bool isOpaque)
{
    if (isOpaque)
        m_flags |= BLOCK_BIT_IS_OPAQUE;
    else
        m_flags &= ~BLOCK_BIT_IS_OPAQUE;
}

bool Block::IsSolid() const
{
    return (m_flags & BLOCK_BIT_IS_SOLID) != 0;
}

void Block::SetIsSolid(bool isSolid)
{
    if (isSolid)
        m_flags |= BLOCK_BIT_IS_SOLID;
    else
        m_flags &= ~BLOCK_BIT_IS_SOLID;
}

bool Block::IsVisible() const
{
    return (m_flags & BLOCK_BIT_IS_VISIBLE) != 0;
}

void Block::SetIsVisible(bool isVisible)
{
    if (isVisible)
        m_flags |= BLOCK_BIT_IS_VISIBLE;
    else
        m_flags &= ~BLOCK_BIT_IS_VISIBLE;
}

void Block::SetSpecialState(bool state)
{
    if (state) m_redstoneData |= 0x80;
    else m_redstoneData &= ~0x80;
}

Direction Block::GetPistonFacing()
{
    return (Direction)GetBlockFacing();
}

bool Block::IsPistonExtended()
{
    return GetSpecialState();
}

void Block::SetPistonExtended(bool extended)
{
    SetSpecialState(extended);
}

bool Block::IsStickyPistonHead()
{
    return (GetRedstonePower() & 0x08) != 0;
};

uint8_t Block::GetRepeaterDelay()
{
    // 返回 1-4 tick
    return (GetRedstonePower() & 0x03) + 1;
}

void Block::SetRepeaterDelay(uint8_t delay)
{
    // delay: 1-4
    delay = (delay < 1) ? 1 : (delay > 4) ? 4 : delay;
    uint8_t power = GetRedstonePower();
    power = (power & 0xFC) | ((delay - 1) & 0x03);
    SetRedstonePower(power);
}

bool Block::IsRepeaterLocked()
{
    return (GetRedstonePower() & 0x04) != 0;
}

void Block::SetRepeaterLocked(bool locked)
{
    uint8_t power = GetRedstonePower();
    if (locked)
        power |= 0x04;
    else
        power &= ~0x04;
    SetRedstonePower(power);
}
