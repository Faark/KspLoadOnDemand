#pragma once

#include "ITexture.h"
#include "BufferMemory.h"
#include "ManagedBridge.h"


using namespace System;
using namespace System::IO;


namespace LodNative{
	ref class ImageSettings{
	public:
		bool ThumbnailEnabled;
		int ThumbnailWidth;
		int ThumbnailHeight;
		bool ThumbnailCompress;
		bool CompressHighRes;
	};
	/**
		Work of a texture init:
		- try load thumb from cache.
		- check wether texture cache is required 
		 - if it is, try to verify it
		- if any verification failed load from disk
		 - if both failed create copy of texture for second
		 - run both

	    if the "high res preperation" part gets implemented we have to consider order of operation stuff!
	*/

	// Delayed startup routine for textures...
	ref class TextureInitialization
	{
	private:
		int textureId;
		int started;
		bool isNormal;
		String^ sourceFile; // source file to init from
		String^ thumbFile;
		String^ cacheDir;
		String^ cacheId;
		//IDirect3DTexture9* thumbTexture;
		//int thumbWidth;
		//int thumbHeight;
		//D3DFORMAT thumbFormat;
		String^ ingameDetailedFile; // texture to be loaded in use...
		ImageSettings^ imageProcessingSettings;
		Action<TextureInitialization^>^ onDoneCallback;

		bool hasToGenerateThumb = false;
		bool hasToGenerateHighRes = false;
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
		property String^ SourceFile{
			String^ get(){ return sourceFile; }
		}

	private:
		void TryVerifyThumb();
		void TryVerifyThumb_OnLoaded(BufferMemory::ISegment^ loaded_data);
		void TryVerifyHighRes();
		void VerificationCompleted();
		void Generate_OnLoaded(BufferMemory::ISegment^ loaded_data);
		void GenerateThumb(ITextureBase^ source_texture);
		void GenerateHighRes(ITextureBase^ source_texture);
		void InitializationComplete();
		TextureInitialization() { }

		ref class ThumbnailLoadedCallbackScope{
		private:
			int textureId;
		public:
			ThumbnailLoadedCallbackScope(int texture_id) : textureId(texture_id){}
			void Run(IntPtr texturePtr){
				ManagedBridge::ThumbnailUpdated(textureId, (IDirect3DTexture9*)texturePtr.ToPointer());
			}
			static Action<IntPtr>^ Create(int texture_id){
				return gcnew Action<IntPtr>(gcnew ThumbnailLoadedCallbackScope(texture_id), &Run);
			}
		};

	public:
		static void Start(int texture_id, String^ source_file, String^ cache_id, String^ cache_dir, /*IDirect3DTexture9* thumbnail_texture, */bool is_normal, ImageSettings^ imageSettings,  Action<TextureInitialization^>^ on_done_callback);
	};
}