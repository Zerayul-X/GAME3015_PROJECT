//***************************************************************************************
// TitleState.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************

#include "TitleState.hpp"
#include "SpriteNode.h"
#include "Game.hpp"

// Constructor for the TitleState class
TitleState::TitleState(StateStack* stack, Context* context)
    : State(stack, context)
{
    // Clear all rendering items, reset frame resources, and build materials
    mAllRitems.clear();
    mContext->game->ResetFrameResources();
    mContext->game->BuildMaterials();

    // Create background sprite and set its properties
    std::unique_ptr<SpriteNode> backgroundSprite = std::make_unique<SpriteNode>(this);
    backgroundSprite->SetMatGeoDrawName("Aircrafts_Title", "boxGeo", "box");
    backgroundSprite->setScale(60, 1.0, 50.0);
    backgroundSprite->setPosition(0, 0, 0);
    mSceneGraph->attachChild(std::move(backgroundSprite));
    
    // Build the scene graph and frame resources
    mSceneGraph->build();
    mContext->game->BuildFrameResources(mAllRitems.size());
}

// Destructor
TitleState::~TitleState()
{
}

// Draw the scene
void TitleState::draw()
{
    mSceneGraph->draw();
}

// Update the scene
bool TitleState::update(const GameTimer& gt)
{
    mSceneGraph->update(gt);
    return true;
}

// Handle events, such as button clicks
bool TitleState::handleEvent(WPARAM btnState)
{
    // Remove the current state and add the menu state to the stack
    requestStackPop();
    requestStackPush(States::Menu);
    return true;
}

// Handle real time input
bool TitleState::handleRealtimeInput()
{
    return true;
}
