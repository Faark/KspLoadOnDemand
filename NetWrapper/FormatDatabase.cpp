#include "Stdafx.h"
#include "FormatDatabase.h"


#include "ThumbnailTexture.h"
#include "MBMTexture.h"

static FormatDatabase::FormatDatabase(){
	AddRecognition(gcnew Func<System::IO::FileInfo^, array<Byte>^, MBMTexture^>(&MBMTexture::Recignizer));
	AddConversion(gcnew Func<ThumbnailTexture^, BitmapFormat^>( &ThumbnailTexture::ConvertToBitmap));
	AddConversion(gcnew Func<MBMTexture^, BitmapFormat^>(&MBMTexture::ConvertToBitmap));
}