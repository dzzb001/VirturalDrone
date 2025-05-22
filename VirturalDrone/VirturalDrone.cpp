// VirturalDrone.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Input.h"
#include "drone.h"
#include "Media/MeadiaPaser.h"

int main()
{
	Input::CInput input;
	input.CaptureSignal();

	CDrone drone;
	drone.Init();
	input.Loop();
	drone.Deinit();

    return 0;
}

