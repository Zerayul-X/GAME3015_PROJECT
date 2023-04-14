//***************************************************************************************
// StateStack.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************

#include "StateStack.hpp"
#include <cassert>
#include "Game.hpp"

// Constructor for StateStack
StateStack::StateStack(State::Context context)
	: mStack()
	, mPendingList()
	, mContext(context)
	, mFactories()
{
}

// Iterates over the stack and calls update method of each state
void StateStack::update(const GameTimer& gt)
{
	for (auto itr = mStack.rbegin(); itr != mStack.rend(); ++itr)
	{
		if (!(*itr)->update(gt))
			break;
	}

	applyPendingChanges();
}

// Iterates over the stack and calls draw method of each state
void StateStack::draw()
{
	for (State::StatePtr& state : mStack)
		state->draw();
}

// Iterates over the stack and calls handleEvent method of each state
void StateStack::handleEvent(WPARAM btnState)
{
	for (auto itr = mStack.rbegin(); itr != mStack.rend(); ++itr)
	{
		if (!(*itr)->handleEvent(btnState))
			break;
	}
}

// Iterates over the stack and calls handleRealtimeInput method of each state
void StateStack::handleRealtimeInput()
{
	for (auto itr = mStack.rbegin(); itr != mStack.rend(); ++itr)
	{
		if (!(*itr)->handleRealtimeInput())
			break;
	}
}

// Pushes a new state onto the pending list with the given state ID
void StateStack::pushState(States::ID stateID)
{
	mPendingList.push_back(PendingChange(Push, stateID));
}

// Pushes a pop command onto the pending list
void StateStack::popState()
{
	mPendingList.push_back(PendingChange(Pop));
}

// Pushes a clear command onto the pending list
void StateStack::clearStates()
{
	mPendingList.push_back(PendingChange(Clear));
}

// Returns true if the stack is empty, false otherwise
bool StateStack::isEmpty() const
{
	return mStack.empty();
}

// Gets the camera position of the top state on the stack
XMFLOAT3 StateStack::getCameraPos()
{
	if (mStack.size() != 0)
	{
		return mStack.back()->getCameraPos();
	}
	return XMFLOAT3(0.f,0.f,0.f);
}

// Gets the target position of the top state on the stack
XMFLOAT3 StateStack::getTargetPos()
{
	if (mStack.size() != 0)
	{
		return mStack.back()->getTargetPos();
	}
	return XMFLOAT3(0.f, 0.f, 0.f);
}

// Returns a pointer to the current state at the top of the stack
State* StateStack::getCurrentState()
{
	return mStack.back().get();
}

// Creates a new state with the given state ID using the corresponding factory method
State::StatePtr StateStack::createState(States::ID stateID)
{
	auto found = mFactories.find(stateID);
	assert(found != mFactories.end());
	return found->second();
}

// Apply any pending changes to the state stack
void StateStack::applyPendingChanges()
{
	// Depending on the change type, push, pop, or clear the stack
	for (PendingChange change : mPendingList)
	{
		switch (change.action)
		{
		case Push:
			mStack.push_back(createState(change.stateID));
			break;
		case Pop:
			mStack.pop_back();
			break;
		case Clear:
			mStack.clear();
			break;
		}
	}

	// Clear the list of pending changes
	mPendingList.clear();
}

// Constructor for a pending change
StateStack::PendingChange::PendingChange(Action action, States::ID stateID)
	: action(action)
	, stateID(stateID)
{
}
