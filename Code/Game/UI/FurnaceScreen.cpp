// Game/UI/FurnaceScreen.cpp
#include "FurnaceScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/ProgressBar.h"
#include "Engine/Core/EngineCommon.hpp"

FurnaceScreen::FurnaceScreen(UISystem* uiSystem, int furnaceID)
    : UIScreen(uiSystem, UIScreenType::FURNACE, true)
    , m_furnaceID(furnaceID)
{
}

FurnaceScreen::~FurnaceScreen()
{
}

void FurnaceScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    
    m_camera->SetOrthographicView(Vec2(0, 0), Vec2(1600, 900));
    
    BuildBackground();
    BuildFurnaceSlots();
    BuildProgressBars();
    BuildPlayerGrid();
    RegisterSlotEvents();
}

void FurnaceScreen::BuildBackground()
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
    m_elements.push_back(m_backgroundDimmer);
    
    AABB2 furnacePanelBounds(400, 150, 1200, 750);
    m_furnacePanel = new Panel(
        m_canvas,
        furnacePanelBounds,
        Rgba8(70, 70, 70),
        nullptr,
        true,
        Rgba8(120, 120, 120)
    );
    m_elements.push_back(m_furnacePanel);
    
    TextSetting titleSetting;
    titleSetting.m_text = "Furnace";
    titleSetting.m_color = Rgba8(220, 220, 220);
    titleSetting.m_height = 36.0f;
    
    Vec2 titlePos(450, 710);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
    m_elements.push_back(m_titleText);
}

void FurnaceScreen::BuildFurnaceSlots()
{
    float slotSize = 70.0f;
    
    // 输入槽（上方）
    AABB2 inputBounds(650, 620, 650 + slotSize, 620 + slotSize);
    m_inputSlot = new ItemSlot(m_canvas, inputBounds, 0);
    m_elements.push_back(m_inputSlot);
    
    // 燃料槽（下方）
    AABB2 fuelBounds(650, 500, 650 + slotSize, 500 + slotSize);
    m_fuelSlot = new ItemSlot(m_canvas, fuelBounds, 1);
    m_elements.push_back(m_fuelSlot);
    
    // 输出槽（右侧）
    AABB2 outputBounds(880, 560, 880 + slotSize, 560 + slotSize);
    m_outputSlot = new ItemSlot(m_canvas, outputBounds, 2);
    m_elements.push_back(m_outputSlot);
    
    // 熔炉图标（装饰）
    TextSetting iconSetting;
    iconSetting.m_text = "";  // 或者用图片
    iconSetting.m_color = Rgba8(255, 140, 0);
    iconSetting.m_height = 32.0f;
    
    Vec2 iconPos(670, 560);
    Text* furnaceIcon = new Text(m_canvas, iconPos, iconSetting);
    m_elements.push_back(furnaceIcon);
}

void FurnaceScreen::BuildProgressBars()
{
    // 烧炼进度条（横向，从输入到输出）
    AABB2 smeltingBounds(750, 580, 860, 595);
    m_smeltingProgressBar = new ProgressBar(
        m_canvas,
        smeltingBounds,
        0.0f,
        100.0f,
        Rgba8(50, 50, 50),
        Rgba8(255, 140, 0),  // 橙色
        Rgba8::WHITE,
        true
    );
    m_smeltingProgressBar->SetOrientation(ProgressBarOrientation::HORIZONTAL);
    m_elements.push_back(m_smeltingProgressBar);
    
    // 燃料消耗进度条（纵向，在燃料槽内）
    AABB2 fuelBounds(655, 505, 670, 585);
    m_fuelProgressBar = new ProgressBar(
        m_canvas,
        fuelBounds,
        0.0f,
        100.0f,
        Rgba8(50, 50, 50),
        Rgba8(255, 69, 0),  // 深橙色（火焰）
        Rgba8::WHITE,
        true
    );
    m_fuelProgressBar->SetOrientation(ProgressBarOrientation::VERTICAL);
    m_elements.push_back(m_fuelProgressBar);
}

void FurnaceScreen::BuildPlayerGrid()
{
    float slotSize = 70.0f;
    float padding = 6.0f;
    
    Vec2 mainGridStart(450, 420);
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
            m_elements.push_back(slot);
        }
    }
    
    Vec2 hotbarStart(450, 190);
    for (int i = 0; i < PLAYER_HOTBAR; i++)
    {
        int slotIndex = PLAYER_ROWS * PLAYER_COLS + i;
        
        float x = hotbarStart.x + i * (slotSize + padding);
        float y = hotbarStart.y;
        
        AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
        
        ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
        m_playerSlots.push_back(slot);
        m_elements.push_back(slot);
    }
}

void FurnaceScreen::RegisterSlotEvents()
{
    // TODO: 注册槽位事件
}

void FurnaceScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    // TODO: 从游戏逻辑获取熔炉状态并更新UI
}

void FurnaceScreen::SetInputItem(ItemData const& item)
{
    if (m_inputSlot)
    {
        m_inputSlot->SetItem(item);
    }
}

void FurnaceScreen::SetFuelItem(ItemData const& item)
{
    if (m_fuelSlot)
    {
        m_fuelSlot->SetItem(item);
    }
}

void FurnaceScreen::SetOutputItem(ItemData const& item)
{
    if (m_outputSlot)
    {
        m_outputSlot->SetItem(item);
    }
}

void FurnaceScreen::UpdateSmeltingProgress(float progress)
{
    if (m_smeltingProgressBar)
    {
        m_smeltingProgressBar->SetValueNormalized(progress);
    }
}

void FurnaceScreen::UpdateFuelProgress(float progress)
{
    if (m_fuelProgressBar)
    {
        m_fuelProgressBar->SetValueNormalized(progress);
    }
}