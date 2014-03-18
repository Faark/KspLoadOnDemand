#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

#include "Stdafx.h"

// Provides callbacks to the KSP runtime
ref class ManagedBridge{
private:
	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate void OnThumbnailUpdatedDelegate(int textureId);
	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate void OnTextureLoadedDelegate(int textureId, IntPtr nativePtr);
	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate void OnStatusUpdatedDelegate([MarshalAs(UnmanagedType::LPStr)] String^ text);
	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate void OnRequestKspUpdateDelegate();

	static OnThumbnailUpdatedDelegate^ OnThumbnailUpdated;
	static OnTextureLoadedDelegate^ OnTextureLoaded;
	static OnStatusUpdatedDelegate^ OnStatusUpdated;
	static OnRequestKspUpdateDelegate^ OnRequestKspUpdate;

public:
	static void ThumbnailUpdated(int textureId){
		OnThumbnailUpdated(textureId);
	}
	static void TextureLoaded(int textureId, IDirect3DTexture9* highResTexturePtr){
		OnTextureLoaded(textureId, IntPtr(highResTexturePtr));
	}
	static void StatusUpdate(String^ text){
		OnStatusUpdated(text);
	}
	static void RequestKspUpdate(){
		OnRequestKspUpdate();
	}
	static void Setup(void* thumbUpdateCallback, void*textureLoadedCallback, void* statusUpdatedCallback, void* requestKspUpdateCallback){
		OnThumbnailUpdated = Marshal::GetDelegateForFunctionPointer<OnThumbnailUpdatedDelegate^>(IntPtr(thumbUpdateCallback));
		OnTextureLoaded = Marshal::GetDelegateForFunctionPointer<OnTextureLoadedDelegate^>(IntPtr(textureLoadedCallback));
		OnStatusUpdated = Marshal::GetDelegateForFunctionPointer<OnStatusUpdatedDelegate^>(IntPtr(statusUpdatedCallback));
		OnRequestKspUpdate = Marshal::GetDelegateForFunctionPointer<OnRequestKspUpdateDelegate^>(IntPtr(requestKspUpdateCallback));
	}
};