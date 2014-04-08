#pragma once


//WinBase.h on Windows XP, Windows Server 2003, Windows Vista, Windows 7, Windows Server 2008, and Windows Server 2008 R2(include Windows.h);
//Processthreadsapi.h on Windows 8 and Windows Server 2012


/*

Sources:
( http://msdn.microsoft.com/en-us/library/ms686219%28VS.85%29.aspx )
http://msdn.microsoft.com/en-us/library/ms686277%28VS.85%29.aspx
a "simplified" version of:
http://code.logos.com/blog/2008/10/using_background_processing_mode_from_c.html

*/
namespace LodNative{
	ref class ThreadPriority{
	private:
		static bool BGModeIsSupported;
		static ThreadPriority();
	public:
		static void SetCurrentToBackground();
		static void ResetCurrentToNormal();
	};
}
