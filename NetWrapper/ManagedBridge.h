#pragma once

using namespace System;
using namespace System::Collections::Concurrent;
using namespace System::Runtime::InteropServices;

#include "Stdafx.h"

namespace LodNative{
	// Provides callbacks to the KSP runtime
	ref class ManagedBridge{
	private:
		[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
		delegate void OnThumbnailUpdatedDelegate(int textureId, IntPtr nativePtr);
		[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
		delegate void OnTextureLoadedDelegate(int textureId, IntPtr nativePtr);
		[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
		delegate void OnStatusUpdatedDelegate([MarshalAs(UnmanagedType::LPStr)] String^ text);
		[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
		delegate void OnRequestKspUpdateDelegate();
		[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
		delegate void OnSignalThreadIdleDelegate();
		[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
		delegate void OnTexturePreparedDelegate();

		static OnThumbnailUpdatedDelegate^ OnThumbnailUpdated;
		static OnTextureLoadedDelegate^ OnTextureLoaded;
		static OnStatusUpdatedDelegate^ OnStatusUpdated;
		static OnRequestKspUpdateDelegate^ OnRequestKspUpdate;
		static OnSignalThreadIdleDelegate^ OnSignalThreadIdle;
		static OnTexturePreparedDelegate^ OnTexturePrepared;

		static BlockingCollection<Action^>^ Messages = gcnew BlockingCollection<Action^>();
		ref class ThumbnailUpdatedScope{
		public:
			int TextureId;
			IDirect3DTexture9* ThumbTexturePtr;
			ThumbnailUpdatedScope(int textureId, IDirect3DTexture9* thumbTexturePtr) :TextureId(textureId), ThumbTexturePtr(thumbTexturePtr){}
			void Do(){
				OnThumbnailUpdated(TextureId, IntPtr(ThumbTexturePtr));
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
			OnRequestKspUpdate();//those could probably be on the msg-queue itself...
		}
		static void Do_TexturePrepared(){
			OnTexturePrepared();//those could probably be on the msg-queue itself...
		}
	public:
		static void ThumbnailUpdated(int textureId, IDirect3DTexture9* thumbTexturePtr){
			Logger::LogText("ThumbUpdated: " + textureId);
			Messages->Add(gcnew Action(gcnew ThumbnailUpdatedScope(textureId, thumbTexturePtr), &ThumbnailUpdatedScope::Do));
		}
		static void TextureLoaded(int textureId, IDirect3DTexture9* highResTexturePtr){
			Messages->Add(gcnew Action(gcnew TextureLoadedScope(textureId, highResTexturePtr), &TextureLoadedScope::Do));
		}
		static void TexturePrepared(int textureId){
			Messages->Add(gcnew Action(Do_TexturePrepared));
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
		static void Setup(void* thumbUpdateCallback, void*textureLoadedCallback, void* statusUpdatedCallback, void* requestKspUpdateCallback, void* onSignalThreadIdlleCallback, void* texturePreparedCallback){
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
			OnTexturePrepared = (OnTexturePreparedDelegate^)Marshal::GetDelegateForFunctionPointer(IntPtr(texturePreparedCallback), OnTexturePreparedDelegate::typeid);


			System::AppDomain::CurrentDomain->UnhandledException += gcnew System::UnhandledExceptionEventHandler(&ManagedBridge::OnUnhandledException);
		}
	private:
		static bool gotCrash = false;
	public:
		static void SetCrash(){
			gotCrash = true;
			ManagedBridge::RequestKspUpdate();
		}
		static void MayCrash(){
			if (gotCrash)
				throw gcnew Exception("There was an error => lets crash ksp. Check log for infos.");
		}
		static void ManagedBridge::OnUnhandledException(System::Object ^sender, System::UnhandledExceptionEventArgs ^e)
		{
			if (!gotCrash){
				gotCrash = true;
				try{
					auto err = dynamic_cast<Exception^>(e->ExceptionObject);
					if (err != nullptr){
						Logger::LogText("Unhandled exception!");
						Logger::LogException(err);
						auto mem1 = GC::GetTotalMemory(false);
						auto mem2 = GC::GetTotalMemory(true);
						Logger::LogText("GC { 1: " + mem1 + ", 2: " + mem2 + " }");
						auto p = System::Diagnostics::Process::GetCurrentProcess();
						Logger::LogText("Process {" +
							Environment::NewLine + "	Id: " + p->Id +
							Environment::NewLine + "	MachineName: " + p->MachineName +
							Environment::NewLine + "	MaxWorkingSet: " + p->MaxWorkingSet +
							Environment::NewLine + "	MinWorkingSet: " + p->MinWorkingSet +
							Environment::NewLine + "	NonpagedSystemMemorySize64: " + p->NonpagedSystemMemorySize64 +
							Environment::NewLine + "	PagedMemorySize64: " + p->PagedMemorySize64 +
							Environment::NewLine + "	PagedSystemMemorySize64: " + p->PagedSystemMemorySize64 +
							Environment::NewLine + "	PeakPagedMemorySize64: " + p->PeakPagedMemorySize64 +
							Environment::NewLine + "	PeakVirtualMemorySize64: " + p->PeakVirtualMemorySize64 +
							Environment::NewLine + "	PeakWorkingSet64: " + p->PeakWorkingSet64 +
							Environment::NewLine + "	PrivateMemorySize64: " + p->PrivateMemorySize64 +
							Environment::NewLine + "	PrivilegedProcessorTime: " + p->PrivilegedProcessorTime +
							Environment::NewLine + "	ProcessName: " + p->ProcessName +
							Environment::NewLine + "	StartTime: " + p->StartTime +
							Environment::NewLine + "	TotalProcessorTime: " + p->TotalProcessorTime +
							Environment::NewLine + "	UserProcessorTime: " + p->UserProcessorTime +
							Environment::NewLine + "	VirtualMemorySize64: " + p->VirtualMemorySize64 +
							Environment::NewLine + "	WorkingSet64: " + p->WorkingSet64 +
							//Environment::NewLine + ": " + p +
							Environment::NewLine + "}");
					}
				}
				finally{
					SetCrash();
				}
			}
		}
	};


}