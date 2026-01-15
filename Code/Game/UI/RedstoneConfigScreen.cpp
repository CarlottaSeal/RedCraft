#include "RedstoneConfigScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/Button.h"
#include "Engine/UI/Slider.h"
#include "Engine/UI/Checkbox.h"
#include "Engine/UI/Sprite.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/World.h"
#include "Game/Block.h"
#include "Game/BlockDefinition.h"
#include "Game/Game.hpp"
#include "Game/Gameplay/RedstoneSimulator.h"
extern Game* g_theGame;
RedstoneConfigScreen::RedstoneConfigScreen(UISystem* uiSystem, World* world, const IntVec3& blockPos)
    : UIScreen(uiSystem, UIScreenType::CUSTOM, g_theGame->m_screenCamera, true)
    , m_world(world)
    , m_targetBlockPos(blockPos)
{
    if (m_world)
    {
        Block block = m_world->GetBlockAtWorldCoords(blockPos.x, blockPos.y, blockPos.z);
        m_targetBlockType = block.m_typeIndex;
        m_selectedDirection = (Direction)block.GetBlockFacing();
    }
}

RedstoneConfigScreen::~RedstoneConfigScreen()
{
}
void RedstoneConfigScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildBackground();
    BuildHeader();
    BuildDirectionSelector();
    BuildRepeaterConfig();
    //BuildComparatorConfig();
    BuildObserverConfig();
    BuildPistonConfig();
    BuildButtons();
    
    ShowConfigPanelForBlockType(m_targetBlockType);
    LoadCurrentConfig();
}

void RedstoneConfigScreen::BuildBackground()
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
        Rgba8::BLACK,
        AABB2(0, 0, 0, 0)  // UV
    );
    
    // 配置面板 - 60% 宽度 × 70% 高度，居中
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    AABB2 configBounds(
        panelX,
        panelY,
        panelX + panelWidth,
        panelY + panelHeight
    );
    
    m_configPanel = new Panel(
        m_canvas,
        configBounds,
        Rgba8(45, 35, 35),
        nullptr,
        true,
        Rgba8(140, 70, 70),
        AABB2(0, 0, 0, 0)  // UV
    );
}

void RedstoneConfigScreen::BuildHeader()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    // 标题
    TextSetting titleSetting;
    titleSetting.m_text = "Redstone Component Config";
    titleSetting.m_color = Rgba8(255, 140, 140);
    titleSetting.m_height = size.y * 0.04f;
    titleSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 titlePos(panelX + panelWidth * 0.05f, panelY + panelHeight * 0.93f);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
    
    // 方块类型
    TextSetting typeSetting;
    typeSetting.m_text = "Type: " + GetBlockTypeName(m_targetBlockType);
    typeSetting.m_color = Rgba8(220, 220, 220);
    typeSetting.m_height = size.y * 0.025f;
    typeSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 typePos(panelX + panelWidth * 0.05f, panelY + panelHeight * 0.86f);
    m_blockTypeText = new Text(m_canvas, typePos, typeSetting);
    
    // 位置信息
    TextSetting posSetting;
    posSetting.m_text = Stringf("Position: (%d, %d, %d)", 
        m_targetBlockPos.x, m_targetBlockPos.y, m_targetBlockPos.z);
    posSetting.m_color = Rgba8(180, 180, 180);
    posSetting.m_height = size.y * 0.020f;
    posSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 posTextPos(panelX + panelWidth * 0.05f, panelY + panelHeight * 0.81f);
    m_positionText = new Text(m_canvas, posTextPos, posSetting);
    
    // 方块图标
    float iconSize = size.y * 0.08f;
    AABB2 iconBounds(
        panelX + panelWidth * 0.88f,
        panelY + panelHeight * 0.81f,
        panelX + panelWidth * 0.88f + iconSize,
        panelY + panelHeight * 0.81f + iconSize
    );
    m_blockIcon = new Sprite(m_canvas, iconBounds, nullptr);
}

void RedstoneConfigScreen::BuildDirectionSelector()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    // 方向选择面板
    float dirPanelWidth = panelWidth * 0.40f;
    float dirPanelHeight = panelHeight * 0.35f;
    float dirPanelX = panelX + panelWidth * 0.05f;
    float dirPanelY = panelY + panelHeight * 0.38f;
    
    AABB2 dirPanelBounds(
        dirPanelX,
        dirPanelY,
        dirPanelX + dirPanelWidth,
        dirPanelY + dirPanelHeight
    );
    
    m_directionPanel = new Panel(
        m_canvas,
        dirPanelBounds,
        Rgba8(38, 28, 28),
        nullptr,
        true,
        Rgba8(90, 55, 55),
        AABB2(0, 0, 0, 0)  // UV
    );
    
    // 标签
    TextSetting labelSetting;
    labelSetting.m_text = "Facing Direction";
    labelSetting.m_color = Rgba8(220, 200, 200);
    labelSetting.m_height = size.y * 0.022f;
    labelSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 labelPos(dirPanelX + dirPanelWidth * 0.1f, dirPanelY + dirPanelHeight * 0.85f);
    m_directionLabel = new Text(m_canvas, labelPos, labelSetting);
    
    // 方向按钮 - 十字布局
    float btnSize = size.x * 0.028f;
    float centerX = dirPanelX + dirPanelWidth * 0.5f;
    float centerY = dirPanelY + dirPanelHeight * 0.42f;
    float spacing = size.x * 0.032f;
    
    struct DirButton
    {
        Direction dir;
        float offsetX, offsetY;
        std::string label;
    };
    
    DirButton buttons[] = {
        { DIRECTION_UP,    0,           spacing * 1.5f, "U" },
        { DIRECTION_NORTH, 0,           spacing * 0.5f, "N" },
        { DIRECTION_WEST,  -spacing,    0,              "W" },
        { DIRECTION_EAST,  spacing,     0,              "E" },
        { DIRECTION_SOUTH, 0,          -spacing * 0.5f, "S" },
        { DIRECTION_DOWN,  0,          -spacing * 1.5f, "D" }
    };
    
    m_directionButtons.clear();
    
    for (const DirButton& db : buttons)
    {
        float x = centerX + db.offsetX - btnSize * 0.5f;
        float y = centerY + db.offsetY - btnSize * 0.5f;
        
        AABB2 btnBounds(x, y, x + btnSize, y + btnSize);
        
        bool isSelected = (db.dir == m_selectedDirection);
        Rgba8 btnColor = GetDirectionButtonColor(db.dir, isSelected);
        
        Button* btn = new Button(
            m_canvas,
            btnBounds,
            btnColor,
            Rgba8(100, 100, 100),
            Rgba8::WHITE,
            db.label,
            Vec2(0.5f, 0.5f),
            "",
            AABB2(0,0,0,0)
        );
        m_directionButtons.push_back(btn);
    }
}

void RedstoneConfigScreen::BuildRepeaterConfig()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    float repPanelWidth = panelWidth * 0.50f;
    float repPanelHeight = panelHeight * 0.35f;
    float repPanelX = panelX + panelWidth * 0.48f;
    float repPanelY = panelY + panelHeight * 0.38f;
    
    AABB2 repeaterBounds(
        repPanelX,
        repPanelY,
        repPanelX + repPanelWidth,
        repPanelY + repPanelHeight
    );
    
    m_repeaterPanel = new Panel(
        m_canvas,
        repeaterBounds,
        Rgba8(38, 28, 28),
        nullptr,
        true,
        Rgba8(90, 55, 55),
        AABB2(0, 0, 0, 0)  // UV
    );
    m_repeaterPanel->SetEnabled(false);
    
    // 延迟标签
    TextSetting delayLabelSetting;
    delayLabelSetting.m_text = "Delay (ticks)";
    delayLabelSetting.m_color = Rgba8(220, 200, 200);
    delayLabelSetting.m_height = size.y * 0.020f;
    delayLabelSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 delayLabelPos(repPanelX + repPanelWidth * 0.1f, repPanelY + repPanelHeight * 0.80f);
    m_delayLabel = new Text(m_canvas, delayLabelPos, delayLabelSetting);
    m_delayLabel->SetEnabled(false);
    
    // 延迟滑块
    float sliderWidth = repPanelWidth * 0.70f;
    float sliderHeight = size.y * 0.022f;
    AABB2 sliderBounds(
        repPanelX + repPanelWidth * 0.1f,
        repPanelY + repPanelHeight * 0.60f,
        repPanelX + repPanelWidth * 0.1f + sliderWidth,
        repPanelY + repPanelHeight * 0.60f + sliderHeight
    );
    
    m_delaySlider = new Slider(
        m_canvas,
        sliderBounds,
        1.0f, 4.0f, 1.0f,
        Rgba8(60, 40, 40),
        Rgba8(120, 80, 80),
        Rgba8(200, 100, 100)
    );
    m_delaySlider->SetEnabled(false);
    
    // 延迟值显示
    TextSetting delayValueSetting;
    delayValueSetting.m_text = "1";
    delayValueSetting.m_color = Rgba8(255, 180, 180);
    delayValueSetting.m_height = size.y * 0.028f;
    delayValueSetting.m_alignment = Vec2(0.5f, 0.5f);
    
    Vec2 delayValuePos(repPanelX + repPanelWidth * 0.88f, repPanelY + repPanelHeight * 0.61f);
    m_delayValueText = new Text(m_canvas, delayValuePos, delayValueSetting);
    m_delayValueText->SetEnabled(false);
    
    // 锁定复选框
    float checkSize = size.y * 0.033f;
    AABB2 lockedCheckBounds(
        repPanelX + repPanelWidth * 0.1f,
        repPanelY + repPanelHeight * 0.25f,
        repPanelX + repPanelWidth * 0.1f + checkSize,
        repPanelY + repPanelHeight * 0.25f + checkSize
    );
    
    m_lockedCheckbox = new Checkbox(
        m_canvas,
        lockedCheckBounds,
        false,
        Rgba8(60, 40, 40),
        Rgba8(200, 100, 100),
        Rgba8(100, 60, 60)
    );
    m_lockedCheckbox->SetEnabled(false);
    
    // 锁定标签
    TextSetting lockedLabelSetting;
    lockedLabelSetting.m_text = "Locked";
    lockedLabelSetting.m_color = Rgba8(200, 180, 180);
    lockedLabelSetting.m_height = size.y * 0.018f;
    lockedLabelSetting.m_alignment = Vec2(0.0f, 0.5f);
    
    Vec2 lockedLabelPos(
        repPanelX + repPanelWidth * 0.1f + checkSize * 1.3f,
        repPanelY + repPanelHeight * 0.265f
    );
    m_lockedLabel = new Text(m_canvas, lockedLabelPos, lockedLabelSetting);
    m_lockedLabel->SetEnabled(false);
}

void RedstoneConfigScreen::BuildComparatorConfig()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    // 比较器面板
    float compPanelWidth = panelWidth * 0.50f;
    float compPanelHeight = panelHeight * 0.35f;
    float compPanelX = panelX + panelWidth * 0.48f;
    float compPanelY = panelY + panelHeight * 0.38f;
    
    AABB2 comparatorBounds(
        compPanelX,
        compPanelY,
        compPanelX + compPanelWidth,
        compPanelY + compPanelHeight
    );
    
    // m_comparatorPanel = new Panel(
    //     m_canvas,
    //     comparatorBounds,
    //     Rgba8(38, 28, 28),
    //     nullptr,
    //     true,
    //     Rgba8(90, 55, 55),
    //     AABB2(0, 0, 0, 0)  // UV
    // );
    // m_comparatorPanel->SetEnabled(false);
    
    // 其他组件...
}

void RedstoneConfigScreen::BuildObserverConfig()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    // Observer面板
    float obsPanelWidth = panelWidth * 0.50f;
    float obsPanelHeight = panelHeight * 0.35f;
    float obsPanelX = panelX + panelWidth * 0.48f;
    float obsPanelY = panelY + panelHeight * 0.38f;
    
    AABB2 observerBounds(
        obsPanelX,
        obsPanelY,
        obsPanelX + obsPanelWidth,
        obsPanelY + obsPanelHeight
    );
    
    m_observerPanel = new Panel(
        m_canvas,
        observerBounds,
        Rgba8(38, 28, 28),
        nullptr,
        true,
        Rgba8(90, 55, 55),
        AABB2(0, 0, 0, 0)  // UV
    );
    m_observerPanel->SetEnabled(false);
    
    // 观察方向标签
    TextSetting observingLabelSetting;
    observingLabelSetting.m_text = "Observing Direction:";
    observingLabelSetting.m_color = Rgba8(220, 200, 200);
    observingLabelSetting.m_height = size.y * 0.020f;
    observingLabelSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 obsLabelPos(obsPanelX + obsPanelWidth * 0.1f, obsPanelY + obsPanelHeight * 0.70f);
    m_observerDirectionLabel = new Text(m_canvas, obsLabelPos, observingLabelSetting);
    m_observerDirectionLabel->SetEnabled(false);
    
    // 方向值显示
    TextSetting dirValueSetting;
    dirValueSetting.m_text = "North";
    dirValueSetting.m_color = Rgba8(255, 180, 180);
    dirValueSetting.m_height = size.y * 0.024f;
    dirValueSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 dirValuePos(obsPanelX + obsPanelWidth * 0.1f, obsPanelY + obsPanelHeight * 0.55f);
    m_observerDirectionText = new Text(m_canvas, dirValuePos, dirValueSetting);
    m_observerDirectionText->SetEnabled(false);
    
    // 状态文本
    TextSetting statusSetting;
    statusSetting.m_text = "Status: Idle";
    statusSetting.m_color = Rgba8(180, 180, 180);
    statusSetting.m_height = size.y * 0.018f;
    statusSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 statusPos(obsPanelX + obsPanelWidth * 0.1f, obsPanelY + obsPanelHeight * 0.35f);
    m_observerStatusText = new Text(m_canvas, statusPos, statusSetting);
    m_observerStatusText->SetEnabled(false);
}

void RedstoneConfigScreen::BuildPistonConfig()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    // Piston面板
    float pistPanelWidth = panelWidth * 0.50f;
    float pistPanelHeight = panelHeight * 0.35f;
    float pistPanelX = panelX + panelWidth * 0.48f;
    float pistPanelY = panelY + panelHeight * 0.38f;
    
    AABB2 pistonBounds(
        pistPanelX,
        pistPanelY,
        pistPanelX + pistPanelWidth,
        pistPanelY + pistPanelHeight
    );
    
    m_pistonPanel = new Panel(
        m_canvas,
        pistonBounds,
        Rgba8(38, 28, 28),
        nullptr,
        true,
        Rgba8(90, 55, 55),
        AABB2(0, 0, 0, 0)  // UV
    );
    m_pistonPanel->SetEnabled(false);
    
    // 活塞类型
    TextSetting typeSetting;
    typeSetting.m_text = "Type: Piston";
    typeSetting.m_color = Rgba8(220, 200, 200);
    typeSetting.m_height = size.y * 0.020f;
    typeSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 typePos(pistPanelX + pistPanelWidth * 0.1f, pistPanelY + pistPanelHeight * 0.75f);
    m_pistonTypeText = new Text(m_canvas, typePos, typeSetting);
    m_pistonTypeText->SetEnabled(false);
    
    // 伸出状态
    TextSetting stateSetting;
    stateSetting.m_text = "State: Retracted";
    stateSetting.m_color = Rgba8(180, 180, 180);
    stateSetting.m_height = size.y * 0.018f;
    stateSetting.m_alignment = Vec2(0.0f, 1.0f);
    
    Vec2 statePos(pistPanelX + pistPanelWidth * 0.1f, pistPanelY + pistPanelHeight * 0.60f);
    m_pistonExtendedText = new Text(m_canvas, statePos, stateSetting);
    m_pistonExtendedText->SetEnabled(false);
    
    // 测试伸出按钮
    float btnWidth = pistPanelWidth * 0.72f;
    float btnHeight = size.y * 0.036f;
    
    AABB2 testBtnBounds(
        pistPanelX + pistPanelWidth * 0.14f,
        pistPanelY + pistPanelHeight * 0.25f,
        pistPanelX + pistPanelWidth * 0.14f + btnWidth,
        pistPanelY + pistPanelHeight * 0.25f + btnHeight
    );
    
    m_pistonTestButton = new Button(
        m_canvas,
        testBtnBounds,
        Rgba8(70, 100, 120),
        Rgba8(100, 140, 170),
        Rgba8::WHITE,
        "Test Extend",
        Vec2(0.5f, 0.5f),
        "",
        AABB2(0, 0, 0, 0) 
    );
    m_pistonTestButton->SetEnabled(false);
}

void RedstoneConfigScreen::BuildButtons()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    
    float panelWidth = size.x * 0.60f;
    float panelHeight = size.y * 0.70f;
    float panelX = (size.x - panelWidth) * 0.5f;
    float panelY = (size.y - panelHeight) * 0.5f;
    
    float btnWidth = size.x * 0.09f;
    float btnHeight = size.y * 0.044f;
    float spacing = size.x * 0.02f;
    
    float totalWidth = 2 * btnWidth + spacing;
    float startX = panelX + (panelWidth - totalWidth) * 0.5f;
    float btnY = panelY + panelHeight * 0.08f;
    
    // Apply 按钮
    AABB2 applyBounds(
        startX,
        btnY,
        startX + btnWidth,
        btnY + btnHeight
    );
    
    m_applyButton = new Button(
        m_canvas,
        applyBounds,
        Rgba8(60, 120, 60),
        Rgba8(80, 160, 80),
        Rgba8::WHITE,
        "Apply",
        Vec2(0.5f, 0.5f),
        "",
        AABB2(0, 0, 0, 0)  // UV
    );
    
    // Cancel 按钮
    AABB2 cancelBounds(
        startX + btnWidth + spacing,
        btnY,
        startX + btnWidth + spacing + btnWidth,
        btnY + btnHeight
    );
    
    m_cancelButton = new Button(
        m_canvas,
        cancelBounds,
        Rgba8(120, 50, 50),
        Rgba8(160, 70, 70),
        Rgba8::WHITE,
        "Cancel",
        Vec2(0.5f, 0.5f),
        "",
        AABB2(0, 0, 0, 0)  // UV
    );
}

void RedstoneConfigScreen::ShowConfigPanelForBlockType(uint8_t blockType)
{
    HideAllConfigPanels();
    
    // 方向选择器对所有红石组件都显示
    if (m_directionPanel) m_directionPanel->SetEnabled(true);
    if (m_directionLabel) m_directionLabel->SetEnabled(true);
    for (Button* btn : m_directionButtons)
    {
        if (btn) btn->SetEnabled(true);
    }
    
    // 根据类型显示特定配置
    if (blockType == BLOCK_TYPE_REPEATER_OFF || blockType == BLOCK_TYPE_REPEATER_ON)
    {
        if (m_repeaterPanel) m_repeaterPanel->SetEnabled(true);
        if (m_delayLabel) m_delayLabel->SetEnabled(true);
        if (m_delaySlider) m_delaySlider->SetEnabled(true);
        if (m_delayValueText) m_delayValueText->SetEnabled(true);
        if (m_lockedCheckbox) m_lockedCheckbox->SetEnabled(true);
        if (m_lockedLabel) m_lockedLabel->SetEnabled(true);
    }
    // else if (blockType == BLOCK_TYPE_COMPARATOR || blockType == BLOCK_TYPE_COMPARATOR_ON)
    // {
    //     if (m_comparatorPanel) m_comparatorPanel->SetEnabled(true);
    //     if (m_modeLabel) m_modeLabel->SetEnabled(true);
    //     if (m_compareModeButton) m_compareModeButton->SetEnabled(true);
    //     if (m_subtractModeButton) m_subtractModeButton->SetEnabled(true);
    // }
    else if (blockType == BLOCK_TYPE_REDSTONE_OBSERVER)
    {
        if (m_observerPanel) m_observerPanel->SetEnabled(true);
        if (m_watchDirLabel) m_watchDirLabel->SetEnabled(true);
        if (m_observerInfoText) m_observerInfoText->SetEnabled(true);
    }
    else if (blockType == BLOCK_TYPE_REDSTONE_PISTON || blockType == BLOCK_TYPE_REDSTONE_STICKY_PISTON)
    {
        if (m_pistonPanel) m_pistonPanel->SetEnabled(true);
        if (m_pistonTypeLabel) m_pistonTypeLabel->SetEnabled(true);
        if (m_pistonStateText) m_pistonStateText->SetEnabled(true);
        if (m_testExtendButton) m_testExtendButton->SetEnabled(true);
        
        // 更新活塞类型标签
        if (m_pistonTypeLabel)
        {
            std::string typeStr = (blockType == BLOCK_TYPE_REDSTONE_STICKY_PISTON) 
                ? "Piston Type: Sticky" 
                : "Piston Type: Normal";
            m_pistonTypeLabel->SetText(typeStr);
        }
    }
}

void RedstoneConfigScreen::HideAllConfigPanels()
{
    // 中继器
    if (m_repeaterPanel) m_repeaterPanel->SetEnabled(false);
    if (m_delayLabel) m_delayLabel->SetEnabled(false);
    if (m_delaySlider) m_delaySlider->SetEnabled(false);
    if (m_delayValueText) m_delayValueText->SetEnabled(false);
    if (m_lockedCheckbox) m_lockedCheckbox->SetEnabled(false);
    if (m_lockedLabel) m_lockedLabel->SetEnabled(false);
    
    // // 比较器
    // if (m_comparatorPanel) m_comparatorPanel->SetEnabled(false);
    // if (m_modeLabel) m_modeLabel->SetEnabled(false);
    // if (m_compareModeButton) m_compareModeButton->SetEnabled(false);
    // if (m_subtractModeButton) m_subtractModeButton->SetEnabled(false);
    
    // 侦测器
    if (m_observerPanel) m_observerPanel->SetEnabled(false);
    if (m_watchDirLabel) m_watchDirLabel->SetEnabled(false);
    if (m_observerInfoText) m_observerInfoText->SetEnabled(false);
    
    // 活塞
    if (m_pistonPanel) m_pistonPanel->SetEnabled(false);
    if (m_pistonTypeLabel) m_pistonTypeLabel->SetEnabled(false);
    if (m_pistonStateText) m_pistonStateText->SetEnabled(false);
    if (m_testExtendButton) m_testExtendButton->SetEnabled(false);
}

void RedstoneConfigScreen::LoadCurrentConfig()
{
    if (!m_world)
        return;
    
    Block block = m_world->GetBlockAtWorldCoords(
        m_targetBlockPos.x, m_targetBlockPos.y, m_targetBlockPos.z);
    
    m_selectedDirection = (Direction)block.GetBlockFacing();
    
    // 更新方向按钮高亮
    for (int i = 0; i < (int)m_directionButtons.size(); i++)
    {
        Direction dir = (Direction)i;
        bool isSelected = (dir == m_selectedDirection);
        if (m_directionButtons[i])
        {
            m_directionButtons[i]->SetBackgroundColor(GetDirectionButtonColor(dir, isSelected));
        }
    }
    
    // 加载特定组件配置
    if (m_targetBlockType == BLOCK_TYPE_REPEATER_OFF || m_targetBlockType == BLOCK_TYPE_REPEATER_ON)
    {
        int delay = block.GetRepeaterDelay();
        if (m_delaySlider) m_delaySlider->SetValue((float)delay);
        if (m_delayValueText) m_delayValueText->SetText(std::to_string(delay));
        
        // bool locked = block.IsRepeaterLocked();
        // if (m_lockedCheckbox) m_lockedCheckbox->SetChecked(locked);
    }
    // else if (m_targetBlockType == BLOCK_TYPE_COMPARATOR || m_targetBlockType == BLOCK_TYPE_COMPARATOR_ON)
    // {
    //     bool subtractMode = (block.GetComparatorMode() == 1);
    //     if (subtractMode)
    //     {
    //         if (m_compareModeButton) m_compareModeButton->SetBackgroundColor(Rgba8(80, 80, 80));
    //         if (m_subtractModeButton) m_subtractModeButton->SetBackgroundColor(Rgba8(100, 150, 100));
    //     }
    //     else
    //     {
    //         if (m_compareModeButton) m_compareModeButton->SetBackgroundColor(Rgba8(100, 150, 100));
    //         if (m_subtractModeButton) m_subtractModeButton->SetBackgroundColor(Rgba8(80, 80, 80));
    //     }
    // }
    else if (m_targetBlockType == BLOCK_TYPE_REDSTONE_PISTON || m_targetBlockType == BLOCK_TYPE_REDSTONE_STICKY_PISTON)
    {
        bool extended = block.IsPistonExtended();
        if (m_pistonStateText)
        {
            m_pistonStateText->SetText(extended ? "State: Extended" : "State: Retracted");
            m_pistonStateText->SetColor(extended ? Rgba8(200, 150, 150) : Rgba8(150, 200, 150));
        }
    }
}

void RedstoneConfigScreen::ApplyConfig()
{
    if (!m_world)
        return;
    
    BlockIterator iter = m_world->GetBlockIterator(m_targetBlockPos);
    if (!iter.IsValid())
        return;
    
    Block* block = iter.GetBlock();
    if (!block)
        return;
    
    // 应用方向
    block->SetBlockFacing((uint8_t)m_selectedDirection);
    
    // 应用特定配置
    if (m_targetBlockType == BLOCK_TYPE_REPEATER_OFF || m_targetBlockType == BLOCK_TYPE_REPEATER_ON)
    {
        if (m_delaySlider)
        {
            int delay = (int)m_delaySlider->GetValue();
            block->SetRepeaterDelay(delay);
        }
        // if (m_lockedCheckbox)
        //     block->SetRepeaterLocked(m_lockedCheckbox->IsChecked());
    }
    // else if (m_targetBlockType == BLOCK_TYPE_COMPARATOR || m_targetBlockType == BLOCK_TYPE_COMPARATOR_ON)
    // {
    //     // 模式通过按钮点击已经设置
    // }
    
    // 标记区块为脏
    Chunk* chunk = iter.GetChunk();
    if (chunk)
    {
        chunk->MarkSelfDirty();
        m_world->m_hasDirtyChunk = true;
    }
    
    if (m_world->m_redstoneSimulator)
    {
        m_world->m_redstoneSimulator->OnBlockPlaced(iter);
    }
}

void RedstoneConfigScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    // 更新延迟显示
    if (m_delaySlider && m_delayValueText && m_delaySlider->IsEnabled())
    {
        int delay = (int)m_delaySlider->GetValue();
        m_delayValueText->SetText(std::to_string(delay));
    }
}

void RedstoneConfigScreen::Render() const
{
    g_theRenderer->BindTexture(nullptr);
    UIScreen::Render();
}

void RedstoneConfigScreen::OnEnter()
{
    UIScreen::OnEnter();
}

void RedstoneConfigScreen::OnExit()
{
    UIScreen::OnExit();
}

void RedstoneConfigScreen::OnDirectionSelected(Direction dir)
{
    m_selectedDirection = dir;
    
    // 更新按钮高亮
    for (int i = 0; i < (int)m_directionButtons.size(); i++)
    {
        Direction btnDir = (Direction)i;
        bool isSelected = (btnDir == m_selectedDirection);
        if (m_directionButtons[i])
        {
            m_directionButtons[i]->SetBackgroundColor(GetDirectionButtonColor(btnDir, isSelected));
        }
    }
}

void RedstoneConfigScreen::OnDelayChanged(float value)
{
    if (m_delayValueText)
    {
        m_delayValueText->SetText(std::to_string((int)value));
    }
}

void RedstoneConfigScreen::OnLockedChanged(bool locked)
{
    UNUSED(locked);
}

void RedstoneConfigScreen::OnCompareModeSelected()
{
    // if (m_compareModeButton) m_compareModeButton->SetBackgroundColor(Rgba8(100, 150, 100));
    // if (m_subtractModeButton) m_subtractModeButton->SetBackgroundColor(Rgba8(80, 80, 80));
    //
    // // 设置模式
    // if (m_world)
    // {
    //     BlockIterator iter = m_world->GetBlockIterator(m_targetBlockPos);
    //     if (iter.IsValid())
    //     {
    //         Block* block = iter.GetBlock();
    //         if (block) block->SetComparatorMode(0);
    //     }
    // }
}

void RedstoneConfigScreen::OnSubtractModeSelected()
{
    //if (m_compareModeButton) m_compareModeButton->SetBackgroundColor(Rgba8(80, 80, 80));
    //if (m_subtractModeButton) m_subtractModeButton->SetBackgroundColor(Rgba8(100, 150, 100));
    
    // // 设置模式
    // if (m_world)
    // {
    //     BlockIterator iter = m_world->GetBlockIterator(m_targetBlockPos);
    //     if (iter.IsValid())
    //     {
    //         Block* block = iter.GetBlock();
    //         if (block) block->SetComparatorMode(1);
    //     }
    // }
}

void RedstoneConfigScreen::OnTestExtendClicked()
{
    if (!m_world)
        return;
    
    if (!m_world->m_redstoneSimulator)
        return;
    
    BlockIterator iter = m_world->GetBlockIterator(m_targetBlockPos);
    if (iter.IsValid())
    {
        Block* block = iter.GetBlock();
        if (block)
        {
            bool isExtended = block->IsPistonExtended();
            if (isExtended)
            {
                m_world->m_redstoneSimulator->RetractPiston(iter);
            }
            else
            {
                m_world->m_redstoneSimulator->ExtendPiston(iter);
            }
            
            // 更新状态显示
            if (m_pistonStateText)
            {
                bool newState = !isExtended;
                m_pistonStateText->SetText(newState ? "State: Extended" : "State: Retracted");
                m_pistonStateText->SetColor(newState ? Rgba8(200, 150, 150) : Rgba8(150, 200, 150));
            }
        }
    }
}

void RedstoneConfigScreen::OnApplyClicked()
{
    ApplyConfig();
    // 关闭界面
    // g_theEventSystem->FireEvent("CloseRedstoneConfig");
}

void RedstoneConfigScreen::OnCancelClicked()
{
    // 关闭界面，不保存
    // g_theEventSystem->FireEvent("CloseRedstoneConfig");
}

std::string RedstoneConfigScreen::GetDirectionName(Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_NORTH: return "North (+Y)";
    case DIRECTION_SOUTH: return "South (-Y)";
    case DIRECTION_EAST:  return "East (+X)";
    case DIRECTION_WEST:  return "West (-X)";
    case DIRECTION_UP:    return "Up (+Z)";
    case DIRECTION_DOWN:  return "Down (-Z)";
    default:              return "Unknown";
    }
}

std::string RedstoneConfigScreen::GetBlockTypeName(uint8_t blockType) const
{
    switch (blockType)
    {
    case BLOCK_TYPE_REPEATER_OFF:
    case BLOCK_TYPE_REPEATER_ON:
        return "Redstone Repeater";
    case BLOCK_TYPE_REDSTONE_OBSERVER:
        return "Observer";
    case BLOCK_TYPE_REDSTONE_PISTON:
        return "Piston";
    case BLOCK_TYPE_REDSTONE_STICKY_PISTON:
        return "Sticky Piston";
    case BLOCK_TYPE_REDSTONE_LEVER:
        return "Lever";
    case BLOCK_TYPE_BUTTON_STONE:
        return "Stone Button";
    case BLOCK_TYPE_REDSTONE_TORCH:
        return "Redstone Torch";
    default:
        return "Unknown Component";
    }
}

Rgba8 RedstoneConfigScreen::GetDirectionButtonColor(Direction dir, bool selected) const
{
    UNUSED(dir);
    if (selected)
        return Rgba8(200, 100, 100);  // 选中 - 亮红
    else
        return Rgba8(80, 50, 50);     // 未选中 - 暗红
}