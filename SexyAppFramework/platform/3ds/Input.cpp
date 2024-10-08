#include <3ds.h>
#include <unordered_map>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

/*
static PadState pad;
static std::unordered_map<u64, KeyCode> keyMaps = {
	{HidNpadButton_Plus,  KEYCODE_ESCAPE},
	{HidNpadButton_Minus, KEYCODE_SPACE},
	{HidNpadButton_A,     KEYCODE_RETURN},
	{HidNpadButton_L,     KEYCODE_LBUTTON},
	{HidNpadButton_R,     KEYCODE_RBUTTON},
};
*/

void SexyAppBase::InitInput()
{
	/*
	// Configure our supported input layout: a single player with standard controller styles
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);

	// Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
	padInitializeDefault(&pad);

	hidInitializeTouchScreen();
	*/

	if (!mMouseIn)
		mMouseIn = true;
}

bool SexyAppBase::StartTextInput(std::string& theInput)
{
	/*
	char buf[512];

	SwkbdConfig kbd;
	swkbdCreate(&kbd, 0);
	swkbdConfigMakePresetDefault(&kbd);
	swkbdConfigSetType(&kbd, SwkbdType_Normal);

	swkbdConfigSetGuideText(&kbd, "Enter text...");
	swkbdConfigSetOkButtonText(&kbd, "OK");

	Result rc = swkbdShow(&kbd, buf, sizeof(buf));
	swkbdClose(&kbd);

	if (R_SUCCEEDED(rc))
	{
		theInput = buf;
		return true;
	}
	*/

	return false;
}

void SexyAppBase::StopTextInput()
{
	
}

bool SexyAppBase::ProcessDeferredMessages(bool singleMessage)
{
	/*
	if (!appletMainLoop())
	{
		mShutdown = true;
		return false;
	}

	static s32 prev_touchcount = 0;

	// Scan the gamepad. This should be done once for each frame
	padUpdate(&pad);

	u64 kDown = padGetButtonsDown(&pad);
	u64 kUp = padGetButtonsUp(&pad);

	if (kDown)
	{
		mLastUserInputTick = mLastTimerTime;

		for (auto& k : keyMaps)
		{
			if (kDown & k.first)
				mWidgetManager->KeyDown(k.second);
		}
	}

	if (kUp)
	{
		mLastUserInputTick = mLastTimerTime;

		for (auto& k : keyMaps)
		{
			if (kDown & k.first)
				mWidgetManager->KeyUp(k.second);
		}
	}

	HidTouchScreenState state = {0};
	hidGetTouchScreenStates(&state, 1);
	static int x=0, y=0;
	if (state.count)
	{
		mLastUserInputTick = mLastTimerTime;

		x = (int)state.touches[0].x;
		y = (int)state.touches[0].y;
		mWidgetManager->RemapMouse(x, y);
		mWidgetManager->MouseMove(x, y);
	}

	if (state.count && !prev_touchcount)
		mWidgetManager->MouseDown(x, y, 1);
	else if (!state.count && prev_touchcount)
		mWidgetManager->MouseUp(x, y, 1);

	prev_touchcount = state.count;
	*/

	return false;
}
