#include <string>
#include "../dependencies/include/natives.h"

using namespace std;
#ifndef IPL
#define IPL
class Ipl
{
public:
	std::string iplName;
	bool iplState;
	// Set to true once iplState has been snapshot for the current "session"
	// (i.e. since the last RemoveIpl(saveState=true) consumed it). Lets an
	// explicit pre-teleport CaptureStates take priority over the in-Unload
	// auto-capture, which fires after the player has been punted to
	// (11000,11000,1500) and would otherwise overwrite the snapshot with a
	// false reading from GTA's streaming-thread teardown-in-progress.
	bool iplStateCaptured = false;

	std::string GetIplName();
	void SetIplName(const std::string& ipl);

	bool GetIplState();
	void SetIplState();
	void SetIplState(bool state);

	void RequestIpl(bool checkState);
	void RequestIplIfNotActive();

	void RemoveIpl(bool saveState);
	void RemoveIplIfActive();
};
#endif