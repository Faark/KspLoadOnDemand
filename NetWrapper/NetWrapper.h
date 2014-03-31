// NetWrapper.h

#pragma once

using namespace System;

#include "Logger.h"
#include "TextureManager.h"
#include "ManagedBridge.h"
#include "KSPThread.h"
#include "GPU.h"
/*
NotImplementedException / Todo: Exceptions appear to not travel though CLRs... they at least crash KSP.
Think about letting them travel anyway, though for now crashing is kinda nice.
*/

extern "C" __declspec(dllexport) void NlodDebug_DumpTexture(void* texturePtr, const char* file_name){
	try{
		auto texture = (IDirect3DTexture9*)texturePtr;
		D3DSURFACE_DESC desc;
		texture->GetLevelDesc(0, &desc);
		auto bmp = gcnew Drawing::Bitmap(desc.Width, desc.Height, AssignableFormat::PixelFormatFromD3DFormat(desc.Format));
		auto bmpData = bmp->LockBits(Drawing::Rectangle(0, 0, desc.Width, desc.Height), ImageLockMode::WriteOnly, bmp->PixelFormat);
		D3DLOCKED_RECT rect;
		texture->LockRect(0, &rect, NULL, 0);
		GPU::CopyBuffer((unsigned char*)rect.pBits, rect.Pitch, (unsigned char*)bmpData->Scan0.ToPointer(), bmpData->Stride, AssignableFormat::GetByteDepthForFormat(desc.Format)*desc.Width, desc.Height);
		texture->UnlockRect(0);
		bmp->UnlockBits(bmpData);
		bmp->Save(gcnew String(file_name), ImageFormat::Png);
	}
	catch (Exception^ err){
		Logger::LogException(err);
		throw;
	}
	Logger::MayCrash();
}
extern "C" __declspec(dllexport) void NlodSetup(const char* cache_directory, void* thumbUpdateCallback, void*textureLoadedCallback, void* statusUpdatedCallback, void* requestUpdateFromKspThreadCallback, void* onSignalThreadIdlleCallback){
	/*array<String^, 1> ^ names = System::Reflection::Assembly::GetExecutingAssembly()->GetManifestResourceNames();
	for (int i = 0; i < names->Length; i++){
	System::IO::File::AppendAllText("C:\\ksp_rtLodNew\\bridgeNet.txt", names[i]);
	System::IO::File::AppendAllText("C:\\ksp_rtLodNew\\bridgeNet.txt", Environment::NewLine);
	}*/
	try{
		String^ cacheDirectory = gcnew String(cache_directory);
		Logger::Setup(cacheDirectory);
		ManagedBridge::Setup(thumbUpdateCallback, textureLoadedCallback, statusUpdatedCallback, requestUpdateFromKspThreadCallback, onSignalThreadIdlleCallback);
		TextureManager::Setup(cacheDirectory);
	}
	catch (Exception^ err){
		try{
			Logger::LogException(err);
		}
		catch (Exception^err){
			System::IO::File::WriteAllText("LoadOnDemand.log", err->ToString());
			if (err->InnerException != nullptr){
				System::IO::File::AppendAllText("LoadOnDemand.log", err->InnerException->ToString());
			}
		}
		throw;
	}
	Logger::MayCrash();
}
extern "C" __declspec(dllexport) int NlodRegisterTexture(const char* file, const char* cacheKey, void* thumbTexturePtr, bool isNormalMap){
	try{
		String^ f = gcnew String(file);
		String^ ck = gcnew String(cacheKey);
		Logger::LogTrace(String::Format("Enter RegisterTexture({0}, {1}, {2}, " + isNormalMap + ")", f, ck, IntPtr(thumbTexturePtr).ToInt32()));
		if (thumbTexturePtr == NULL){
			throw gcnew ArgumentException("Thumb texture ptr is null!");
		}
		return TextureManager::RegisterTexture(f, ck, (IDirect3DTexture9*)thumbTexturePtr, isNormalMap);
	}
	catch (Exception^err){
		Logger::LogException(err);
		throw err;
	}
	finally{
		Logger::LogTrace("Leave RegisterTexture");
	}
	Logger::MayCrash();
}
extern "C" __declspec(dllexport) void NlodRequestTextureLoad(int nativeId){
	try{
		Logger::LogTrace(String::Format("Enter TextureLoad({0})", nativeId));
		TextureManager::RequestTextureLoad(nativeId);
	}
	catch (Exception^err){
		Logger::LogException(err);
		throw err;
	}
	finally{
		Logger::LogTrace("Leave TextureLoad");
	}
	Logger::MayCrash();
}
extern "C" __declspec(dllexport) void NlodRequestTextureUnload(int nativeId){
	try{
		Logger::LogTrace(String::Format("Enter TextureUnload({0})", nativeId));
		TextureManager::RequestTextureUnload(nativeId);
	}
	catch (Exception^err){
		Logger::LogException(err);
		throw err;
	}
	finally{
		Logger::LogTrace("Leave TextureUnload");
	}
	Logger::MayCrash();
}
extern "C" __declspec(dllexport) bool NlodRequestedUpdate(void* deviceRefTexturePtr){
	try{
		Logger::LogTrace("Enter RequestedUpdate");
		return KSPThread::RequestedUpdate((IDirect3DTexture9*)deviceRefTexturePtr);
	}
	catch (Exception^err){
		Logger::LogException(err);
		throw;
	}
	finally{
		Logger::LogTrace("Leave RequestedUpdate");
	}
	Logger::MayCrash();
}
extern "C" __declspec(dllexport) void NlodStartSignalMessages()
{
	try{
		Logger::LogTrace("Enter StartSignalMessages");
		ManagedBridge::StartMessageLoop();
	}
	catch (Exception^err){
		Logger::LogException(err);
		throw;
	}
	finally{
		Logger::LogTrace("Leave StartSignalMessages");
	}
	Logger::MayCrash();
}