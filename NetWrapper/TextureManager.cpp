#include "stdafx.h"
#include "TextureManager.h"
#include "Lock.h"
#include "Disk.h"
#include "GPU.h"
#include "ThumbnailTexture.h"

void TextureManager::Setup(String^ cache_directory)
{
	mCacheDir = cache_directory;

	/*
	// Crash on gcnew Bitmap... a ParameterException though its outa mem?
	auto dump = gcnew array<Drawing::Bitmap^>(1024 * 10);
	for (int x = 0; x < dump->Length; x++){
		Logger::LogText("Created " + x);
		dump[x] = gcnew Bitmap(1024 * 1, 1024 * 1, PixelFormat::Format32bppArgb);
	}
	for (int x = 0; x < dump->Length; x++){
		dump[x]->SetPixel(10, 12, Color::FromArgb(1, 2, 3));
		delete dump[x];
	}
	delete dump;
	/**/
}


void TextureManager::StartLoadHighResTextureScope::ProcessLoadedData(array<Byte>^ loaded_data){
	//this->textureData->IsNormal
	auto img = FormatDatabase::Recognize(textureData->HighResFile, loaded_data);
	auto bmp = FormatDatabase::ConvertTo<BitmapFormat^>(img)->MayToNormal(textureData->IsNormal);
	GPU::CreateHighResTextureAsync(bmp, textureId);
	//GPU::CreateHighResTextureAsync(FormatDatabase::ConvertToAssignable(FormatDatabase::Recognize(textureData->HighResFile, loaded_data)), textureId);
	// FormatDatabase.Recognize(info.File, bytes).ConvertToAssignable().AssignToAsync(id);
}
void TextureManager::StartLoadHighResTexture(int textureId, TextureData^ data){
/*	if (data->IsNormal){
		Logger::LogText("Skipping load because of normal: " + textureId);
		return;
	}
*/	Disk::RequestFile(data->HighResFile, gcnew Action<array<Byte>^>(gcnew StartLoadHighResTextureScope(textureId, data), &StartLoadHighResTextureScope::ProcessLoadedData));
}
void TextureManager::OnTexturePreperationCompleted(TextureInitialization^ tex_init){

	Logger::LogText("Texture prepared: " + tex_init->TextureId);
	// Todo: Move this to KSP thread so we don't have to lock/consider MT stuff?
	Lock lock;
	try{
		lock.Take(textures);

		TextureData^ texData = textures[tex_init->TextureId];
		texData->HighResFile = tex_init->HighResolutionTextureFile;
		if (texData->IsRequested){
			StartLoadHighResTexture(tex_init->TextureId, texData);
		}
	}
	finally{
		lock.TryExit();
	}
}

int TextureManager::RegisterTexture(String^ file, String^ cache, IDirect3DTexture9* thumb, bool normal)
{
	/*
	//throw gcnew NotImplementedException("Debug: Load via DX and save few bytes...");
	auto dxBytes = gcnew array<Byte>(100);
	IDirect3DDevice9* device;
	thumb->GetDevice(&device);
	IDirect3DTexture9* dxTex;
	auto unmanagedStr = System::Runtime::InteropServices::Marshal::StringToHGlobalUni(file);
	D3DXCreateTextureFromFile(device, (LPWSTR)unmanagedStr.ToPointer(), &dxTex);
	System::Runtime::InteropServices::Marshal::FreeHGlobal(unmanagedStr);
	D3DLOCKED_RECT dxRect;
	dxTex->LockRect(0, &dxRect, NULL, 0);
	auto dxPtr = (unsigned char*)dxRect.pBits;
	for (int x = 0; x < dxBytes->Length; x++){
		dxBytes[x] = *dxPtr;
		dxPtr++;
	}
	dxTex->UnlockRect(0);


	ITextureBase::dontLogConverts = true;
	//throw gcnew NotImplementedException("Debug: Load to Bitmap, lock and save few bytes...");
	auto bmpBytes = gcnew array<Byte>(100);
	auto bmpUnknown = gcnew BitmapFormat(gcnew Bitmap(file), false, gcnew TextureDebugInfo(file));
	auto bmpObj = bmpUnknown->ConvertTo(PixelFormat::Format32bppArgb);
	auto bmp = bmpObj->Bitmap;

	auto bmpData = bmp->LockBits(Drawing::Rectangle(0, 0, bmp->Width, bmp->Height), ImageLockMode::ReadOnly, PixelFormat::Format32bppArgb);
	auto bmpPtr = (unsigned char*)bmpData->Scan0.ToPointer();
	for (int x = 0; x < bmpBytes->Length; x++){
		bmpBytes[x] = *bmpPtr;
		bmpPtr++;
	}
	bmp->UnlockBits(bmpData);

	
	//throw gcnew NotImplementedException("Debug: Load to ThumbStruct and save few bytes...");
	auto thumbObj = ThumbnailTexture::ConvertFromBitmap(bmpObj);
	auto thumbBytes = thumbObj->GetFileBytes();
	ITextureBase::dontLogConverts = false;

	auto dxSb = gcnew System::Text::StringBuilder();
	auto bmpSb = gcnew System::Text::StringBuilder();
	auto thSb = gcnew System::Text::StringBuilder();

	for (int x = 0; x < dxBytes->Length; x++){
		dxSb->Append(dxBytes[x]);
		dxSb->Append(",");
		bmpSb->Append(bmpBytes[x]);
		bmpSb->Append(",");
		thSb->Append(thumbBytes[x + ThumbnailTexture::ByteDataHeaderOffset]);
		thSb->Append(",");
	}
	Logger::LogText(
		Environment::NewLine
		+ Environment::NewLine + "File: " + file
		+ Environment::NewLine + "DX:{" + dxSb->ToString() + "done}"
		+ Environment::NewLine + "BMP:{" + bmpSb->ToString() + "done}"
		+ Environment::NewLine + "THUMB:{" + thSb->ToString() + "done}"
		+ Environment::NewLine
		+ Environment::NewLine);
	*/


	Lock lock;
	try{
		lock.Take(textures);

		int textureId = textures->Count;
		TextureData^ texData = gcnew TextureData();
		texData->IsNormal = normal;
		textures->Add(texData);

		//return textureId;
		//if (normal)
		//{
			//Logger::LogText("DEBUG!");
			//return textureId;
		//}

		TextureInitialization::Start(
			textureId, 
			file, 
			cache, 
			CacheDir,
			thumb,
			normal,
			gcnew Action<TextureInitialization^>(&TextureManager::OnTexturePreperationCompleted)
			);

		return textureId;
	}
	finally{
		lock.TryExit();
	}
}

void TextureManager::RequestTextureLoad(int textureId){
	TextureData^ texData = textures[textureId];
	if (texData->IsRequested){
		// Cant happen anymore since we have Requested at other level. IsRequested here shouldn't be necessary anymore...
		//return; // this can happen in case of Get[Texture]->RequestLoad, Release, Get->RequestLoad (Still loading). Atm not caught on KSP runtime, thus ignoring it for now.
		throw gcnew NotImplementedException("Was already requested. Todo: Do we actually want to support this? Kinda same as HighResTexture check, btw");
	}
	texData->IsRequested = true;// Todo: Possible race condition with OnTexturePreperationCompleted, though i don't just want to lock this method.
	if (texData->HighResTexture != NULL){
		/*
		* Todo: Think about this. Requesting an already loaded texture might indicate a bug (especially since it might request an unload while still used)
		* But we also want some local caching to not reload all textures on sceene load. (guess unloading x mb for a new y mb texture?!)
		*/
		throw gcnew NotImplementedException("Texture seems already loaded. Todo: As we don't have caching yet, its probably a bug...");
	}
	if (texData->HighResFile != nullptr){
		StartLoadHighResTexture(textureId, texData);
	}
	else{
		Lock lock;
		try{
			lock.Take(textures);

			if (texData->HighResFile == nullptr){
				return;
			}
		}
		finally{
			lock.TryExit();
		}
		StartLoadHighResTexture(textureId, texData);
	}
}
void TextureManager::RequestTextureUnload(int textureId){
	TextureData^ texData = textures[textureId];
	if (!texData->IsRequested){
		throw gcnew InvalidOperationException("Cant unload a texture that wasn't requested for load in the first place.");
	}
	texData->IsRequested = false;
	if (texData->HighResTexture != NULL){
		// Todo: Caching, not just releasing!
		texData->HighResTexture->Release();
		texData->HighResTexture = NULL;
	}
	else{
		throw gcnew Exception("Todo/Debug: This shouldn't be possible anymore, i guess");
	}
}