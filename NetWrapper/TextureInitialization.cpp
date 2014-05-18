#include "stdafx.h"
#include "TextureInitialization.h"
#include "Work.h"
#include "Disk.h"
#include "ThumbnailTexture.h"
#include "FormatDatabase.h"
#include "GPU.h"

using namespace LodNative;


void TextureInitialization::TryVerifyThumb(){
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
	ThumbnailTexture^ thumb;
	try{
		thumb = gcnew ThumbnailTexture(loaded_data, isNormal, gcnew TextureDebugInfo(thumbFile));
		loaded_data = nullptr;
		if (thumb->Width != thumbWidth || thumb->Height != thumbHeight || thumb->Format != thumbFormat){
			throw gcnew FormatException("Loaded format (" + thumb->Width + "x" + thumb->Height + ", " + AssignableFormat::StringFromD3DFormat(thumb->Format) + ") does not match expected format (" + thumbWidth + "x" + thumbHeight + ", " + AssignableFormat::StringFromD3DFormat(thumbFormat) + ")");
		}
	}
	catch (Exception^ err){
		if (loaded_data != nullptr){
			loaded_data->Free();
		}
		Logger::LogText("Thumbnail cache is invalid: " + thumbFile + Environment::NewLine + "Error: " + err->ToString());
		File::Delete(thumbFile);
		hasToGenerateThumb = true;
		TryVerifyHighRes();
		return;
	}
	Logger::LogText("Thumb cache hit, loading " + thumbFile + " to " + textureId);
	GPU::AssignDataToThumbnailAsync(thumb, thumbTexture, textureId);
	delete thumb;
	TryVerifyHighRes();
}
void TextureInitialization::TryVerifyHighRes(){

	ingameDetailedFile = sourceFile;
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
	if (hasToGenerateHighRes || hasToGenerateThumb){
		Disk::RequestFile(sourceFile, gcnew Action<BufferMemory::ISegment^>(this, &TextureInitialization::Generate_OnLoaded));
	}
	else{
		InitializationComplete();
	}
}
void TextureInitialization::Generate_OnLoaded(BufferMemory::ISegment^ loaded_data){
	ITextureBase^ source_texture = nullptr;
	try{
		source_texture = FormatDatabase::Recognize(sourceFile, loaded_data);
	}
	catch (Exception^){
		loaded_data->Free();
		throw;
	}

	if (hasToGenerateThumb){
		GenerateThumb(hasToGenerateHighRes ? FormatDatabase::Clone(source_texture) : source_texture);
	}
	if (hasToGenerateHighRes){
		GenerateHighRes(source_texture);
	}

	InitializationComplete();
}

void TextureInitialization::GenerateThumb(ITextureBase^ loaded_texture){
	/*
	* Encode to readable format
	* Resize data
	* Encode to trg?
	* WriteToDisk
	* StartCompression?!
	*/
	ThumbnailTexture^ thumb = ThumbnailTexture::ConvertFromBitmap(FormatDatabase::ConvertTo<BitmapFormat^>(loaded_texture)->MayToNormal(isNormal), thumbWidth, thumbHeight, thumbFormat);
	Logger::LogText("Writing cache: " + thumbFile + (thumb->IsNormal ? " (Normal)" : " (Texture)"));
	Disk::WriteFile(thumbFile, thumb->GetFileBytes());
	GPU::AssignDataToThumbnailAsync(thumb, thumbTexture, textureId);
	delete thumb;
}

void TextureInitialization::GenerateHighRes(ITextureBase^ loaded_texture){
	throw gcnew NotImplementedException("This features doesn't yet exist, shouldn't be used and this error should not happen.");
	delete loaded_texture;
	/*
	* var trg = tex.ConvertTo....
	* trg.AssignTo(mytarget);
	* InitializationComplete();
	*//*
	});*/
}

void TextureInitialization::InitializationComplete(){
	onDoneCallback(this);
}



void TextureInitialization::Start(int texture_id, String^ source_file, String^ cache_id, String^ cache_dir, IDirect3DTexture9* thumbnail_texture, bool is_normal, Action<TextureInitialization^>^ on_done_callback){
	TextureInitialization^ ti = gcnew TextureInitialization();
	ti->started = false;
	ti->textureId = texture_id;
	ti->isNormal = is_normal;
	ti->sourceFile = source_file;
	ti->thumbFile = System::IO::Path::Combine(cache_dir, cache_id + ".THUMB");
	ti->thumbTexture = thumbnail_texture;
	ti->onDoneCallback = on_done_callback;
	D3DSURFACE_DESC thumbDesc;
	thumbnail_texture->GetLevelDesc(0, &thumbDesc);
	ti->thumbWidth = thumbDesc.Width;
	ti->thumbHeight = thumbDesc.Height;
	ti->thumbFormat = thumbDesc.Format;
	Work::ScheduleWork(gcnew Action(ti, &TextureInitialization::TryVerifyThumb));
}