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
#include "Game/Gameplay/RedstoneSimulator.h"

RedstoneConfigScreen::RedstoneConfigScreen(UISystem* uiSystem, World* world, const IntVec3& blockPos)
    : UIScreen(uiSystem, UIScreenType::CUSTOM, true)
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
    
    m_camera->SetOrthographicView(Vec2(0, 0), Vec2(1600, 900));
    
    BuildBackground();
    BuildHeader();
    BuildDirectionSelector();
    BuildRepeaterConfig();
    BuildComparatorConfig();
    BuildObserverConfig();
    BuildPistonConfig();
    BuildButtons();
    
    // 根据方块类型显示对应配置面板
    ShowConfigPanelForBlockType(m_targetBlockType);
    LoadCurrentConfig();
}

void RedstoneConfigScreen::BuildBackground()
{
    AABB2 screenBounds(0, 0, 1600, 900);
    m_backgroundDimmer = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(0, 0, 0, 200),
        nullptr,
        false,
        Rgba8::BLACK
    );
    m_elements.push_back(m_backgroundDimmer);
    
    // 配置面板 - 红石风格的暗红色调
    AABB2 configBounds(450, 150, 1150, 750);
    m_configPanel = new Panel(
        m_canvas,
        configBounds,
        Rgba8(50, 35, 35),
        nullptr,
        true,
        Rgba8(120, 60, 60)
    );
    m_elements.push_back(m_configPanel);
}

void RedstoneConfigScreen::BuildHeader()
{
    // 标题
    TextSetting titleSetting;
    titleSetting.m_text = "Redstone Component Config";
    titleSetting.m_color = Rgba8(255, 150, 150);
    titleSetting.m_height = 36.0f;
    
    Vec2 titlePos(480, 710);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
    m_elements.push_back(m_titleText);
    
    // 方块类型
    TextSetting typeSetting;
    typeSetting.m_text = "Type: " + GetBlockTypeName(m_targetBlockType);
    typeSetting.m_color = Rgba8(200, 200, 200);
    typeSetting.m_height = 22.0f;
    
    Vec2 typePos(480, 675);
    m_blockTypeText = new Text(m_canvas, typePos, typeSetting);
    m_elements.push_back(m_blockTypeText);
    
    // 位置信息
    TextSetting posSetting;
    std::string posStr = "Position: (" + std::to_string(m_targetBlockPos.x) 
                       + ", " + std::to_string(m_targetBlockPos.y)
                       + ", " + std::to_string(m_targetBlockPos.z) + ")";
    posSetting.m_text = posStr;
    posSetting.m_color = Rgba8(150, 150, 150);
    posSetting.m_height = 18.0f;
    
    Vec2 posTextPos(480, 650);
    m_positionText = new Text(m_canvas, posTextPos, posSetting);
    m_elements.push_back(m_positionText);
    
    // 方块图标
    AABB2 iconBounds(1050, 660, 1120, 730);
    m_blockIcon = new Sprite(m_canvas, iconBounds, nullptr);
    // TODO: 设置正确的纹理
    m_elements.push_back(m_blockIcon);
}

void RedstoneConfigScreen::BuildDirectionSelector()
{
    // 方向选择面板
    AABB2 dirPanelBounds(480, 480, 780, 630);
    m_directionPanel = new Panel(
        m_canvas,
        dirPanelBounds,
        Rgba8(40, 30, 30),
        nullptr,
        true,
        Rgba8(80, 50, 50)
    );
    m_elements.push_back(m_directionPanel);
    
    // 方向标签
    TextSetting labelSetting;
    labelSetting.m_text = "Facing Direction";
    labelSetting.m_color = Rgba8(200, 180, 180);
    labelSetting.m_height = 20.0f;
    
    Vec2 labelPos(500, 605);
    m_directionLabel = new Text(m_canvas, labelPos, labelSetting);
    m_elements.push_back(m_directionLabel);
    
    // 方向按钮布局 (十字形)
    //        [UP]
    //   [W] [N] [E]
    //        [S]
    //       [DOWN]
    
    float btnSize = 40.0f;
    float centerX = 630.0f;
    float centerY = 545.0f;
    float spacing = 45.0f;
    
    struct DirButton
    {
        Direction dir;
        float offsetX, offsetY;
        std::string label;
    };
    
    DirButton buttons[] = {
        { DIRECTION_UP,    0,  spacing * 1.5f, "U" },
        { DIRECTION_NORTH, 0,  spacing * 0.5f, "N" },
        { DIRECTION_WEST,  -spacing, 0,        "W" },
        { DIRECTION_EAST,  spacing,  0,        "E" },
        { DIRECTION_SOUTH, 0, -spacing * 0.5f, "S" },
        { DIRECTION_DOWN,  0, -spacing * 1.5f, "D" }
    };
    
    for (const DirButton& db : buttons)
    {
        float x = centerX + db.offsetX - btnSize * 0.5f;
        float y = centerY + db.offsetY - btnSize * 0.5f;
        
        AABB2 btnBounds(x, y, x + btnSize, y + btnSize);
        
        bool isSelected = (db.dir == m_selectedDirection);
        Rgba8 btnColor = GetDirectionButtonColor(db.dir, isSelected);
        
        Button* btn = new Button(
            nullptr,
            btnBounds,
            btnColor,
            Rgba8::WHITE,
            "SelectDir_" + std::to_string((int)db.dir),
            db.label,
            Vec2(0.5f, 0.5f)
        );
        
        m_canvas->AddElementToCanvas(btn);
        m_directionButtons.push_back(btn);
        m_elements.push_back(btn);
    }
}

void RedstoneConfigScreen::BuildRepeaterConfig()
{
    // 中继器配置面板
    AABB2 repeaterBounds(800, 480, 1120, 630);
    m_repeaterPanel = new Panel(
        m_canvas,
        repeaterBounds,
        Rgba8(40, 30, 30),
        nullptr,
        true,
        Rgba8(80, 50, 50)
    );
    m_repeaterPanel->SetEnabled(false);
    m_elements.push_back(m_repeaterPanel);
    
    // 延迟标签
    TextSetting delayLabelSetting;
    delayLabelSetting.m_text = "Delay (ticks)";
    delayLabelSetting.m_color = Rgba8(200, 180, 180);
    delayLabelSetting.m_height = 18.0f;
    
    Vec2 delayLabelPos(820, 605);
    m_delayLabel = new Text(m_canvas, delayLabelPos, delayLabelSetting);
    m_delayLabel->SetEnabled(false);
    m_elements.push_back(m_delayLabel);
    
    // 延迟滑块
    AABB2 sliderBounds(820, 560, 1050, 580);
    m_delaySlider = new Slider(
        m_canvas,
        sliderBounds,
        1.0f, 4.0f, 1.0f,  // 1-4 ticks
        Rgba8(60, 40, 40),
        Rgba8(120, 80, 80),
        Rgba8(200, 100, 100)
    );
    m_delaySlider->SetEnabled(false);
    m_elements.push_back(m_delaySlider);
    
    // 延迟值显示
    TextSetting delayValueSetting;
    delayValueSetting.m_text = "1";
    delayValueSetting.m_color = Rgba8(255, 200, 200);
    delayValueSetting.m_height = 24.0f;
    
    Vec2 delayValuePos(1070, 560);
    m_delayValueText = new Text(m_canvas, delayValuePos, delayValueSetting);
    m_delayValueText->SetEnabled(false);
    m_elements.push_back(m_delayValueText);
    
    // 锁定复选框
    AABB2 lockedCheckBounds(820, 510, 850, 540);
    m_lockedCheckbox = new Checkbox(
        m_canvas,
        lockedCheckBounds,
        false,
        Rgba8(60, 40, 40),
        Rgba8(200, 100, 100),
        Rgba8(100, 60, 60)
    );
    m_lockedCheckbox->SetEnabled(false);
    m_elements.push_back(m_lockedCheckbox);
    
    // 锁定标签
    TextSetting lockedLabelSetting;
    lockedLabelSetting.m_text = "Locked";
    lockedLabelSetting.m_color = Rgba8(180, 160, 160);
    lockedLabelSetting.m_height = 16.0f;
    
    Vec2 lockedLabelPos(860, 515);
    m_lockedLabel = new Text(m_canvas, lockedLabelPos, lockedLabelSetting);
    m_lockedLabel->SetEnabled(false);
    m_elements.push_back(m_lockedLabel);
}

void RedstoneConfigScreen::BuildComparatorConfig()
{
    // // 比较器配置面板
    // AABB2 comparatorBounds(800, 480, 1120, 630);
    // m_comparatorPanel = new Panel(
    //     m_canvas,
    //     comparatorBounds,
    //     Rgba8(40, 30, 30),
    //     nullptr,
    //     true,
    //     Rgba8(80, 50, 50)
    // );
    // m_comparatorPanel->SetEnabled(false);
    // m_elements.push_back(m_comparatorPanel);
    
    // 模式标签
    // TextSetting modeLabelSetting;
    // modeLabelSetting.m_text = "Operation Mode";
    // modeLabelSetting.m_color = Rgba8(200, 180, 180);
    // modeLabelSetting.m_height = 18.0f;
    //
    // Vec2 modeLabelPos(820, 605);
    // m_modeLabel = new Text(m_canvas, modeLabelPos, modeLabelSetting);
    // m_modeLabel->SetEnabled(false);
    // m_elements.push_back(m_modeLabel);
    
    // // 比较模式按钮
    // AABB2 compareBtnBounds(820, 540, 960, 585);
    // m_compareModeButton = new Button(
    //     nullptr,
    //     compareBtnBounds,
    //     Rgba8(100, 150, 100),
    //     Rgba8::WHITE,
    //     "ComparatorCompare",
    //     "Compare",
    //     Vec2(0.5f, 0.5f)
    // );
    // m_compareModeButton->SetEnabled(false);
    // m_canvas->AddElementToCanvas(m_compareModeButton);
    // m_elements.push_back(m_compareModeButton);
    
    // 相减模式按钮
    // AABB2 subtractBtnBounds(970, 540, 1110, 585);
    // m_subtractModeButton = new Button(
    //     nullptr,
    //     subtractBtnBounds,
    //     Rgba8(80, 80, 80),
    //     Rgba8::WHITE,
    //     "ComparatorSubtract",
    //     "Subtract",
    //     Vec2(0.5f, 0.5f)
    // );
    // m_subtractModeButton->SetEnabled(false);
    // m_canvas->AddElementToCanvas(m_subtractModeButton);
    // m_elements.push_back(m_subtractModeButton);
}

void RedstoneConfigScreen::BuildObserverConfig()
{
    // 侦测器配置面板
    AABB2 observerBounds(800, 480, 1120, 630);
    m_observerPanel = new Panel(
        m_canvas,
        observerBounds,
        Rgba8(40, 30, 30),
        nullptr,
        true,
        Rgba8(80, 50, 50)
    );
    m_observerPanel->SetEnabled(false);
    m_elements.push_back(m_observerPanel);
    
    // 观察方向标签
    TextSetting watchLabelSetting;
    watchLabelSetting.m_text = "Watching Direction";
    watchLabelSetting.m_color = Rgba8(200, 180, 180);
    watchLabelSetting.m_height = 18.0f;
    
    Vec2 watchLabelPos(820, 605);
    m_watchDirLabel = new Text(m_canvas, watchLabelPos, watchLabelSetting);
    m_watchDirLabel->SetEnabled(false);
    m_elements.push_back(m_watchDirLabel);
    
    // 侦测器信息
    TextSetting infoSetting;
    infoSetting.m_text = "Observer detects block changes\nin the direction it faces.\nOutput signal on the back.";
    infoSetting.m_color = Rgba8(150, 150, 150);
    infoSetting.m_height = 14.0f;
    
    Vec2 infoPos(820, 570);
    m_observerInfoText = new Text(m_canvas, infoPos, infoSetting);
    m_observerInfoText->SetEnabled(false);
    m_elements.push_back(m_observerInfoText);
}

void RedstoneConfigScreen::BuildPistonConfig()
{
    // 活塞配置面板
    AABB2 pistonBounds(800, 480, 1120, 630);
    m_pistonPanel = new Panel(
        m_canvas,
        pistonBounds,
        Rgba8(40, 30, 30),
        nullptr,
        true,
        Rgba8(80, 50, 50)
    );
    m_pistonPanel->SetEnabled(false);
    m_elements.push_back(m_pistonPanel);
    
    // 活塞类型标签
    TextSetting typeLabelSetting;
    typeLabelSetting.m_text = "Piston Type: Normal";
    typeLabelSetting.m_color = Rgba8(200, 180, 180);
    typeLabelSetting.m_height = 18.0f;
    
    Vec2 typeLabelPos(820, 605);
    m_pistonTypeLabel = new Text(m_canvas, typeLabelPos, typeLabelSetting);
    m_pistonTypeLabel->SetEnabled(false);
    m_elements.push_back(m_pistonTypeLabel);
    
    // 活塞状态
    TextSetting stateSetting;
    stateSetting.m_text = "State: Retracted";
    stateSetting.m_color = Rgba8(150, 200, 150);
    stateSetting.m_height = 16.0f;
    
    Vec2 statePos(820, 575);
    m_pistonStateText = new Text(m_canvas, statePos, stateSetting);
    m_pistonStateText->SetEnabled(false);
    m_elements.push_back(m_pistonStateText);
    
    // 测试伸出按钮
    AABB2 testBtnBounds(820, 510, 1000, 555);
    m_testExtendButton = new Button(
        nullptr,
        testBtnBounds,
        Rgba8(100, 120, 150),
        Rgba8::WHITE,
        "PistonTestExtend",
        "Test Extend",
        Vec2(0.5f, 0.5f)
    );
    m_testExtendButton->SetEnabled(false);
    m_canvas->AddElementToCanvas(m_testExtendButton);
    m_elements.push_back(m_testExtendButton);
}

void RedstoneConfigScreen::BuildButtons()
{
    float buttonWidth = 150.0f;
    float buttonHeight = 45.0f;
    float spacing = 20.0f;
    float bottomY = 180.0f;
    
    // 应用按钮
    AABB2 applyBounds(700, bottomY, 700 + buttonWidth, bottomY + buttonHeight);
    m_applyButton = new Button(
        nullptr,
        applyBounds,
        Rgba8(80, 150, 80),
        Rgba8::WHITE,
        "RedstoneConfigApply",
        "Apply",
        Vec2(0.5f, 0.5f)
    );
    m_canvas->AddElementToCanvas(m_applyButton);
    m_elements.push_back(m_applyButton);
    
    // 取消按钮
    AABB2 cancelBounds(700 + buttonWidth + spacing, bottomY, 
                       700 + buttonWidth * 2 + spacing, bottomY + buttonHeight);
    m_cancelButton = new Button(
        nullptr,
        cancelBounds,
        Rgba8(150, 80, 80),
        Rgba8::WHITE,
        "RedstoneConfigCancel",
        "Cancel",
        Vec2(0.5f, 0.5f)
    );
    m_canvas->AddElementToCanvas(m_cancelButton);
    m_elements.push_back(m_cancelButton);
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
    else if (blockType == BLOCK_TYPE_OBSERVER)
    {
        if (m_observerPanel) m_observerPanel->SetEnabled(true);
        if (m_watchDirLabel) m_watchDirLabel->SetEnabled(true);
        if (m_observerInfoText) m_observerInfoText->SetEnabled(true);
    }
    else if (blockType == BLOCK_TYPE_PISTON || blockType == BLOCK_TYPE_STICKY_PISTON)
    {
        if (m_pistonPanel) m_pistonPanel->SetEnabled(true);
        if (m_pistonTypeLabel) m_pistonTypeLabel->SetEnabled(true);
        if (m_pistonStateText) m_pistonStateText->SetEnabled(true);
        if (m_testExtendButton) m_testExtendButton->SetEnabled(true);
        
        // 更新活塞类型标签
        if (m_pistonTypeLabel)
        {
            std::string typeStr = (blockType == BLOCK_TYPE_STICKY_PISTON) 
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
    else if (m_targetBlockType == BLOCK_TYPE_PISTON || m_targetBlockType == BLOCK_TYPE_STICKY_PISTON)
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
    case BLOCK_TYPE_OBSERVER:
        return "Observer";
    case BLOCK_TYPE_PISTON:
        return "Piston";
    case BLOCK_TYPE_STICKY_PISTON:
        return "Sticky Piston";
    case BLOCK_TYPE_LEVER:
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