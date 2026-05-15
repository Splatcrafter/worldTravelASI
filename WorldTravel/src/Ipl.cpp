#include "Ipl.h"

using namespace std;



std::string Ipl::GetIplName()
{
	return iplName;
}

void Ipl::SetIplName(const std::string& ipl)
{
	iplName = ipl;
	iplState = STREAMING::IS_IPL_ACTIVE((char*)iplName.c_str());
	// Intentionally not flagging this as a "captured" snapshot — at startup
	// the streaming system has not necessarily finished activating every
	// default IPL, so the in-Unload auto-capture should still get a chance
	// to refresh the value before the first removal.
}



bool Ipl::GetIplState()
{
	return iplState;
}

void Ipl::SetIplState()
{
	iplState = STREAMING::IS_IPL_ACTIVE((char*)iplName.c_str());
	iplStateCaptured = true;
}

void Ipl::SetIplState(bool state)
{
	iplState = state;
	iplStateCaptured = true;
}



void Ipl::RequestIpl(bool checkState)
{
	if (checkState)
	{
		if (!iplState)
		{
			return;
		}
	}
	STREAMING::REQUEST_IPL((char*)iplName.c_str());
}

void Ipl::RequestIplIfNotActive()
{
	if (!STREAMING::IS_IPL_ACTIVE((char*)iplName.c_str()))
	{
		STREAMING::REQUEST_IPL((char*)iplName.c_str());
	}
}



void Ipl::RemoveIpl(bool saveState)
{
	if (saveState)
	{
		// Only auto-snapshot when no explicit pre-teleport capture happened
		// for this session. Otherwise we'd overwrite the (correct) state
		// captured while the player was still in the level with a (wrong)
		// reading taken after the player has been thrown to
		// (11000,11000,1500) — GTA's streaming thread has by then started
		// tearing down LS IPLs in response, and IS_IPL_ACTIVE returns false
		// for IPLs that should stay tracked as active. Consume the flag so
		// the next session has to re-capture.
		if (!iplStateCaptured)
			SetIplState();
		iplStateCaptured = false;
	}
	STREAMING::REMOVE_IPL((char*)iplName.c_str());
}

void Ipl::RemoveIplIfActive()
{
	if (STREAMING::IS_IPL_ACTIVE((char*)iplName.c_str()))
	{
		STREAMING::REMOVE_IPL((char*)iplName.c_str());
	}
}