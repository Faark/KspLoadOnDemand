#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Collections::Concurrent;

#include "TextureInitialization.h"
#include "FormatDatabase.h"
#include "ManagedBridge.h"
#include "Disk.h"

namespace LodNative{
	// All public call to non-properties have to come from the Unity-Thread!
	// Todo: Crappy behavior on quick load & unload calls... especially spamming. Sure, a local cache might help plus we shouldn't load the same multiple times
	ref class TextureManager
	{
	private:
		ref class TextureData{
		public:
			bool IsNormal;
			bool IsRequested = false;// Todo: This flag shouldn't be required anymore. See usage for details | UPDATE: Its at least used to prevent load until prepared. This has to be reworked anyway.
			IDirect3DTexture9* HighResTexture = NULL;
			String^ HighResFile;
			String^ SourceFile;
		};
		ref class StartLoadHighResTextureScope{
			int textureId;
			TextureData^ textureData;
		public:
			StartLoadHighResTextureScope(int texture_id, TextureData^ texture_data){
				textureId = texture_id;
				textureData = texture_data;
			}
			void ProcessLoadedData(BufferMemory::ISegment^ loaded_data);
		};
		ref class LoadQueue : Disk::IoRequestProvider{
		private:
			ConcurrentDictionary<int, TextureData^>^ Queue = gcnew ConcurrentDictionary<int, TextureData^>();
		public:
			void RequestLoad(int textureId, TextureData^ textureData){
				if (!Queue->TryAdd(textureId, textureData)){
					throw gcnew System::Exception("This should be impossible.");
				}
			}
			bool CancelLoadRequest(int textureId){
				TextureData^ value;
				return Queue->TryRemove(textureId, value);
			}
			virtual bool TryGetIoRequest(Disk::IoRequest% request){
				auto en = Queue->GetEnumerator();
				try{
					if (en->MoveNext()){
						int textureId = en->Current.Key;
						TextureData^ data;
						if (Queue->TryRemove(textureId, data)){
							request = Disk::IoRequest(data->HighResFile, gcnew Action<BufferMemory::ISegment^>(gcnew StartLoadHighResTextureScope(textureId, data), &StartLoadHighResTextureScope::ProcessLoadedData));
							return true;
						}
						else{
							// this looks like a race situation... lets just retry?
							return TryGetIoRequest(request);
						}
					}
					return false;
				}
				finally{
					delete en;
				}
			}
		};

		static String^ mCacheDir;
		static List<TextureData^>^ textures = gcnew List<TextureData^>();
		//static List<TextureInitialization^>^ textureCurrentlyInitializing = gcnew List<TextureInitialization^>();
		static ConcurrentDictionary<int, TextureData^>^ TexturesToLoadQueue = gcnew ConcurrentDictionary<int, TextureData^>();
		static LoadQueue^ mLoadQueue = gcnew LoadQueue();

		//static void StartLoadHighResTexture(int textureId, TextureData^ data);
		static void OnTexturePreperationCompleted(TextureInitialization^ tex_init);

	public:
		static property String^ CacheDir{
			String^ get(){ return mCacheDir; }
		}

		static void Setup(String^ cache_directory);
		static int RegisterTexture(String^ file, String^ cache, /*IDirect3DTexture9* thumb,*/ bool normal, ImageSettings^ imageSettings);
		static void RequestTextureLoad(int id);
		static bool CancelTextureLoad(int id);
		static void RequestTextureUnload(int id);

	private:
		ref class TextureLoadedCallbackScope{
		public:
			int textureId;
			TextureLoadedCallbackScope(int texture_id) :textureId(texture_id){}
			void Run(IntPtr texturePtr){
				OnTextureLoadedToGPU((IDirect3DTexture9*)texturePtr.ToPointer(), textureId);
			}
		};
		static void OnTextureLoadedToGPU(IDirect3DTexture9* texture, int textureId){
			auto textureData = textures[textureId];
			if (textureData->IsRequested && (textureData->HighResTexture == nullptr)){
				textureData->HighResTexture = texture;
				Logger::LogText("Loaded Texture " + textureId + ", informing KSP runtime");
				ManagedBridge::TextureLoaded(textureId, texture);
			}
			else{
				texture->Release();
				Logger::LogText("Dropping loaded texture for ID: " + textureId);
			}
		}
	};

}