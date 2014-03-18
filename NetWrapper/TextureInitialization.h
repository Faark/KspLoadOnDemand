#pragma once

#include "ITexture.h"

using namespace System;
using namespace System::IO;
// Delayed startup routine for textures...
ref class TextureInitialization
{
private:
	int textureId;
	int started;
	bool isNormal;
	String^ sourceFile; // source file to init from
	ITextureBase^ sourceFileLoaded;
	String^ thumbFile;
	IDirect3DTexture9* thumbTexture;
	int thumbWidth;
	int thumbHeight;
	D3DFORMAT thumbFormat;
	String^ ingameDetailedFile; // texture to be loaded in use...
	Action<TextureInitialization^>^ onDoneCallback;
public:
	property int TextureId{ 
		int get(){ return textureId; }
	}
	property bool Started{
		bool get(){ return started == 1; }
	}
	property String^ HighResolutionTextureFile {
		String^ get(){ return ingameDetailedFile; }
	}

private:
	ref class GetOrLoadSourceFileScope{
	public:
		TextureInitialization^ Self;
		Action<ITextureBase^>^ Callback;
		GetOrLoadSourceFileScope(TextureInitialization^ self, Action<ITextureBase^>^ callback){
			Self = self;
			Callback = callback;
		}
		void OnLoaded(array<Byte>^ loaded_data);
	};
	void GetOrLoadSourceFile(Action<ITextureBase^>^ onLoadedCallback);

	void ThumbInitialize();
	void ThumbTryLoadFromDisk();
	void ThumbTryLoadFromDisk_OnLoaded(array<Byte>^ loaded_data);
	void ThumbGenerate();
	void ThumbGenerate_OnLoaded(ITextureBase^ loaded_texture);
	void CompressionInitialize();
	void CompressionGenerate();
	void CompressionGenerate_OnLoaded(ITextureBase^ loaded_texture);
	void InitializationComplete();
	TextureInitialization();
public:
	static void Start(int texture_id, String^ source_file, String^ cache_id, String^ cache_dir, IDirect3DTexture9* thumbnail_texture, bool is_normal, Action<TextureInitialization^>^ on_done_callback);
};
