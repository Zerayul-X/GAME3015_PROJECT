//***************************************************************************************
// State.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************

#include "State.hpp"
#include "StateStack.hpp"
#include "SceneNode.hpp"
#include "Game.hpp"

// Constructor for the State Context class
State::Context::Context(Game* _game, Player* _player)
	: game(_game), 
	player(_player)
{
}

// Constructor for the State class
State::State(StateStack* stack, Context* context)
	: mStack(stack)
	, mContext(context)
	, mCameraPos(0.f, 0.f, 0.f)
	, mSceneGraph(std::make_unique<SceneNode>(this))
{
}

// Destructor
State::~State()
{

}

// Returns a pointer to the Context object
State::Context* State::getContext() const
{
	return mContext;
}

// Adds a new state to the stack
void State::requestStackPush(States::ID stateID)
{
	mStack->pushState(stateID);
}

// Removes the current state from the stack
void State::requestStackPop()
{
	mStack->popState();
}

// Removes all states from the stack
void State::requestStateClear()
{
	mStack->clearStates();
}


