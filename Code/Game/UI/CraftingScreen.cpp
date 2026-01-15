#include "CraftingScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/Button.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Game.hpp"
extern Game* g_theGame;
CraftingScreen::CraftingScreen(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::CRAFTING_TABLE, g_theGame->m_screenCamera, true)
{
}

CraftingScreen::~CraftingScreen()
{
}

void CraftingScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildBackground();
    BuildCraftingGrid();
    BuildResultSlot();
    BuildPlayerGrid();
    RegisterSlotEvents();
}

void CraftingScreen::BuildBackground()
{
    AABB2 screenBounds(0, 0, 1600, 900);
    m_backgroundDimmer = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(0, 0, 0, 180),
        nullptr,
        false,
        Rgba8::BLACK
    );
    
    AABB2 craftingPanelBounds(300, 150, 1300, 750);
    m_craftingPanel = new Panel(
        m_canvas,
        craftingPanelBounds,
        Rgba8(60, 60, 60),
        nullptr,
        true,
        Rgba8(120, 120, 120)
    );
    
    TextSetting titleSetting;
    titleSetting.m_text = "Crafting Table";
    titleSetting.m_color = Rgba8(220, 220, 220);
    titleSetting.m_height = 36.0f;
    
    Vec2 titlePos(350, 710);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
}

void CraftingScreen::BuildCraftingGrid()
{
    float slotSize = 70.0f;
    float padding = 6.0f;
    Vec2 gridStart(450, 620);  // 左上角起始位置
    
    // 3x3 合成网格
    for (int row = 0; row < CRAFTING_GRID_SIZE; row++)
    {
        for (int col = 0; col < CRAFTING_GRID_SIZE; col++)
        {
            int slotIndex = row * CRAFTING_GRID_SIZE + col;
            
            float x = gridStart.x + col * (slotSize + padding);
            float y = gridStart.y - row * (slotSize + padding);
            
            AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
            
            ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
            m_craftingSlots.push_back(slot);
        }
    }
}

void CraftingScreen::BuildResultSlot()
{
    float slotSize = 70.0f;
    Vec2 resultPos(850, 550);  // 合成网格右侧
    
    AABB2 resultBounds(resultPos.x, resultPos.y, 
                       resultPos.x + slotSize, resultPos.y + slotSize);
    
    m_resultSlot = new ItemSlot(m_canvas, resultBounds, 0);
    m_resultSlot->SetDisabled(true);  // 结果槽不能直接放入物品
    
    // 箭头图标（可选）
    TextSetting arrowSetting;
    arrowSetting.m_text = "->";
    arrowSetting.m_color = Rgba8(180, 180, 180);
    arrowSetting.m_height = 40.0f;
    
    Vec2 arrowPos(760, 560);
    Text* arrow = new Text(m_canvas, arrowPos, arrowSetting);
    
    // Craft 按钮
    AABB2 craftButtonBounds(850, 480, 920, 530);
    m_craftButton = new Button(
        m_canvas,
        craftButtonBounds,
        Rgba8(100, 200, 100),
        Rgba8::WHITE,
        Rgba8::WHITE,
        "CraftItem"
    );
}

void CraftingScreen::BuildPlayerGrid()
{
    float slotSize = 70.0f;
    float padding = 6.0f;
    
    // 玩家主背包
    Vec2 mainGridStart(350, 400);
    for (int row = 0; row < PLAYER_ROWS; row++)
    {
        for (int col = 0; col < PLAYER_COLS; col++)
        {
            int slotIndex = row * PLAYER_COLS + col;
            
            float x = mainGridStart.x + col * (slotSize + padding);
            float y = mainGridStart.y - row * (slotSize + padding);
            
            AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
            
            ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
            m_playerSlots.push_back(slot);
        }
    }
    
    // 快捷栏
    Vec2 hotbarStart(350, 170);
    for (int i = 0; i < PLAYER_HOTBAR; i++)
    {
        int slotIndex = PLAYER_ROWS * PLAYER_COLS + i;
        
        float x = hotbarStart.x + i * (slotSize + padding);
        float y = hotbarStart.y;
        
        AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
        
        ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
        m_playerSlots.push_back(slot);
    }
}

void CraftingScreen::RegisterSlotEvents()
{
    // 合成网格槽位事件
    for (ItemSlot* slot : m_craftingSlots)
    {
        if (!slot) continue;
        
        slot->OnClickEvent([this]() {
            CheckRecipe();  // 每次放置物品后检查配方
        });
    }
    
    // Craft 按钮事件
    // (已经通过 EventSystem 注册)
}

void CraftingScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    // 持续检查配方
    CheckRecipe();
}

void CraftingScreen::OnCraftButtonClicked()
{
    CraftItem();
}

void CraftingScreen::CheckRecipe()
{
    // TODO: 实现配方匹配逻辑
    // 1. 获取当前合成网格中的物品
    // 2. 与已知配方对比
    // 3. 如果匹配，显示结果
    
    // 示例：简单的木板合成（任意位置放一个原木 = 4个木板）
    bool hasLog = false;
    for (ItemSlot* slot : m_craftingSlots)
    {
        if (!slot->IsEmpty() && slot->GetItem().itemID == 1)  // 假设1是原木ID
        {
            hasLog = true;
            break;
        }
    }
    
    if (hasLog && m_resultSlot)
    {
        ItemData planks;
        planks.itemID = 2;  // 木板ID
        planks.name = "Planks";
        planks.count = 4;
        // planks.texture = ...
        
        m_resultSlot->SetItem(planks);
        m_craftButton->SetEnabled(true);
    }
    else
    {
        if (m_resultSlot)
        {
            m_resultSlot->ClearItem();
        }
        if (m_craftButton)
        {
            m_craftButton->SetEnabled(false);
        }
    }
}

void CraftingScreen::CraftItem()
{
    // TODO: 执行合成
    // 1. 消耗合成网格中的材料
    // 2. 将结果物品给玩家
    
    if (!m_resultSlot || m_resultSlot->IsEmpty())
    {
        return;
    }
    
    // 获取结果
    ItemData result = m_resultSlot->GetItem();
    
    // 清空材料
    for (ItemSlot* slot : m_craftingSlots)
    {
        if (!slot->IsEmpty())
        {
            ItemData material = slot->GetItem();
            material.count--;
            
            if (material.count <= 0)
            {
                slot->ClearItem();
            }
            else
            {
                slot->SetItem(material);
            }
        }
    }
    
    // 给玩家物品（尝试添加到背包）
    bool added = false;
    for (ItemSlot* slot : m_playerSlots)
    {
        if (slot->IsEmpty())
        {
            slot->SetItem(result);
            added = true;
            break;
        }
        else if (slot->GetItem().CanStackWith(result))
        {
            int remaining = slot->AddItem(result);
            if (remaining == 0)
            {
                added = true;
                break;
            }
        }
    }
    
    // 清空结果槽
    m_resultSlot->ClearItem();
    
    // 重新检查配方
    CheckRecipe();
}