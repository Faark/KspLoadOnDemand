#pragma once

using namespace System;
using namespace System::Collections::Generic;

#include "TextureInitialization.h"
#include "FormatDatabase.h"
#include "ManagedBridge.h"

// All public call to non-properties have to come from the Unity-Thread!
// Todo: Crappy behavior on quick load & unload calls... especially spamming. Sure, a local cache might help plus we shouldn't load the same multiple times
ref class TextureManager
{
private:
	ref class TextureData{
	public:
		bool IsNormal;
		bool IsRequested = false;// Todo: This flag shouldn't be required anymore. See usage for details
		IDirect3DTexture9* HighResTexture = NULL;
		String^ HighResFile;
	};
	static String^ mCacheDir;
	static List<TextureData^>^ textures = gcnew List<TextureData^>();
	//static List<TextureInitialization^>^ textureCurrentlyInitializing = gcnew List<TextureInitialization^>();
	static ConcurrentDictionary<int, TextureData^>^ TexturesToLoadQueue = gcnew ConcurrentDictionary<int, TextureData^>();

	ref class StartLoadHighResTextureScope{
		int textureId;
		TextureData^ textureData;
	public:
		StartLoadHighResTextureScope(int texture_id, TextureData^ texture_data){
			textureId = texture_id;
			textureData = texture_data;
		}
		void ProcessLoadedData(array<Byte>^ loaded_data);
	};
	static void StartLoadHighResTexture(int textureId, TextureData^ data);
	static void OnTexturePreperationCompleted(TextureInitialization^ tex_init);

public:
	static property String^ CacheDir{
		String^ get(){ return mCacheDir; }
	}

	static void Setup(String^ cache_directory);
	static int RegisterTexture(String^ file, String^ cache, IDirect3DTexture9* thumb, bool normal);
	static void RequestTextureLoad(int id);
	static void RequestTextureUnload(int id);

internal:
	static void OnTextureLoadedToGPU(IDirect3DTexture9* texture, int textureId){
		auto textureData = textures[textureId];
		if (textureData->IsRequested && (textureData->HighResTexture == nullptr)){
			textureData->HighResTexture = texture;
			Logger::LogText("Loaded Texture " + textureId + ", informing KSP runtme");
			ManagedBridge::TextureLoaded(textureId, texture);
		}
		else{
			texture->Release();
			Logger::LogText("Dropping loaded texture for ID: " + textureId);
		}
	}
};

