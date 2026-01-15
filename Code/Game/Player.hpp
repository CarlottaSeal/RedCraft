#pragma once
#include "Gamecommon.hpp"
#include "Physics/Entity.h"
#include "Physics/GameCamera.h"
#include "Engine/Renderer/Camera.hpp"

struct Item;
class PlayerInventory;
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
	
	void CollectNearbyDrops();

	Direction GetOrthoDirection();
	Direction GetHorizontalDirection();
	Direction GetOrthoDirectionOpposite();

	void SelectHotbarSlot(int slot);
	int GetSelectedHotbarSlot() const;
	Item const& GetSelectedItem() const;
    
	PlayerInventory* GetInventory() { return m_inventory; }
	PlayerInventory const* GetInventory() const { return m_inventory; }
    
	void HandleHotbarInput();

    GameCamera* GetGameCamera() { return m_gameCamera; }
    Camera& GetWorldCamera() { return m_worldCamera; }

public:
    Camera m_worldCamera;
    GameCamera* m_gameCamera = nullptr;

	std::string m_playerModeString;

	PlayerInventory* m_inventory = nullptr;  
	
    // Input state
    bool m_isRunning = false;
    float m_normalSpeed = 6.0f;
	float m_maxSpeed = 20.f;
};