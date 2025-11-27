#pragma once
#include "Gamecommon.hpp"
#include "Physics/Entity.h"
#include "Physics/GameCamera.h"
#include "Engine/Renderer/Camera.hpp"

class Game;

class Player : public Entity
{
public:
    Player(Game* owner, Vec3 const& startPos);
    virtual ~Player();

    virtual void Update(float deltaSeconds) override;
    virtual void Render() const override;
	
    void UpdateInput(float deltaSeconds);
    void UpdateFromKeyboard(float deltaSeconds);
    void UpdateFromController(float deltaSeconds);
	
    void CyclePhysicsMode();
    void CycleCameraMode();

	Direction GetOrthoDirection();
	Direction GetHorizontalDirection();
	Direction GetOrthoDirectionOpposite();
	
    // Camera
    GameCamera* GetGameCamera() { return m_gameCamera; }
    Camera& GetWorldCamera() { return m_worldCamera; }

public:
    Camera m_worldCamera;
    GameCamera* m_gameCamera = nullptr;

	std::string m_playerModeString;
	
    // Input state
    bool m_isRunning = false;
    float m_normalSpeed = 6.0f;
	float m_maxSpeed = 20.f;
};