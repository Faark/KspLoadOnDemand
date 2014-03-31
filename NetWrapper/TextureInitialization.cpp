#include "stdafx.h"
#include "TextureInitialization.h"
#include "Work.h"
#include "Disk.h"
#include "ThumbnailTexture.h"
#include "FormatDatabase.h"
#include "GPU.h"


void TextureInitialization::GetOrLoadSourceFileScope::OnLoaded(array<Byte>^ loaded_data){
	Callback(Self->sourceFileLoaded = FormatDatabase::Recognize(Self->sourceFile, loaded_data));
}
void TextureInitialization::GetOrLoadSourceFile(Action<ITextureBase^>^ onLoadedCallback){
	if (sourceFileLoaded == nullptr){
		Disk::RequestFile(sourceFile, gcnew Action<array<Byte>^>(gcnew GetOrLoadSourceFileScope(this, onLoadedCallback), &GetOrLoadSourceFileScope::OnLoaded));
	}
	else
	{
		onLoadedCallback(sourceFileLoaded);
	}
}

void TextureInitialization::ThumbInitialize(){
	if (File::Exists(thumbFile))
	{
		ThumbTryLoadFromDisk();
	}
	else
	{
		ThumbGenerate();
	}
}


void TextureInitialization::ThumbTryLoadFromDisk(){
	Disk::RequestFile(thumbFile, gcnew Action<array<Byte>^>(this, &TextureInitialization::ThumbTryLoadFromDisk_OnLoaded));
}

void TextureInitialization::ThumbTryLoadFromDisk_OnLoaded(array<Byte>^ loaded_data){
	ThumbnailTexture^ thumb;
	try{
		thumb = gcnew ThumbnailTexture(loaded_data, isNormal, gcnew TextureDebugInfo(thumbFile));
		if (thumb->Width != thumbWidth || thumb->Height != thumbHeight || thumb->Format != thumbFormat){
			throw gcnew FormatException("Loaded format (" + thumb->Width + "x" + thumb->Height + ", " + AssignableFormat::StringFromD3DFormat(thumb->Format) + ") does not match expected format (" + thumbWidth + "x" + thumbHeight + ", " + AssignableFormat::StringFromD3DFormat(thumbFormat) + ")");
		}
	}
	catch (Exception^ err){
		Logger::LogText("Thumbnail cache is invalid: " + thumbFile + Environment::NewLine + "Error: " + err->ToString());
		File::Delete(thumbFile);
		ThumbGenerate();
		return;
	}
	Logger::LogText("Thumb cache hit, loading " + thumbFile + " to " + textureId);
	GPU::AssignDataToThumbnailAsync(thumb, thumbTexture, textureId);
	CompressionInitialize();
}

void TextureInitialization::ThumbGenerate(){
	GetOrLoadSourceFile(gcnew Action<ITextureBase^>(this, &TextureInitialization::ThumbGenerate_OnLoaded));
}
void TextureInitialization::ThumbGenerate_OnLoaded(ITextureBase^ loaded_texture){
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
	CompressionInitialize();
}

void TextureInitialization::CompressionInitialize(){
	ingameDetailedFile = sourceFile;
	InitializationComplete();
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

void TextureInitialization::CompressionGenerate(){
	GetOrLoadSourceFile(gcnew Action<ITextureBase^>(this, &TextureInitialization::CompressionGenerate_OnLoaded));
}
void TextureInitialization::CompressionGenerate_OnLoaded(ITextureBase^ loaded_texture){
	throw gcnew NotImplementedException("No texture compression, atm!");
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
	Work::ScheduleWork(gcnew Action(ti, &TextureInitialization::ThumbInitialize));
}

TextureInitialization::TextureInitialization()
{
}
