#include "SettingsScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Button.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/Slider.h"
#include "Engine/UI/Checkbox.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Game.hpp"
extern Game* g_theGame;
SettingsScreen::SettingsScreen(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::OPTIONS, g_theGame->m_screenCamera, true)
{
}

SettingsScreen::~SettingsScreen()
{
}

void SettingsScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildBackground();
    BuildTitle();
    BuildAudioSettings();
    BuildGraphicsSettings();
    BuildButtons();
}

void SettingsScreen::BuildBackground()
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
    
    AABB2 settingsPanelBounds(400, 150, 1200, 750);
    m_settingsPanel = new Panel(
        m_canvas,
        settingsPanelBounds,
        Rgba8(50, 50, 50),
        nullptr,
        true,
        Rgba8(120, 120, 120)
    );
}

void SettingsScreen::BuildTitle()
{
    TextSetting titleSetting;
    titleSetting.m_text = "Settings";
    titleSetting.m_color = Rgba8(230, 230, 230);
    titleSetting.m_height = 42.0f;
    
    Vec2 titlePos(450, 710);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
}

void SettingsScreen::BuildAudioSettings()
{
    float labelX = 450.0f;
    float sliderX = 650.0f;
    float sliderWidth = 400.0f;
    float startY = 640.0f;
    float spacing = 60.0f;
    
    // Master Volume
    TextSetting labelSetting;
    labelSetting.m_text = "Master Volume:";
    labelSetting.m_color = Rgba8::WHITE;
    labelSetting.m_height = 24.0f;
    
    Vec2 masterLabelPos(labelX, startY);
    m_masterVolumeLabel = new Text(m_canvas, masterLabelPos, labelSetting);
    
    AABB2 masterSliderBounds(sliderX, startY - 5, sliderX + sliderWidth, startY + 15);
    m_masterVolumeSlider = new Slider(
        m_canvas,
        masterSliderBounds,
        0.0f, 100.0f, 100.0f,  // min, max, current
        Rgba8(80, 80, 80),
        Rgba8(150, 150, 150),
        Rgba8(100, 200, 100)
    );
    
    // Music Volume
    startY -= spacing;
    labelSetting.m_text = "Music Volume:";
    Vec2 musicLabelPos(labelX, startY);
    m_musicVolumeLabel = new Text(m_canvas, musicLabelPos, labelSetting);
    
    AABB2 musicSliderBounds(sliderX, startY - 5, sliderX + sliderWidth, startY + 15);
    m_musicVolumeSlider = new Slider(
        m_canvas,
        musicSliderBounds,
        0.0f, 100.0f, 75.0f,
        Rgba8(80, 80, 80),
        Rgba8(150, 150, 150),
        Rgba8(100, 150, 200)
    );
    
    // SFX Volume
    startY -= spacing;
    labelSetting.m_text = "Sound Effects:";
    Vec2 sfxLabelPos(labelX, startY);
    m_sfxVolumeLabel = new Text(m_canvas, sfxLabelPos, labelSetting);
    
    AABB2 sfxSliderBounds(sliderX, startY - 5, sliderX + sliderWidth, startY + 15);
    m_sfxVolumeSlider = new Slider(
        m_canvas,
        sfxSliderBounds,
        0.0f, 100.0f, 85.0f,
        Rgba8(80, 80, 80),
        Rgba8(150, 150, 150),
        Rgba8(200, 150, 100)
    );
}

void SettingsScreen::BuildGraphicsSettings()
{
    float labelX = 450.0f;
    float controlX = 850.0f;
    float startY = 420.0f;
    float spacing = 50.0f;
    
    TextSetting labelSetting;
    labelSetting.m_color = Rgba8::WHITE;
    labelSetting.m_height = 24.0f;
    
    // FOV Slider
    labelSetting.m_text = "Field of View:";
    Vec2 fovLabelPos(labelX, startY);
    m_fovLabel = new Text(m_canvas, fovLabelPos, labelSetting);
    
    AABB2 fovSliderBounds(controlX, startY - 5, controlX + 300, startY + 15);
    m_fovSlider = new Slider(
        m_canvas,
        fovSliderBounds,
        60.0f, 110.0f, 90.0f,
        Rgba8(80, 80, 80),
        Rgba8(150, 150, 150),
        Rgba8(150, 200, 150)
    );
    
    // Fullscreen Checkbox
    startY -= spacing;
    labelSetting.m_text = "Fullscreen:";
    Vec2 fullscreenLabelPos(labelX, startY);
    Text* fullscreenLabel = new Text(m_canvas, fullscreenLabelPos, labelSetting);
    
    AABB2 fullscreenCheckBounds(controlX, startY - 5, controlX + 30, startY + 25);
    m_fullscreenCheckbox = new Checkbox(
        m_canvas,
        fullscreenCheckBounds,
        false,
        Rgba8(80, 80, 80),
        Rgba8(100, 200, 100),
        Rgba8(120, 120, 120)
    );
    
    // VSync Checkbox
    startY -= spacing;
    labelSetting.m_text = "VSync:";
    Vec2 vsyncLabelPos(labelX, startY);
    Text* vsyncLabel = new Text(m_canvas, vsyncLabelPos, labelSetting);
    
    AABB2 vsyncCheckBounds(controlX, startY - 5, controlX + 30, startY + 25);
    m_vsyncCheckbox = new Checkbox(
        m_canvas,
        vsyncCheckBounds,
        true,
        Rgba8(80, 80, 80),
        Rgba8(100, 200, 100),
        Rgba8(120, 120, 120)
    );
}

void SettingsScreen::BuildButtons()
{
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;
    float centerX = 800.0f;
    float buttonY = 200.0f;
    
    AABB2 doneBounds(
        centerX - buttonWidth * 0.5f, buttonY,
        centerX + buttonWidth * 0.5f, buttonY + buttonHeight
    );
    m_doneButton = new Button(
        m_canvas,
        doneBounds,
        Rgba8(100, 200, 100),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Done",
        Vec2(0.5f, 0.5f)
    );
}

void SettingsScreen::OnDoneClicked()
{
    // 保存设置
    // g_theGameUIManager->CloseSettings();
}