#pragma once


using namespace System;
using namespace System::Collections::Concurrent;

#include "ManagedBridge.h"

namespace LodNative{
	// GPU work queue...
	ref class KSPThread{

		static BlockingCollection<Action<IntPtr>^>^ PriorityJobs = gcnew BlockingCollection<Action<IntPtr>^>();//infinite, since those are usually issued from KSP-Thread as well
		static BlockingCollection<Action<IntPtr>^>^ Jobs = gcnew BlockingCollection<Action<IntPtr>^>(2);
		static array<BlockingCollection<Action<IntPtr>^>^>^ AllJobs;
		static KSPThread(){
			array<BlockingCollection<Action<IntPtr>^>^>^ AllJobsArray = { PriorityJobs, Jobs };
			AllJobs = AllJobsArray;
		}
		static int IsUpdateRequested = 0;
		static void RequestUpdateIfNecessary(){
			if (System::Threading::Interlocked::Exchange(IsUpdateRequested, 1) == 0){
				ManagedBridge::RequestKspUpdate();
			}
		}
	public:
		static void EnqueueJob(Action<IntPtr>^ job, bool highPriority){
			(highPriority ? PriorityJobs : Jobs)->Add(job);
			RequestUpdateIfNecessary();
		}
		static void EnqueueJob(Action<IntPtr>^ job){
			EnqueueJob(job, false);
		}
		static void Update(IDirect3DTexture9* devicePtrTexture){
			IDirect3DDevice9* device;
			devicePtrTexture->GetDevice(&device);
			// Todo1: & Todo2: Limit time / gpu traffic / whatever ... see RequestedUpdate below for that...
			auto devicePtr = IntPtr(device);
			while (true){
				Action<IntPtr>^ currentJob;
				if (!PriorityJobs->TryTake(currentJob)){
					if (!Jobs->TryTake(currentJob)){
						return;
					}
				}
				currentJob(devicePtr);
			}
		}
		static bool RequestedUpdate(IDirect3DTexture9* devicePtrTexture){
			IsUpdateRequested = false;
			Update(devicePtrTexture);
			return false;
		}
	};
}