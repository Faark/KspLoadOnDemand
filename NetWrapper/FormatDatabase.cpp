#include "Stdafx.h"
#include "FormatDatabase.h"


#include "ThumbnailTexture.h"
#include "MBMTexture.h"
#include "TargaImage.h"


BitmapFormat^ FormatDatabase::RecognizeTGA(FileInfo^ file, array<Byte>^ data){
	if (file->Extension->ToUpper() == ".TGA"){
		Paloma::TargaImage^ img = nullptr;
		try{
			auto img = gcnew Paloma::TargaImage();
			img->LoadTGAFromMemory(data);
			return (gcnew BitmapFormat(gcnew Bitmap(img->Image), false, gcnew TextureDebugInfo(file->FullName)))->MayToNormal(BitmapFormat::FileNameIndicatesTextureShouldBeNormal(file));
		}
#if NDEBUG
		catch (Exception^ err){
			throw gcnew Exception(
				"Failed to load TGA file: " + file->FullName
				+ (img != nullptr ? (img->Image != nullptr ? (", " + img->Image->Width + "x" + img->Image->Height + "@" + img->Image->PixelFormat.ToString()) : "") : "")
				, err);
		}
#endif
		finally{
			if (img != nullptr)
				delete img;
		}
		//return gcnew BitmapFormat(gcnew Bitmap(16, 16, PixelFormat::Format32bppArgb), false, gcnew TextureDebugInfo("FakeTGA"));
	}
	return nullptr;
}
static FormatDatabase::FormatDatabase(){
	AddRecognition(gcnew Func<FileInfo^, array<Byte>^, MBMTexture^>(&MBMTexture::Recignizer));
	AddRecognition(gcnew Func<FileInfo^, array<Byte>^, BitmapFormat^>(&FormatDatabase::RecognizeTGA));
	AddConversion(gcnew Func<ThumbnailTexture^, BitmapFormat^>(&ThumbnailTexture::ConvertToBitmap));
	AddConversion(gcnew Func<MBMTexture^, BitmapFormat^>(&MBMTexture::ConvertToBitmap));

}