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
		finally{
			if (img != nullptr)
				delete img;
		}
	}
	return nullptr;
}
static FormatDatabase::FormatDatabase(){
	AddRecognition(gcnew Func<FileInfo^, array<Byte>^, MBMTexture^>(&MBMTexture::Recignizer));
	AddRecognition(gcnew Func<FileInfo^, array<Byte>^, BitmapFormat^>(&FormatDatabase::RecognizeTGA));
	AddConversion(gcnew Func<ThumbnailTexture^, BitmapFormat^>(&ThumbnailTexture::ConvertToBitmap));
	AddConversion(gcnew Func<MBMTexture^, BitmapFormat^>(&MBMTexture::ConvertToBitmap));
}