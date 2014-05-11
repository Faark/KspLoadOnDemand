#pragma once


using namespace System;
using namespace System::Threading;



namespace LodNative{
	// Just a shortcut to system.thread. Doesn't save much, but helps since i usually forget that syntax^^. It can't help with try-catch, though...
	ref struct Lock{
	private:
		bool flag;
		Object^ obj;
	public:
		void Take(Object^ lock_object){
			Monitor::Enter(obj = lock_object, flag);
		}
		void TryExit(){
			if (flag){
				Monitor::Exit(obj);
			}
		}
	};
}