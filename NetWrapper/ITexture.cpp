#include "Stdafx.h"
#include "ITexture.h"
#include "FormatDatabase.h"
#include "BitmapFormat.h"

ITextureBase::ITextureBase(TextureDebugInfo^ debug_info){
	DebugInfo = debug_info;
}
generic <class T> where T: ITextureBase
T ITextureBase::Log(T texture){
	return texture;
	if (!dontLogConverts){
		try{
			auto debug_info = texture->DebugInfo;
			dontLogConverts = true;
			FormatDatabase::ConvertTo<BitmapFormat^>(texture)->Bitmap->Save(debug_info->LogFileBaseName + "_" + DateTime::Now.Ticks + "_" + debug_info->Modifiers + ".png", ImageFormat::Png);
		}
		catch (Exception^ err){
			Logger::LogText("Logfail!");
			Logger::LogException(err);
		}
		finally{
			dontLogConverts = false;
		}
	}
	return texture;
}


generic <class T> where T: ITextureBase 
T ITextureBase::ConvertTo()
{
	return FormatDatabase::ConvertTo<T>(this);
}