#pragma once
#include "Engine/UI/UIScreen.h"

class Button;
class Panel;
class Text;

class WorldSelectScreen : public UIScreen
{
public:
    WorldSelectScreen(UISystem* uiSystem);
    virtual ~WorldSelectScreen() override;
    
    void Build() override;
    void Update(float deltaSeconds) override;
    void Render() const override;
    
private:
    void OnInfiniteWorldClicked();
    void OnFarmWorldClicked();
    
private:
    Panel* m_background = nullptr;
    Text* m_titleText = nullptr;
    
    Button* m_infiniteButton = nullptr;
    Button* m_farmButton = nullptr;
};