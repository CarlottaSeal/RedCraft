#include "Item.h"

std::string Item::GetBlockFromItem(std::string const& itemName) //都没用到
{
    // 作物：种子/果实 → 初始阶段方块 
    if (itemName == "Wheat") return "Wheat0";
    if (itemName == "Carrot") return "Carrots0";
    if (itemName == "Potato") return "Potatoes0";
    if (itemName == "Beetroot") return "Beetroots0";
    
    if (itemName == "PumpkinSeeds") return "PumpkinStem0";
    if (itemName == "MelonSeeds") return "MelonStem0";
    if (itemName == "NetherWart") return "NetherWart0";
    
    // ========== 红石：物品 → 特殊方块 ==========
    if (itemName == "RedstoneDust") return "RedstoneWireDot";
    if (itemName == "RedstoneRepeater") return "RepeaterOff";
    if (itemName == "RedstoneComparator") return "Comparator";
    
    // ========== 灯具：物品 → 关闭状态 ==========
    if (itemName == "RedstoneLamp") return "RedstoneLamp";  // 默认关闭
    
    // ========== 火把：物品 → 点亮状态 ==========
    if (itemName == "RedstoneTorch") return "RedstoneTorch";  // 默认亮
    
    // ========== 活塞头：不能放置 ==========
    if (itemName == "PistonHead") return "";  // 空字符串 = 不能放置
    if (itemName == "StickyPistonHead") return "";
    
    // ========== 纯物品：不能放置 ==========
    if (itemName == "Bonemeal") return "";
    
    // ========== 默认规则：同名映射 ==========
    // Stone → Stone
    // Dirt → Dirt
    // OakLog → OakLog
    // 等等...
    return itemName;
}

std::string Item::GetBlockFromItem(Item item)
{
    return GetBlockFromItem(item.GetName());
}

bool Item::CanPlaceItem(std::string const& itemName)
{
    return !GetBlockFromItem(itemName).empty();
}

bool Item::CanPlaceItem(Item item)
{
    return !GetBlockFromItem(item.GetName()).empty();
}
