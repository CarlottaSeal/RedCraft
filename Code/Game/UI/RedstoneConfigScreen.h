#pragma once
#include "Engine/UI/UIScreen.h"
#include <vector>

#include "FarmMonitorScreen.h"
#include "Engine/Math/IntVec3.h"

//6方向选择器配置朝向
//中继器：延迟滑块 (1-4 ticks)、锁定状态
//比较器：比较/相减模式切换
//侦测器：观察方向显示
//活塞：类型显示、测试伸出按钮

enum Direction : uint8_t;
class Panel;
class Text;
class Button;
class Slider;
class Checkbox;
class Sprite;
class World;

// 红石组件配置数据
struct RedstoneComponentConfig
{
    IntVec3 m_worldPos;
    uint8_t m_blockType = 0;
    Direction m_facing = DIRECTION_NORTH;
    
    // 组件特定数据
    union
    {
        struct { int delay; bool locked; } repeater;
        struct { bool subtractMode; } comparator;
        struct { bool extended; bool sticky; } piston;
        struct { Direction watchDir; } observer;
    } m_data;
};

class RedstoneConfigScreen : public UIScreen
{
public:
    RedstoneConfigScreen(UISystem* uiSystem, World* world, const IntVec3& blockPos);
    virtual ~RedstoneConfigScreen() override;
    
    virtual void Build() override;
    virtual void Update(float deltaSeconds) override;
    virtual void OnEnter() override;
    virtual void OnExit() override;
    
private:
    World* m_world = nullptr;
    IntVec3 m_targetBlockPos;
    uint8_t m_targetBlockType = 0;
    
    // 通用 UI
    Panel* m_backgroundDimmer = nullptr;
    Panel* m_configPanel = nullptr;
    Text* m_titleText = nullptr;
    Text* m_blockTypeText = nullptr;
    Text* m_positionText = nullptr;
    Sprite* m_blockIcon = nullptr;
    
    // 方向配置
    Panel* m_directionPanel = nullptr;
    Text* m_directionLabel = nullptr;
    std::vector<Button*> m_directionButtons;  // 6个方向按钮
    Direction m_selectedDirection = DIRECTION_NORTH;
    
    // 中继器配置
    Panel* m_repeaterPanel = nullptr;
    Text* m_delayLabel = nullptr;
    Slider* m_delaySlider = nullptr;
    Text* m_delayValueText = nullptr;
    Checkbox* m_lockedCheckbox = nullptr;
    Text* m_lockedLabel = nullptr;
    
    // // 比较器配置
    // Panel* m_comparatorPanel = nullptr;
    // Button* m_compareModeButton = nullptr;
    // Button* m_subtractModeButton = nullptr;
    // Text* m_modeLabel = nullptr;
    
    // 侦测器配置
    Panel* m_observerPanel = nullptr;
    Text* m_watchDirLabel = nullptr;
    Text* m_observerInfoText = nullptr;
    
    // 活塞配置
    Panel* m_pistonPanel = nullptr;
    Text* m_pistonTypeLabel = nullptr;
    Text* m_pistonStateText = nullptr;
    Button* m_testExtendButton = nullptr;
    
    // 通用按钮
    Button* m_applyButton = nullptr;
    Button* m_cancelButton = nullptr;
    
    // 构建方法
    void BuildBackground();
    void BuildHeader();
    void BuildDirectionSelector();
    void BuildRepeaterConfig();
    void BuildComparatorConfig();
    void BuildObserverConfig();
    void BuildPistonConfig();
    void BuildButtons();
    
    // 显示/隐藏特定配置面板
    void ShowConfigPanelForBlockType(uint8_t blockType);
    void HideAllConfigPanels();
    
    // 数据加载/保存
    void LoadCurrentConfig();
    void ApplyConfig();
    
    // 事件处理
    void OnDirectionSelected(Direction dir);
    void OnDelayChanged(float value);
    void OnLockedChanged(bool locked);
    void OnCompareModeSelected();
    void OnSubtractModeSelected();
    void OnTestExtendClicked();
    void OnApplyClicked();
    void OnCancelClicked();
    
    // 辅助
    std::string GetDirectionName(Direction dir) const;
    std::string GetBlockTypeName(uint8_t blockType) const;
    Rgba8 GetDirectionButtonColor(Direction dir, bool selected) const;
};