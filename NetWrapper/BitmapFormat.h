#pragma once



using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Drawing::Imaging;
using namespace System::IO;

#include "ITexture.h"
#include "BufferMemory.h"

namespace LodNative{
	ref class BitmapFormat : IAssignableTexture{
		Bitmap^ imgData;
		bool isNormal;
	public:
		BitmapFormat(Bitmap^ img, bool is_normal, TextureDebugInfo^ info) :IAssignableTexture(info){
			imgData = img;
			isNormal = is_normal;
			Log(this);
		}
		property bool IsNormal{bool get(){ return isNormal; } }
		property Bitmap^ Bitmap{Drawing::Bitmap^ get(){ return imgData; } }

		BitmapFormat^ ToNormal();
		BitmapFormat^ MayToNormal(bool toNormal);
		BitmapFormat^ Resize(int new_width, int new_height);
		BitmapFormat^ ConvertTo(PixelFormat format);
		BitmapFormat^ ConvertTo(D3DFORMAT format);
		BitmapFormat^ SetAlpha(Byte new_alpha);

		static BitmapFormat^ LoadUnknownFile(FileInfo^ file, BufferMemory::ISegment^ data);
		static bool FileNameIndicatesTextureShouldBeNormal(FileInfo^ file){
			return Path::GetFileNameWithoutExtension(file->FullName)->ToUpper()->EndsWith("NRM");
		}

		virtual  AssignableFormat^ GetAssignableFormat() override;
	private:
		ref class BitmapAssignableData : AssignableData{
			Drawing::Bitmap^ bmp;
		public:
			BitmapAssignableData(AssignableFormat^ format, Drawing::Bitmap^ bitmap, TextureDebugInfo^ info) :AssignableData(format, info){
				bmp = bitmap;
			}
			virtual void AssignTo(AssignableTarget^ target) override;
		};
	public:
		virtual AssignableData^ GetAssignableData(AssignableFormat^ assignableFormat, Func<AssignableFormat^, AssignableData^>^ %delayedCb) override;
	};

}