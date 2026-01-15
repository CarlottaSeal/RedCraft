#include "PlayerInventory.h"

PlayerInventory::PlayerInventory()
{
    InitializeDefaults();
}

PlayerInventory::~PlayerInventory()
{
}

void PlayerInventory::SetSelectedHotbarSlot(int slot)
{
    m_selectedHotbarSlot = m_activeRow * SLOTS_PER_ROW + slot;
}

Item const& PlayerInventory::GetHotbarSlot(int index) const
{
    static Item empty;
    if (index < 0 || index >= TOTAL_EQUIPMENT_SLOTS)
        return empty;
    return m_hotbar[index];
}

Item const& PlayerInventory::GetSelectedItem() const
{
    return GetHotbarSlot(m_selectedHotbarSlot);
}

Item const& PlayerInventory::GetMainInventorySlot(int index) const
{
    static Item empty;
    if (index < 0 || index >= 27)
        return empty;
    return m_mainInventory[index];
}

// // ========== 添加物品 ==========
// bool PlayerInventory::AddItem(std::string const& itemName, int count)
// {
//     if (itemName.empty() || count <= 0)
//         return false;
//     
//     // 策略：优先堆叠到快捷栏，再到主背包
//     
//     // 1. 尝试堆叠到快捷栏已有物品
//     for (int i = 0; i < 9; i++)
//     {
//         if (m_hotbar[i].GetName() == itemName)
//         {
//             m_hotbar[i].m_count += count;
//             return true;
//         }
//     }
//     
//     // 2. 尝试堆叠到主背包已有物品
//     for (int i = 0; i < 27; i++)
//     {
//         if (m_mainInventory[i].GetName() == itemName)
//         {
//             m_mainInventory[i].m_count += count;
//             return true;
//         }
//     }
//     
//     // 3. 放入快捷栏空槽位
//     for (int i = 0; i < 9; i++)
//     {
//         if (m_hotbar[i].IsEmpty())
//         {
//             m_hotbar[i].Set(itemName, count);
//             return true;
//         }
//     }
//     
//     // 4. 放入主背包空槽位
//     for (int i = 0; i < 27; i++)
//     {
//         if (m_mainInventory[i].IsEmpty())
//         {
//             m_mainInventory[i].Set(itemName, count);
//             return true;
//         }
//     }
//     
//     return false;  // 背包满了
// }
//
// // ========== 移除物品 ==========
// bool PlayerInventory::RemoveItem(std::string const& itemName, int count)
// {
//     if (itemName.empty() || count <= 0)
//         return false;
//     
//     int remaining = count;
//     
//     // 1. 从快捷栏移除
//     for (int i = 0; i < 9 && remaining > 0; i++)
//     {
//         if (m_hotbar[i].GetName() == itemName)
//         {
//             int toRemove = (m_hotbar[i].GetCount() < remaining) ? m_hotbar[i].GetCount() : remaining;
//             m_hotbar[i].m_count -= toRemove;
//             remaining -= toRemove;
//             
//             if (m_hotbar[i].GetCount() <= 0)
//                 m_hotbar[i].Clear();
//         }
//     }
//     
//     // 2. 从主背包移除
//     for (int i = 0; i < 27 && remaining > 0; i++)
//     {
//         if (m_mainInventory[i].GetName() == itemName)
//         {
//             int toRemove = (m_mainInventory[i].GetCount() < remaining) ? m_mainInventory[i].GetCount() : remaining;
//             m_mainInventory[i].m_count -= toRemove;
//             remaining -= toRemove;
//             
//             if (m_mainInventory[i].GetCount() <= 0)
//                 m_mainInventory[i].Clear();
//         }
//     }
//     
//     return remaining == 0;  // 是否成功移除全部
// }
//
// // ========== 消耗指定槽位 ==========
// bool PlayerInventory::ConsumeItem(int slotIndex, int count)
// {
//     if (slotIndex < 0 || slotIndex >= 9)
//         return false;
//     
//     Item& item = m_hotbar[slotIndex];
//     if (item.GetCount() < count)
//         return false;
//     
//     item.m_count -= count;
//     if (item.GetCount() <= 0)
//         item.Clear();
//     
//     return true;
// }

void PlayerInventory::SetHotbarSlot(int index, std::string const& itemName)
{
    if (index < 0 || index >= TOTAL_EQUIPMENT_SLOTS)
        return;
    m_hotbar[index].Set(itemName);
}

bool PlayerInventory::HasItem(std::string const& itemName, int minCount) const
{
    return true;
    //return GetItemCount(itemName) >= minCount;
}

int PlayerInventory::GetItemCount(std::string const& itemName) const
{
    // int total = 0;
    //
    // // 快捷栏
    // for (int i = 0; i < 9; i++)
    // {
    //     if (m_hotbar[i].GetName() == itemName)
    //         total += m_hotbar[i].GetCount();
    // }
    //
    // // 主背包
    // for (int i = 0; i < 27; i++)
    // {
    //     if (m_mainInventory[i].GetName() == itemName)
    //         total += m_mainInventory[i].GetCount();
    // }
    //
    // return total;
    return 999;
}

void PlayerInventory::InitializeDefaults()
{
    m_hotbar[0].Set("Glowstone");
    m_hotbar[1].Set("RedstoneLine");
    m_hotbar[2].Set("RedstoneTorch");
    m_hotbar[3].Set("RedstoneBlock");
    m_hotbar[4].Set("RedstonePiston") ;
    m_hotbar[5].Set("RedstoneLever");
    m_hotbar[6].Set("RedstoneLamp");
    m_hotbar[7].Set("RedstoneRepeater");
    m_hotbar[8].Set("RedstoneObserver");

    m_hotbar[9].Set("Wheat");
    m_hotbar[10].Set("Potato");
    m_hotbar[11].Set("Carrot");
    m_hotbar[12].Set("Beetroot");
    m_hotbar[13].Set("Pumpkin");
    m_hotbar[14].Set("Melon");
    m_hotbar[15].Set("NetherWart");
    m_hotbar[16].Set("SoulSand");
    m_hotbar[17].Set("Water");
}

void PlayerInventory::Clear()
{
    for (int i = 0; i < 9; i++)
        m_hotbar[i].Clear();
    
    for (int i = 0; i < 27; i++)
        m_mainInventory[i].Clear();
    
    m_selectedHotbarSlot = 0;
}

void PlayerInventory::SwitchHotbarRow()
{
    m_activeRow = (m_activeRow + 1) % 2;  // 0 ↔ 1
    
    int slotInRow = m_selectedHotbarSlot % SLOTS_PER_ROW;
    m_selectedHotbarSlot = m_activeRow * SLOTS_PER_ROW + slotInRow;
}

int PlayerInventory::GetActiveRow() const
{
    return m_activeRow;
}

int PlayerInventory::GetSelectedSlotInRow() const
{
    return m_selectedHotbarSlot % 9;
}

