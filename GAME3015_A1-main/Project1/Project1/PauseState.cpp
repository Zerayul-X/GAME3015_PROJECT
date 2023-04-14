//***************************************************************************************
// PauseState.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "PauseState.h"
#include "SpriteNode.h"
#include "Game.hpp"

// Constructor 
PauseState::PauseState(StateStack* stack, Context* context) : State(stack, context)
{
    // Clear the vector of all render items
    mAllRitems.clear();

    // Reset the frame resources for the game and build the materials
    mContext->game->ResetFrameResources();
    mContext->game->BuildMaterials();

    // Create a sprite node for the background and set its properties
    std::unique_ptr<SpriteNode> backgroundSprite = std::make_unique<SpriteNode>(this);
    backgroundSprite->SetMatGeoDrawName("Aircrafts_Pause", "boxGeo", "box");
    backgroundSprite->setScale(60, 1.0, 50.0);
    backgroundSprite->setPosition(0, 0, 0);

    // Attach the sprite node to the scene graph
    mSceneGraph->attachChild(std::move(backgroundSprite));
    // Build the scene graph
    mSceneGraph->build();
    // Build the frame resources with the size of the render item vector
    mContext->game->BuildFrameResources(mAllRitems.size());
}

// Destructor
PauseState::~PauseState()
{
}

// Draws the scene graph
void PauseState::draw()
{
    mSceneGraph->draw();
}

// Updates the scene graph with the given game timer
bool PauseState::update(const GameTimer& gt)
{
    mSceneGraph->update(gt);

    return true;
}

// Handles events for the PauseState
bool PauseState::handleEvent(WPARAM btnState)
{
    // Check P for Resume
    if (d3dUtil::IsKeyDown('P'))
    {
        requestStackPop();
        requestStackPush(States::Game);
    } 

    // Check Q for Quit
    else if (d3dUtil::IsKeyDown('Q'))
    {
        requestStackPop();
        requestStackPush(States::Menu);
    }

    return true;
}

// Handles real time input for the PauseState
bool PauseState::handleRealtimeInput()
{
    return true;
}
