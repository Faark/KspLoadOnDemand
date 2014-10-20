#include "stdafx.h"
#include "TextureInitialization.h"
#include "Work.h"
#include "Disk.h"
#include "CachedTexture.h"
#include "FormatDatabase.h"
#include "GPU.h"

using namespace LodNative;


void TextureInitialization::TryVerifyThumb(){
	Logger::LogText("TexInit" + textureId + ": TryVerifyThumb (thumb " + thumbFile + ", src " + sourceFile + ")");
	if (File::Exists(thumbFile))
	{
		Disk::RequestFile(thumbFile, gcnew Action<BufferMemory::ISegment^>(this, &TextureInitialization::TryVerifyThumb_OnLoaded));
	}
	else
	{
		hasToGenerateThumb = true;
		TryVerifyHighRes();
	}
}
void TextureInitialization::TryVerifyThumb_OnLoaded(BufferMemory::ISegment^ loaded_data){
	Logger::LogText("TexInit" + textureId + ": TryVerifyThumb_OnLoaded");
	CachedTexture^ thumb;
	try{
		thumb = gcnew CachedTexture(loaded_data, gcnew TextureDebugInfo(thumbFile, textureId));//gcnew CachedTexture(loaded_data, isNormal, 
		if (isNormal && !thumb->IsNormal )// Todo: Is has_to_be != isNormal reason enough?
		{
			throw gcnew FormatException("Thumb has to be a normal but isn't!");
		}
		auto hasToBeCompressed = !isNormal && imageProcessingSettings->ThumbnailCompress && FormatDatabase::CouldDXTCompressionMakesAnySense(thumb);
		if (hasToBeCompressed != ((thumb->Format == D3DFORMAT::D3DFMT_DXT1) || (thumb->Format == D3DFORMAT::D3DFMT_DXT5))){
			if (hasToBeCompressed){
				throw gcnew FormatException("Thumb should have been DXT compressed but isn't!");
			}
			else{
				throw gcnew FormatException("Thumb is DXT compressed but shouldn't!");
			}
		}
		loaded_data = nullptr;
		/*if (thumb->Width != thumbWidth || thumb->Height != thumbHeight || thumb->Format != thumbFormat){
			throw gcnew FormatException("Loaded format (" + thumb->Width + "x" + thumb->Height + ", " + DirectXStuff::StringFromD3DFormat(thumb->Format) + ") does not match expected format (" + thumbWidth + "x" + thumbHeight + ", " + DirectXStuff::StringFromD3DFormat(thumbFormat) + ")");
		}*/
	}
	catch (Exception^ err){
		if (loaded_data != nullptr){
			loaded_data->Free();
		}
		if (thumb != nullptr){
			delete thumb;
		}
		Logger::LogText("Thumbnail cache is invalid: " + thumbFile + Environment::NewLine + "Error: " + err->ToString());
		File::Delete(thumbFile);
		hasToGenerateThumb = true;
		TryVerifyHighRes();
		return;
	}
	Logger::LogText("Thumb cache hit, loading " + thumbFile + " to " + textureId);
	GPU::CreateTextureForAssignable(thumb, ThumbnailLoadedCallbackScope::Create(textureId));
	TryVerifyHighRes();
}
void TextureInitialization::TryVerifyHighRes(){
	if (imageProcessingSettings->CompressHighRes){
		ingameDetailedFile = System::IO::Path::Combine(cacheDir, cacheId + ".CACHE");
		if (!File::Exists(ingameDetailedFile)){
			hasToGenerateHighRes = true;
			Logger::LogText("TexInit" + textureId + ".TryVerifyHighRes: Has to build HighRes cache!");
		}
		else{
			// We do not actually verify high res pics, since they are large and doing so would probably take ages. Lets just check whether some exist, if necessary.
			// todo: its probably worth to at least load the header and check for stuff this way, but we so far don't have the infrastructure.
			//       Also load can fail at runtime anyway, so we need that logic nontheless.
			Logger::LogText("TexInit" + textureId + ".TryVerifyHighRes: There seems to be some cache file already. We'll try to use it once necessary!");
		}
	}
	else{
		Logger::LogText("TexInit" + textureId + ".TryVerifyHighRes: Using original file as configured.");
		ingameDetailedFile = sourceFile;
	}
	VerificationCompleted();
	//todo: throw new NotImplementedException();

	/*
	* COMPRESSION & NORMALS?!
	*
	* MISSING: SET TEXTURE PATH ONCE DONE AND CONSIDER LoadToGpuAfter...
	* InitializationComplete();
	*
	*
	* If (Possible/Neccessary)
	* {
	*   if( !Exists ){
	*     Generate();
	*   }else{
	*     do nothing. It will crash on actual load if invalid and we still have the thumb
	*   }
	* }else{
	*   throw notimplemented?
	* }
	*/
}
void TextureInitialization::VerificationCompleted(){
	Logger::LogText("TexInit" + textureId + ": VerificationCompleted");
	if (hasToGenerateHighRes || hasToGenerateThumb){
		Disk::RequestFile(sourceFile, gcnew Action<BufferMemory::ISegment^>(this, &TextureInitialization::Generate_OnLoaded));
	}
	else{
		InitializationComplete();
	}
}
void TextureInitialization::Generate_OnLoaded(BufferMemory::ISegment^ loaded_data){
	Logger::LogText("TexInit" + textureId + ": Generate_OnLoaded");
	ITextureBase^ source_texture = nullptr;
	try{
		source_texture = FormatDatabase::Recognize(sourceFile, loaded_data, textureId);
	}
	catch (Exception^){
		loaded_data->Free();
		throw;
	}

	if (hasToGenerateThumb){
		if (hasToGenerateHighRes){
			auto tmp = source_texture->ConvertTo<BitmapFormat^>();
			source_texture = tmp;
			GenerateThumb(tmp->Clone());
		}
		else{
			GenerateThumb(source_texture);
		}
	}
	if (hasToGenerateHighRes){
		GenerateHighRes(source_texture);
	}

	InitializationComplete();
}
void TextureInitialization::GenerateThumb(ITextureBase^ loaded_texture){
	Logger::LogText("TexInit" + textureId + ": GenerateThumb");
	/*
	* Encode to readable format
	* Resize data
	* Encode to trg?
	* WriteToDisk
	* StartCompression?!
	*/
	
	auto loadedThumb = FormatDatabase::ConvertTo<BitmapFormat^>(loaded_texture)
		->MayToNormal(isNormal)
		->ResizeWithAlphaFix(
			imageProcessingSettings->ThumbnailWidth,
			imageProcessingSettings->ThumbnailHeight
			);
	CachedTexture^ cacheThumb;
	if (!isNormal && imageProcessingSettings->ThumbnailCompress && FormatDatabase::CouldDXTCompressionMakesAnySense(loadedThumb)){
		cacheThumb = CachedTexture::GenerateDXTCompressionFromBitmap(loadedThumb);
	}
	else{
		cacheThumb = CachedTexture::ConvertFromBitmap(loadedThumb);
	}
	Logger::LogText("Writing cache: " + thumbFile + (cacheThumb->IsNormal ? " (Normal)" : " (Texture)"));
	Disk::WriteFile(thumbFile, cacheThumb->GetFileBytes());
	GPU::CreateTextureForAssignable(cacheThumb, ThumbnailLoadedCallbackScope::Create(textureId));
	//GPU::AssignDataToExistingTextureAsync(thumb, thumbTexture, textureId);
}
void TextureInitialization::GenerateHighRes(ITextureBase^ loaded_texture){
	Logger::LogText("TexInit" + textureId + ": GenerateHighRes");
	if (imageProcessingSettings->CompressHighRes){
		auto texture = CachedTexture::GenerateDXTCompressionFromBitmap(loaded_texture->ConvertTo<BitmapFormat^>()->MayToNormal(isNormal));
		Disk::WriteFile(ingameDetailedFile, texture->GetFileBytes());
		delete texture;
	}
	else{
		throw gcnew InvalidOperationException("No compression, so no reason to pre-process atm!");
	}
	/*
	* var trg = tex.ConvertTo....
	* trg.AssignTo(mytarget);
	* InitializationComplete();
	*//*
	});*/
}

void TextureInitialization::InitializationComplete(){
	Logger::LogText("TexInit" + textureId + ": InitializationComplete");
	ManagedBridge::TexturePrepared(textureId);
	onDoneCallback(this);
}



void TextureInitialization::Start(int texture_id, String^ source_file, String^ cache_id, String^ cache_dir, /*IDirect3DTexture9* thumbnail_texture,*/ bool is_normal, ImageSettings^ imageSettings, Action<TextureInitialization^>^ on_done_callback){
	Logger::LogText("TexInit" + texture_id + ": Start");
	TextureInitialization^ ti = gcnew TextureInitialization();
	ti->started = false;
	ti->textureId = texture_id;
	ti->isNormal = is_normal;
	ti->cacheDir = cache_dir;
	ti->cacheId = cache_id;
	ti->sourceFile = source_file;
	ti->thumbFile = System::IO::Path::Combine(cache_dir, cache_id + ".THUMB");
	//ti->thumbTexture = thumbnail_texture;
	ti->imageProcessingSettings = imageSettings;
	ti->onDoneCallback = on_done_callback;
	/*D3DSURFACE_DESC thumbDesc;
	thumbnail_texture->GetLevelDesc(0, &thumbDesc);
	ti->thumbWidth = thumbDesc.Width;
	ti->thumbHeight = thumbDesc.Height;
	ti->thumbFormat = thumbDesc.Format;*/
	if (imageSettings->ThumbnailEnabled){
		Work::ScheduleWork(gcnew Action(ti, &TextureInitialization::TryVerifyThumb));
	}
	else{
		Work::ScheduleWork(gcnew Action(ti, &TextureInitialization::TryVerifyHighRes));
		ManagedBridge::ThumbnailUpdated(texture_id, NULL);
	}
}