#include "FarmMonitorScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/Button.h"
#include "Engine/UI/ProgressBar.h"
#include "Engine/UI/Sprite.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/World.h"
#include "Game/Chunk.h"
#include "Game/Block.h"
#include "Game/BlockDefinition.h"
#include "Game/Game.hpp"
#include "Game/Gameplay/CropSystem.h"
#include "Game/Gameplay/CropDropManager.h"
extern Game* g_theGame;
FarmMonitorScreen::FarmMonitorScreen(UISystem* uiSystem, World* world)
    : UIScreen(uiSystem, UIScreenType::CUSTOM, g_theGame->m_screenCamera, true)
    , m_world(world)
{
}

FarmMonitorScreen::~FarmMonitorScreen()
{
}

void FarmMonitorScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildBackground();
    BuildCropsSection();
    BuildObserversSection();
    BuildStatsSection();
    BuildButtons();
}

void FarmMonitorScreen::BuildBackground()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    // 全屏半透明遮罩
    AABB2 screenBounds(0, 0, size.x, size.y);
    m_backgroundDimmer = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(0, 0, 0, 200),
        nullptr,
        false,
        Rgba8::BLACK
    );
    
    // 主面板 - 占屏幕中心75%宽度，82%高度
    float panelWidth = size.x * 0.75f;
    float panelHeight = size.y * 0.82f;
    float panelX = size.x * 0.125f;
    float panelY = size.y * 0.09f;
    
    AABB2 mainBounds(panelX, panelY, panelX + panelWidth, panelY + panelHeight);
    m_mainPanel = new Panel(
        m_canvas,
        mainBounds,
        Rgba8(45, 55, 45),  // 农场风格的深绿色调
        nullptr,
        true,
        Rgba8(80, 120, 80)
    );
    
    // 标题
    TextSetting titleSetting;
    titleSetting.m_text = "Farm Monitor";
    titleSetting.m_color = Rgba8(200, 255, 200);
    titleSetting.m_height = size.y * 0.047f;
    
    Vec2 titlePos(panelX + size.x * 0.03f, panelY + panelHeight - size.y * 0.055f);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
    
    // 副标题（区域信息）
    TextSetting subtitleSetting;
    subtitleSetting.m_text = "Automatic Farm Control System";
    subtitleSetting.m_color = Rgba8(150, 200, 150);
    subtitleSetting.m_height = size.y * 0.022f;
    
    Vec2 subtitlePos(panelX + size.x * 0.03f, panelY + panelHeight - size.y * 0.088f);
    Text* subtitle = new Text(m_canvas, subtitlePos, subtitleSetting);
}

void FarmMonitorScreen::BuildCropsSection()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    // 计算主面板位置
    float panelWidth = size.x * 0.75f;
    float panelHeight = size.y * 0.82f;
    float panelX = size.x * 0.125f;
    float panelY = size.y * 0.09f;
    
    // 作物区域面板 - 左侧，占主面板的57%宽度，46%高度
    float cropsPanelWidth = panelWidth * 0.57f;
    float cropsPanelHeight = panelHeight * 0.46f;
    float cropsPanelX = panelX + size.x * 0.0125f;
    float cropsPanelY = panelY + panelHeight * 0.405f;
    
    AABB2 cropsPanelBounds(cropsPanelX, cropsPanelY, 
                           cropsPanelX + cropsPanelWidth, 
                           cropsPanelY + cropsPanelHeight);
    m_cropsPanel = new Panel(
        m_canvas,
        cropsPanelBounds,
        Rgba8(35, 45, 35),
        nullptr,
        true,
        Rgba8(60, 90, 60)
    );
    
    // 作物区域标题
    TextSetting sectionTitleSetting;
    sectionTitleSetting.m_text = "Crop Status";
    sectionTitleSetting.m_color = Rgba8(180, 220, 180);
    sectionTitleSetting.m_height = size.y * 0.029f;
    
    Vec2 cropsTitlePos(cropsPanelX + size.x * 0.0125f, 
                       cropsPanelY + cropsPanelHeight - size.y * 0.033f);
    m_cropsSectionTitle = new Text(m_canvas, cropsTitlePos, sectionTitleSetting);

    // 创建作物槽位 (4x3 = 12个)
    float slotWidthRatio = 0.094f;
    float slotHeightRatio = 0.1f;
    float paddingXRatio = 0.0075f;
    float paddingYRatio = 0.011f;
    
    float slotWidth = size.x * slotWidthRatio;
    float slotHeight = size.y * slotHeightRatio;
    float paddingX = size.x * paddingXRatio;
    float paddingY = size.y * paddingYRatio;
    
    Vec2 gridStart(cropsPanelX + size.x * 0.0125f, 
                   cropsPanelY + cropsPanelHeight - size.y * 0.056f);
    
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < CROPS_PER_ROW; col++)
        {
            int index = row * CROPS_PER_ROW + col;
            
            float x = gridStart.x + col * (slotWidth + paddingX);
            float y = gridStart.y - row * (slotHeight + paddingY);
            
            AABB2 slotBounds(x, y - slotHeight, x + slotWidth, y);
            
            // 槽位背景
            Panel* slotPanel = new Panel(
                m_canvas,
                slotBounds,
                Rgba8(25, 35, 25),
                nullptr,
                true,
                Rgba8(50, 70, 50)
            );
            m_cropSlotPanels.push_back(slotPanel);
            
            // 作物图标 (左侧) - 占槽位宽度33%，高度65%
            float iconSize = slotWidth * 0.33f;
            float iconPadding = slotWidth * 0.033f;
            float iconTopPadding = slotHeight * 0.28f;
            
            AABB2 iconBounds(x + iconPadding, 
                           y - slotHeight + iconTopPadding, 
                           x + iconPadding + iconSize, 
                           y - iconPadding);
            Sprite* icon = new Sprite(m_canvas, iconBounds, nullptr);
            icon->SetEnabled(false);
            m_cropIcons.push_back(icon);
            
            // 作物名称 (右侧上方)
            TextSetting nameSetting;
            nameSetting.m_text = "---";
            nameSetting.m_color = Rgba8(200, 200, 200);
            nameSetting.m_height = slotHeight * 0.178f;
            
            Vec2 namePos(x + slotWidth * 0.4f, y - slotHeight * 0.167f);
            Text* nameText = new Text(m_canvas, namePos, nameSetting);
            m_cropNameTexts.push_back(nameText);
            
            // 生长阶段文本 (右侧中间)
            TextSetting stageSetting;
            stageSetting.m_text = "Stage: -/-";
            stageSetting.m_color = Rgba8(150, 150, 150);
            stageSetting.m_height = slotHeight * 0.156f;
            
            Vec2 stagePos(x + slotWidth * 0.4f, y - slotHeight * 0.389f);
            Text* stageText = new Text(m_canvas, stagePos, stageSetting);
            m_cropStageTexts.push_back(stageText);
            
            // 进度条 (底部)
            float progressBarPadding = slotWidth * 0.033f;
            float progressBarHeight = slotHeight * 0.144f;
            
            AABB2 progressBounds(x + progressBarPadding, 
                               y - slotHeight + progressBarPadding, 
                               x + slotWidth - progressBarPadding, 
                               y - slotHeight + progressBarPadding + progressBarHeight);
            ProgressBar* progressBar = new ProgressBar(
                m_canvas,
                progressBounds,
                0.0f, 100.0f,
                Rgba8(30, 30, 30),
                Rgba8(100, 200, 100),
                Rgba8(60, 80, 60),
                true
            );
            progressBar->SetValue(0.0f);
            m_cropProgressBars.push_back(progressBar);
        }
    }
}

void FarmMonitorScreen::BuildObserversSection()
{
    // 侦测器区域面板
    AABB2 observersPanelBounds(920, 380, 1380, 720);
    m_observersPanel = new Panel(
        m_canvas,
        observersPanelBounds,
        Rgba8(45, 35, 35),  // 红石风格的暗红色调
        nullptr,
        true,
        Rgba8(90, 60, 60)
    );
    
    // 侦测器区域标题
    TextSetting sectionTitleSetting;
    sectionTitleSetting.m_text = "Observer Status";
    sectionTitleSetting.m_color = Rgba8(255, 180, 180);
    sectionTitleSetting.m_height = 26.0f;
    
    Vec2 observersTitlePos(940, 690);
    m_observersSectionTitle = new Text(m_canvas, observersTitlePos, sectionTitleSetting);
    
    // 创建侦测器槽位 (4个)
    float slotWidth = 200.0f;
    float slotHeight = 65.0f;
    float paddingY = 10.0f;
    Vec2 gridStart(940, 650);
    
    for (int i = 0; i < MAX_DISPLAYED_OBSERVERS; i++)
    {
        float x = gridStart.x;
        float y = gridStart.y - i * (slotHeight + paddingY);
        
        AABB2 slotBounds(x, y - slotHeight, x + slotWidth, y);
        
        // 槽位背景
        Panel* slotPanel = new Panel(
            m_canvas,
            slotBounds,
            Rgba8(35, 25, 25),
            nullptr,
            true,
            Rgba8(70, 50, 50)
        );
        m_observerSlotPanels.push_back(slotPanel);
        
        // 触发指示灯 (左侧圆形)
        AABB2 indicatorBounds(x + 10, y - slotHeight + 15, x + 40, y - 15);
        Panel* indicator = new Panel(
            m_canvas,
            indicatorBounds,
            Rgba8(60, 30, 30),  // 默认暗红
            nullptr,
            true,
            Rgba8(100, 50, 50)
        );
        m_observerIndicators.push_back(indicator);
        
        // 位置文本
        TextSetting posSetting;
        posSetting.m_text = "Pos: ---";
        posSetting.m_color = Rgba8(180, 180, 180);
        posSetting.m_height = 14.0f;
        
        Vec2 posTextPos(x + 50, y - 20);
        Text* posText = new Text(m_canvas, posTextPos, posSetting);
        m_observerPosTexts.push_back(posText);
        
        // 状态文本
        TextSetting statusSetting;
        statusSetting.m_text = "Status: Idle";
        statusSetting.m_color = Rgba8(150, 150, 150);
        statusSetting.m_height = 14.0f;
        
        Vec2 statusTextPos(x + 50, y - 42);
        Text* statusText = new Text(m_canvas, statusTextPos, statusSetting);
        m_observerStatusTexts.push_back(statusText);
    }
    
    // 添加侦测器说明
    TextSetting helpSetting;
    helpSetting.m_text = "Green = Triggered | Red = Idle";
    helpSetting.m_color = Rgba8(120, 120, 120);
    helpSetting.m_height = 12.0f;
    
    Vec2 helpPos(940, 395);
    Text* helpText = new Text(m_canvas, helpPos, helpSetting);
}

void FarmMonitorScreen::BuildStatsSection()
{
    // 统计面板
    AABB2 statsBounds(220, 100, 900, 360);
    m_statsPanel = new Panel(
        m_canvas,
        statsBounds,
        Rgba8(40, 40, 50),
        nullptr,
        true,
        Rgba8(70, 70, 90)
    );

    // 统计标题
    TextSetting statsTitleSetting;
    statsTitleSetting.m_text = "Farm Statistics";
    statsTitleSetting.m_color = Rgba8(180, 180, 220);
    statsTitleSetting.m_height = 24.0f;
    
    Vec2 statsTitlePos(240, 330);
    Text* statsTitle = new Text(m_canvas, statsTitlePos, statsTitleSetting);
    
    // 统计信息
    TextSetting statSetting;
    statSetting.m_color = Rgba8(200, 200, 200);
    statSetting.m_height = 20.0f;
    
    // 总作物数
    statSetting.m_text = "Total Crops: 0";
    Vec2 totalPos(260, 290);
    m_totalCropsText = new Text(m_canvas, totalPos, statSetting);
    
    // 成熟作物数
    statSetting.m_text = "Mature Crops: 0";
    Vec2 maturePos(260, 260);
    m_matureCropsText = new Text(m_canvas, maturePos, statSetting);
    
    // 侦测器数量
    statSetting.m_text = "Active Observers: 0";
    Vec2 observerPos(260, 230);
    m_observerCountText = new Text(m_canvas, observerPos, statSetting);
    
    // 平均生长进度
    statSetting.m_text = "Average Growth: 0%";
    Vec2 avgPos(260, 200);
    m_avgGrowthText = new Text(m_canvas, avgPos, statSetting);

    // 掉落物统计
    statSetting.m_text = "Pending Drops: 0";
    Vec2 dropsPos(500, 290);
    m_dropsCountText = new Text(m_canvas, dropsPos, statSetting);
    
    // 物品总数
    statSetting.m_text = "Total Items: 0";
    Vec2 itemsPos(500, 260);
    m_dropsItemsText = new Text(m_canvas, itemsPos, statSetting);

    // 进度可视化 - 大进度条
    AABB2 bigProgressBounds(260, 120, 700, 160);
    ProgressBar* bigProgress = new ProgressBar(
        m_canvas,
        bigProgressBounds,
        0.0f, 100.0f,
        Rgba8(30, 30, 40),
        Rgba8(100, 180, 100),
        Rgba8(80, 80, 100),
        true
    );
    bigProgress->SetValue(0.0f);
    
    // 进度标签
    TextSetting progressLabelSetting;
    progressLabelSetting.m_text = "Overall Farm Progress";
    progressLabelSetting.m_color = Rgba8(150, 150, 170);
    progressLabelSetting.m_height = 14.0f;
    
    Vec2 progressLabelPos(260, 170);
    Text* progressLabel = new Text(m_canvas, progressLabelPos, progressLabelSetting);
}

void FarmMonitorScreen::BuildButtons()
{
    float buttonWidth = 180.0f;
    float buttonHeight = 45.0f;
    float startX = 940.0f;
    float startY = 320.0f;
    float spacing = 15.0f;
    
    // 刷新按钮
    AABB2 refreshBounds(startX, startY, startX + buttonWidth, startY + buttonHeight);
    m_refreshButton = new Button(
        m_canvas,
        refreshBounds,
        Rgba8(80, 150, 80),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Refresh Data",
        Vec2(0.5f, 0.5f)
    );
    
    // 一键收获按钮
    startY -= (buttonHeight + spacing);
    AABB2 harvestBounds(startX, startY, startX + buttonWidth, startY + buttonHeight);
    m_harvestAllButton = new Button(
        m_canvas,
        harvestBounds,
        Rgba8(200, 180, 80),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Harvest All Mature",
        Vec2(0.5f, 0.5f)
    );
    
    // 关闭按钮
    startY -= (buttonHeight + spacing);
    AABB2 closeBounds(startX, startY, startX + buttonWidth, startY + buttonHeight);
    m_closeButton = new Button(
        m_canvas,
        closeBounds,
        Rgba8(180, 80, 80),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Close",
        Vec2(0.5f, 0.5f)
    );
    
    // 区域设置提示
    TextSetting tipSetting;
    tipSetting.m_text = "Tip: Use '/farm setarea' to define monitor region";
    tipSetting.m_color = Rgba8(120, 120, 120);
    tipSetting.m_height = 12.0f;
    
    Vec2 tipPos(940, 115);
    Text* tipText = new Text(m_canvas, tipPos, tipSetting);
}

void FarmMonitorScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    // 自动刷新
    m_refreshTimer += deltaSeconds;
    if (m_refreshTimer >= AUTO_REFRESH_INTERVAL)
    {
        m_refreshTimer = 0.0f;
        RefreshData();
    }
}

void FarmMonitorScreen::OnEnter()
{
    UIScreen::OnEnter();
    RefreshData();
}

void FarmMonitorScreen::OnExit()
{
    UIScreen::OnExit();
}

void FarmMonitorScreen::SetMonitorArea(const IntVec3& minCorner, const IntVec3& maxCorner)
{
    m_monitorMin = minCorner;
    m_monitorMax = maxCorner;
    m_hasMonitorArea = true;
    RefreshData();
}

void FarmMonitorScreen::RefreshData()
{
    CollectCropData();
    CollectObserverData();
    UpdateCropDisplays();
    UpdateObserverDisplays();
    UpdateStatsDisplay();
}

void FarmMonitorScreen::CollectCropData()
{
    m_cropData.clear();
    
    if (!m_world || !m_hasMonitorArea)
        return;
    
    CropSystem* cropSystem = m_world->m_cropSystem;
    if (!cropSystem)
        return;
    
    // 扫描监控区域内的所有作物
    for (int x = m_monitorMin.x; x <= m_monitorMax.x; x++)
    {
        for (int y = m_monitorMin.y; y <= m_monitorMax.y; y++)
        {
            for (int z = m_monitorMin.z; z <= m_monitorMax.z; z++)
            {
                Block block = m_world->GetBlockAtWorldCoords(x, y, z);
                
                if (cropSystem->IsGrowable(block.m_typeIndex))
                {
                    CropMonitorData data;
                    data.m_worldPos = IntVec3(x, y, z);
                    data.m_blockType = block.m_typeIndex;
                    
                    const CropDefinition* def = CropDefinition::GetDefinition(block.m_typeIndex);;
                    if (def)
                    {
                        data.m_cropName = def->m_name;
                        data.m_maxStage = (int)def->m_stages.size() - 1;
                        
                        // 找到当前阶段
                        for (int i = 0; i < (int)def->m_stages.size(); i++)
                        {
                            if (def->m_stages[i].m_blockType == block.m_typeIndex)
                            {
                                data.m_currentStage = i;
                                data.m_isMature = def->m_stages[i].m_isMature;
                                break;
                            }
                        }
                        
                        data.m_growthProgress = (data.m_maxStage > 0) 
                            ? (float)data.m_currentStage / (float)data.m_maxStage 
                            : 1.0f;
                    }
                    
                    m_cropData.push_back(data);
                    
                    if (m_cropData.size() >= MAX_DISPLAYED_CROPS * 3)  // 收集更多用于统计
                        break;
                }
            }
        }
    }
}

void FarmMonitorScreen::CollectObserverData()
{
    m_observerData.clear();
    
    if (!m_world || !m_hasMonitorArea)
        return;
    
    // 扫描监控区域内的所有侦测器
    for (int x = m_monitorMin.x; x <= m_monitorMax.x; x++)
    {
        for (int y = m_monitorMin.y; y <= m_monitorMax.y; y++)
        {
            for (int z = m_monitorMin.z; z <= m_monitorMax.z; z++)
            {
                Block block = m_world->GetBlockAtWorldCoords(x, y, z);
                
                if (block.m_typeIndex == BLOCK_TYPE_REDSTONE_OBSERVER)
                {
                    ObserverMonitorData data;
                    data.m_worldPos = IntVec3(x, y, z);
                    data.m_facing = (Direction)block.GetBlockFacing();
                    data.m_isTriggered = block.IsObserverTriggered();  // 需要在Block中实现
                    
                    m_observerData.push_back(data);
                    
                    if (m_observerData.size() >= MAX_DISPLAYED_OBSERVERS)
                        break;
                }
            }
        }
    }
}

void FarmMonitorScreen::UpdateCropDisplays()
{
    for (int i = 0; i < MAX_DISPLAYED_CROPS; i++)
    {
        if (i < (int)m_cropData.size())
        {
            const CropMonitorData& data = m_cropData[i];
            
            // 更新图标
            if (m_cropIcons[i])
            {
                m_cropIcons[i]->SetTexture(GetCropTexture(data.m_blockType));
                m_cropIcons[i]->SetEnabled(true);
            }
            
            // 更新名称
            if (m_cropNameTexts[i])
            {
                m_cropNameTexts[i]->SetText(GetCropDisplayName(data.m_blockType));
                m_cropNameTexts[i]->SetColor(data.m_isMature ? Rgba8(100, 255, 100) : Rgba8(200, 200, 200));
            }
            
            // 更新阶段文本
            if (m_cropStageTexts[i])
            {
                std::string stageStr = "Stage: " + std::to_string(data.m_currentStage + 1) 
                                     + "/" + std::to_string(data.m_maxStage + 1);
                m_cropStageTexts[i]->SetText(stageStr);
            }
            
            // 更新进度条
            if (m_cropProgressBars[i])
            {
                m_cropProgressBars[i]->SetValueNormalized(data.m_growthProgress);
                m_cropProgressBars[i]->SetFillColor(GetGrowthProgressColor(data.m_growthProgress));
            }
            
            // 显示槽位
            if (m_cropSlotPanels[i])
            {
                m_cropSlotPanels[i]->SetEnabled(true);
            }
        }
        else
        {
            // 隐藏空槽位
            if (m_cropIcons[i]) m_cropIcons[i]->SetEnabled(false);
            if (m_cropNameTexts[i]) m_cropNameTexts[i]->SetText("---");
            if (m_cropStageTexts[i]) m_cropStageTexts[i]->SetText("Stage: -/-");
            if (m_cropProgressBars[i]) m_cropProgressBars[i]->SetValue(0.0f);
        }
    }
}

void FarmMonitorScreen::UpdateObserverDisplays()
{
    for (int i = 0; i < MAX_DISPLAYED_OBSERVERS; i++)
    {
        if (i < (int)m_observerData.size())
        {
            const ObserverMonitorData& data = m_observerData[i];
            
            // 更新指示灯颜色
            if (m_observerIndicators[i])
            {
                Rgba8 indicatorColor = data.m_isTriggered 
                    ? Rgba8(100, 255, 100)   // 绿色 = 触发
                    : Rgba8(100, 40, 40);    // 暗红 = 空闲
                m_observerIndicators[i]->SetBackgroundColor(indicatorColor);
            }
            
            // 更新位置文本
            if (m_observerPosTexts[i])
            {
                std::string posStr = "Pos: (" + std::to_string(data.m_worldPos.x) 
                                   + ", " + std::to_string(data.m_worldPos.y)
                                   + ", " + std::to_string(data.m_worldPos.z) + ")";
                m_observerPosTexts[i]->SetText(posStr);
            }
            
            // 更新状态文本
            if (m_observerStatusTexts[i])
            {
                std::string statusStr = data.m_isTriggered ? "Status: TRIGGERED" : "Status: Idle";
                m_observerStatusTexts[i]->SetText(statusStr);
                m_observerStatusTexts[i]->SetColor(data.m_isTriggered 
                    ? Rgba8(100, 255, 100) 
                    : Rgba8(150, 150, 150));
            }
        }
        else
        {
            // 隐藏空槽位
            if (m_observerIndicators[i]) 
                m_observerIndicators[i]->SetBackgroundColor(Rgba8(40, 40, 40));
            if (m_observerPosTexts[i]) 
                m_observerPosTexts[i]->SetText("Pos: ---");
            if (m_observerStatusTexts[i]) 
                m_observerStatusTexts[i]->SetText("Status: N/A");
        }
    }
}

void FarmMonitorScreen::UpdateStatsDisplay()
{
    int totalCrops = (int)m_cropData.size();
    int matureCrops = 0;
    float totalProgress = 0.0f;
    
    for (const CropMonitorData& data : m_cropData)
    {
        if (data.m_isMature) matureCrops++;
        totalProgress += data.m_growthProgress;
    }
    
    float avgProgress = (totalCrops > 0) ? (totalProgress / totalCrops) : 0.0f;
    
    if (m_totalCropsText)
        m_totalCropsText->SetText("Total Crops: " + std::to_string(totalCrops));
    
    if (m_matureCropsText)
    {
        m_matureCropsText->SetText("Mature Crops: " + std::to_string(matureCrops));
        m_matureCropsText->SetColor(matureCrops > 0 ? Rgba8(100, 255, 100) : Rgba8(200, 200, 200));
    }
    
    if (m_observerCountText)
        m_observerCountText->SetText("Active Observers: " + std::to_string((int)m_observerData.size()));
    
    if (m_avgGrowthText)
    {
        int avgPercent = (int)(avgProgress * 100.0f);
        m_avgGrowthText->SetText("Average Growth: " + std::to_string(avgPercent) + "%");
    }

    if (m_world && m_world->m_cropSystem)
    {
        DropStatistics dropStats = m_world->m_cropSystem->GetCropDropManager()->GetStatistics();
        
        if (m_dropsCountText)
        {
            m_dropsCountText->SetText("Pending Drops: " + std::to_string(dropStats.m_totalDrops));
        }
        
        if (m_dropsItemsText)
        {
            m_dropsItemsText->SetText("Total Items: " + std::to_string(dropStats.m_totalItems));
            
            // 如果有物品，显示绿色
            if (dropStats.m_totalItems > 0)
            {
                m_dropsItemsText->SetColor(Rgba8(100, 255, 100));
            }
            else
            {
                m_dropsItemsText->SetColor(Rgba8(200, 200, 200));
            }
        }
    }
}

void FarmMonitorScreen::OnRefreshClicked()
{
    RefreshData();
}

void FarmMonitorScreen::OnHarvestAllClicked()
{
    if (!m_world)
        return;
    
    CropSystem* cropSystem = m_world->m_cropSystem;
    if (!cropSystem)
        return;
    
    // 收获所有成熟作物
    for (const CropMonitorData& data : m_cropData)
    {
        if (data.m_isMature)
        {
            BlockIterator iter = m_world->GetBlockIterator(data.m_worldPos);
            if (iter.IsValid())
            {
                cropSystem->Harvest(iter);
            }
        }
    }
    
    RefreshData();
}

void FarmMonitorScreen::OnCloseClicked()
{
    // 通过事件系统关闭
    // g_theEventSystem->FireEvent("CloseFarmMonitor");
}

Rgba8 FarmMonitorScreen::GetGrowthProgressColor(float progress) const
{
    if (progress >= 1.0f)
        return Rgba8(100, 255, 100);  // 满绿
    else if (progress >= 0.7f)
        return Rgba8(180, 255, 100);  // 黄绿
    else if (progress >= 0.4f)
        return Rgba8(255, 200, 80);   // 橙黄
    else
        return Rgba8(255, 120, 80);   // 橙红
}

std::string FarmMonitorScreen::GetCropDisplayName(uint8_t blockType) const
{
    // 根据方块类型返回显示名称
    // 这里简化处理，实际应该从BlockDefinition获取
    if (blockType >= BLOCK_TYPE_WHEAT_0 && blockType <= BLOCK_TYPE_WHEAT_7)
        return "Wheat";
    if (blockType >= BLOCK_TYPE_CARROTS_0 && blockType <= BLOCK_TYPE_CARROTS_3)
        return "Carrot";
    if (blockType >= BLOCK_TYPE_POTATOES_0 && blockType <= BLOCK_TYPE_POTATOES_3)
        return "Potato";
    
    return "Unknown";
}

Texture* FarmMonitorScreen::GetCropTexture(uint8_t blockType) const
{
    // 从BlockDefinition获取纹理
    // 这里简化处理
    const BlockDefinition& def = BlockDefinition::GetBlockDef(blockType);
    //return def.m_texture;  // 假设BlockDefinition有纹理成员
    return nullptr;
}