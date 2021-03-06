#pragma once



using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Drawing::Imaging;
using namespace System::IO;

#include "ITexture.h"
#include "BufferMemory.h"
#include "Stuff.h"

// Todo: We might be able to improve this class quite a bit by not locking and releasing the BitmapData over and over again. Why don't we let BitmapFormat handle this more intelligently?
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

		property UINT Width {UINT get() override { return Bitmap->Width; }}
		property UINT Height {UINT get() override { return Bitmap->Height; }}
		property bool IsNormal{bool get() override { return isNormal; } }
		property Bitmap^ Bitmap{Drawing::Bitmap^ get(){ return imgData; } }

		BitmapFormat^ ToNormal();
		BitmapFormat^ MayToNormal(bool toNormal);
		BitmapFormat^ Resize(int new_width, int new_height);
		BitmapFormat^ ResizeWithAlphaFix(int new_width, int new_height);
		BitmapFormat^ ConvertTo(PixelFormat format);
		BitmapFormat^ ConvertTo(D3DFORMAT format);
		BitmapFormat^ Clone();
		void SetAlpha(Byte new_alpha);
		void FlipVertical();
		bool CheckForAlphaChannelData(BitmapData^ bmpData);
		bool CheckForAlphaChannelData();
		

		static BitmapFormat^ LoadUnknownFile(FileInfo^ file, BufferMemory::ISegment^ data, int textureId);
		static bool FileNameIndicatesTextureShouldBeNormal(FileInfo^ file){
			return Path::GetFileNameWithoutExtension(file->FullName)->ToUpper()->EndsWith("NRM");
		}
		virtual AssignableFormat GetAssignableFormat() override;
		virtual void AssignToTarget(D3DLOCKED_RECT* trg) override;
		virtual IAssignableTexture^ ConvertToAssignableFormat(AssignableFormat fmt) override;

		/*
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
		~BitmapFormat(){
			delete imgData;
			imgData = nullptr;
		}*/
	};

}