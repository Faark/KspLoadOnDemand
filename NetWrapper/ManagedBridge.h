#pragma once

using namespace System;
using namespace System::Collections::Concurrent;
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
	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate void OnSignalThreadIdleDelegate();

	static OnThumbnailUpdatedDelegate^ OnThumbnailUpdated;
	static OnTextureLoadedDelegate^ OnTextureLoaded;
	static OnStatusUpdatedDelegate^ OnStatusUpdated;
	static OnRequestKspUpdateDelegate^ OnRequestKspUpdate;
	static OnSignalThreadIdleDelegate^ OnSignalThreadIdle;

	static BlockingCollection<Action^>^ Messages = gcnew BlockingCollection<Action^>();
	ref class ThumbnailUpdatedScope{
	public:
		int TextureId;
		ThumbnailUpdatedScope(int textureId) :TextureId(textureId){}
		void Do(){
			OnThumbnailUpdated(TextureId);
		}
	};
	ref class TextureLoadedScope{
	public:
		int TextureId;
		IDirect3DTexture9* HighResTexturePtr;
		TextureLoadedScope(int textureId, IDirect3DTexture9* highResTexturePtr) :TextureId(textureId), HighResTexturePtr(highResTexturePtr){}
		void Do(){
			OnTextureLoaded(TextureId, IntPtr(HighResTexturePtr));
		}
	};
	ref class StatusUpdateScope{
	public:
		String^ Text;
		StatusUpdateScope(String^ text) :Text(text){}
		void Do(){
			OnStatusUpdated(Text);
		}
	};
	static void Do_RequestUpdate(){
		OnRequestKspUpdate();
	}
public:
	static void ThumbnailUpdated(int textureId){
		Messages->Add(gcnew Action(gcnew ThumbnailUpdatedScope(textureId), &ThumbnailUpdatedScope::Do));
	}
	static void TextureLoaded(int textureId, IDirect3DTexture9* highResTexturePtr){
		Messages->Add(gcnew Action(gcnew TextureLoadedScope(textureId, highResTexturePtr), &TextureLoadedScope::Do));
	}
	static void StatusUpdate(String^ text){
		Messages->Add(gcnew Action(gcnew StatusUpdateScope(text), &StatusUpdateScope::Do));
	}
	static void RequestKspUpdate(){
		Messages->Add(gcnew Action(Do_RequestUpdate));
	}
	/*static Action^dbgEmptySpin;
	static void DebugEmptySpinDummy(){
	OnSignalThreadIdle();
	Messages->Add(dbgEmptySpin);
	}*/
	static void StartMessageLoop(){
		//dbgEmptySpin = gcnew Action(&DebugEmptySpinDummy);
		//Messages->Add(dbgEmptySpin);
		// Todo: Do we need error handling? Not rly atm, since exceptions don't travel from mono to net4 anyway but just kill the thread.
		while (true){
			Action^ message;
			if (Messages->TryTake(message, 250)){
				message();
			}
			else{
				OnSignalThreadIdle();
			}
		}
	}
	static void Setup(void* thumbUpdateCallback, void*textureLoadedCallback, void* statusUpdatedCallback, void* requestKspUpdateCallback, void* onSignalThreadIdlleCallback){
		/*
		OnThumbnailUpdated = Marshal::GetDelegateForFunctionPointer<OnThumbnailUpdatedDelegate^>(IntPtr(thumbUpdateCallback));
		OnTextureLoaded = Marshal::GetDelegateForFunctionPointer<OnTextureLoadedDelegate^>(IntPtr(textureLoadedCallback));
		OnStatusUpdated = Marshal::GetDelegateForFunctionPointer<OnStatusUpdatedDelegate^>(IntPtr(statusUpdatedCallback));
		OnRequestKspUpdate = Marshal::GetDelegateForFunctionPointer<OnRequestKspUpdateDelegate^>(IntPtr(requestKspUpdateCallback));
		OnSignalThreadIdle = Marshal::GetDelegateForFunctionPointer<OnSignalThreadIdleDelegate^>(IntPtr(onSignalThreadIdlleCallback));
		*/
		OnThumbnailUpdated = (OnThumbnailUpdatedDelegate^)Marshal::GetDelegateForFunctionPointer(IntPtr(thumbUpdateCallback), OnThumbnailUpdatedDelegate::typeid);
		OnTextureLoaded = (OnTextureLoadedDelegate^)Marshal::GetDelegateForFunctionPointer(IntPtr(textureLoadedCallback), OnTextureLoadedDelegate::typeid);
		OnStatusUpdated = (OnStatusUpdatedDelegate^)Marshal::GetDelegateForFunctionPointer(IntPtr(statusUpdatedCallback), OnStatusUpdatedDelegate::typeid);
		OnRequestKspUpdate = (OnRequestKspUpdateDelegate^)Marshal::GetDelegateForFunctionPointer(IntPtr(requestKspUpdateCallback), OnRequestKspUpdateDelegate::typeid);
		OnSignalThreadIdle = (OnSignalThreadIdleDelegate^)Marshal::GetDelegateForFunctionPointer(IntPtr(onSignalThreadIdlleCallback), OnSignalThreadIdleDelegate::typeid);


		System::AppDomain::CurrentDomain->UnhandledException += gcnew System::UnhandledExceptionEventHandler(&ManagedBridge::OnUnhandledException);
	}

	static bool firstException = true;
	static void ManagedBridge::OnUnhandledException(System::Object ^sender, System::UnhandledExceptionEventArgs ^e)
	{
		if (firstException){
			firstException = false;
			auto err = dynamic_cast<Exception^>(e->ExceptionObject);
			if (err != nullptr){
				Logger::LogText("Unhandled exception!");
				Logger::LogException(err);
				Logger::crashGame = true;
			}
		}
	}
};


