#include "stdafx.h"
#include "ThreadPriority.h"


#include <processthreadsapi.h>

using namespace System;
using namespace System::Threading;


// could not find an external definition for now... though i don't get why
#define THREAD_MODE_BACKGROUND_BEGIN  0x00010000
#define THREAD_MODE_BACKGROUND_END  0x00020000



static LodNative::ThreadPriority::ThreadPriority(){
	auto os = Environment::OSVersion;
	BGModeIsSupported = os->Platform == PlatformID::Win32NT && os->Version >= gcnew Version(6, 0);
}
void LodNative::ThreadPriority::SetCurrentToBackground(){
	if (BGModeIsSupported){
		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	}
	else{
		Thread::CurrentThread->Priority = Threading::ThreadPriority::BelowNormal;
	}
}
void LodNative::ThreadPriority::ResetCurrentToNormal(){
	if (BGModeIsSupported){
		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
	}
	else{
		Thread::CurrentThread->Priority = Threading::ThreadPriority::BelowNormal;
	}
}