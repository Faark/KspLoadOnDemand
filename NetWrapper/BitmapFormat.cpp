#include "stdafx.h"
#include "BitmapFormat.h"
#include "GPU.h"
#include "BufferMemory.h"

using namespace LodNative;


BitmapFormat^ BitmapFormat::ToNormal(){
	if (IsNormal)
	{
		return this;
	}

	// Source: http://answers.unity3d.com/questions/47121/runtime-normal-map-import.html

	// Todo: We are going native anyway. Can't we use native copy and a better texture format?!
	int w = Bitmap->Width;
	int h = Bitmap->Height;
	Drawing::Bitmap^ newImg = gcnew Drawing::Bitmap(w, h, PixelFormat::Format32bppArgb);
	BitmapData^ bmpData = newImg->LockBits(Drawing::Rectangle(0, 0, w, h), ImageLockMode::WriteOnly, PixelFormat::Format32bppArgb);
	// Todo: bmpData->stride can be negative for bottom-up-imgs... don't know this code will react in such a case

	const int bytePerPixel = 4;
	unsigned char * lineStart = (unsigned char*)bmpData->Scan0.ToPointer();
	for (int y = 0; y < h; y++){
		unsigned char * pixelPos = lineStart;
		for (int x = 0; x < w; x++){
			Color c = Bitmap->GetPixel(x, y);
			*(pixelPos++) = c.R;
			*(pixelPos++) = c.G;
			*(pixelPos++) = c.G;
			*(pixelPos++) = c.G;
		}
		lineStart = lineStart + bmpData->Stride;
	}
	/*var remainingPixels = w* h;
	while (remainingPixels > 0)
	{
	var
	remainingPixels--;
	}
	var pos = 0;
	for (int x = 0; x < w; x++)
	{
	for (int y = 0; y < h; y++)
	{
	// Note / Todo: we could lock this as well, but would might hae to worry about different formats?! Also think about going native instead of using a tmp array...
	var c = Bitmap.GetPixel(x, y);
	targetData[pos++] = c.R;
	targetData[pos++] = c.G;
	targetData[pos++] = c.G;
	targetData[pos++] = c.G;
	//newImg.SetPixel(x, y, Color.FromArgb(c.R, c.G, c.G, c.G));
	}
	}

	System.Runtime.InteropServices.Marshal.Copy(targetData, 0, bmpData.Scan0, targetSize);*/
	newImg->UnlockBits(bmpData);
	return gcnew BitmapFormat(newImg, true, DebugInfo->Modify("ToNormal"));
}
BitmapFormat^ BitmapFormat::MayToNormal(bool toNormal){
	return toNormal ? ToNormal() : this;
}
BitmapFormat^ BitmapFormat::Resize(int new_width, int new_height){

	if (Bitmap->Width == new_width && Bitmap->Height == new_height)
	{
		return this;
	}
	Drawing::Bitmap^ newImg = gcnew Drawing::Bitmap(new_width, new_height, PixelFormat::Format32bppArgb);
	Graphics^ gr;
	try{
		gr = Graphics::FromImage(newImg);
		gr->SmoothingMode = SmoothingMode::HighQuality;
		gr->InterpolationMode = InterpolationMode::HighQualityBicubic;
		gr->PixelOffsetMode = PixelOffsetMode::HighQuality;
		gr->DrawImage(Bitmap, Drawing::Rectangle(0, 0, new_width, new_height));
		return gcnew BitmapFormat(newImg, IsNormal, DebugInfo->Modify("Resized"));
	}
	finally{
		if (gr != nullptr)
			delete gr;
	}
}
BitmapFormat^ BitmapFormat::ConvertTo(PixelFormat format){
	return gcnew BitmapFormat(Bitmap->Clone(Drawing::Rectangle(0, 0, Bitmap->Width, Bitmap->Height), format), IsNormal, DebugInfo->Modify("Converted"));
}
BitmapFormat^ BitmapFormat::ConvertTo(D3DFORMAT format){
	return ConvertTo(AssignableFormat::PixelFormatFromD3DFormat(format));
}
BitmapFormat^ BitmapFormat::SetAlpha(Byte new_alpha){
	//from http://stackoverflow.com/questions/6809442/how-to-load-transparent-png-to-bitmap-and-ignore-alpha-channel (kinda)

	int w = Bitmap->Width;
	int h = Bitmap->Height;
	auto fmt = Bitmap->PixelFormat;
	auto trgBmp = gcnew Drawing::Bitmap(w, h, fmt);
	auto trgData = trgBmp->LockBits(Drawing::Rectangle(0, 0, w, h), ImageLockMode::WriteOnly, PixelFormat::Format32bppArgb);
	auto srcData = Bitmap->LockBits(Drawing::Rectangle(0, 0, w, h), ImageLockMode::ReadWrite, PixelFormat::Format32bppArgb);

	auto srcLine = (unsigned char*)srcData->Scan0.ToPointer();
	auto trgLine = (unsigned char*)trgData->Scan0.ToPointer();
	for (int y = 0; y < h; y++){
		auto srcPos = srcLine;
		auto trgPos = trgLine;
		for (int x = 0; x < w; x++){
			*(trgPos++) = *(srcPos++);
			*(trgPos++) = *(srcPos++);
			*(trgPos++) = *(srcPos++);
			*(trgPos++) = new_alpha;
			srcPos++;
		}
		srcLine += srcData->Stride;
		trgLine += trgData->Stride;
	}

	trgBmp->UnlockBits(trgData);
	Bitmap->UnlockBits(srcData);
	return gcnew BitmapFormat(trgBmp, IsNormal, DebugInfo->Modify("SetAlpha" + new_alpha));
}

BitmapFormat^ BitmapFormat::LoadUnknownFile(FileInfo^ file, BufferMemory::ISegment^ data)
{
	MemoryStream^ ms;
	try
	{
		ms = data->CreateStream();
		BitmapFormat^ bmp = gcnew BitmapFormat(gcnew Drawing::Bitmap(Image::FromStream(ms)), false, gcnew TextureDebugInfo(file->FullName));
		if (FileNameIndicatesTextureShouldBeNormal(file))
			return bmp->ToNormal();
		return bmp;
	}
	catch (Exception^ err)
	{
		throw gcnew Exception("Unknown format of file: " + file->FullName, err);
	}
	finally{
		delete ms;
		data->Free();
	}
}


AssignableFormat^ BitmapFormat::GetAssignableFormat(){
	return gcnew AssignableFormat(Bitmap->Width, Bitmap->Height, AssignableFormat::D3DFormatFromPixelFormat(Bitmap->PixelFormat));
}

void BitmapFormat::BitmapAssignableData::AssignTo(AssignableTarget^ target){
	auto bmpData = bmp->LockBits(Drawing::Rectangle(0, 0, bmp->Width, bmp->Height), ImageLockMode::ReadOnly, bmp->PixelFormat);
	GPU::CopyRawBytesTo(target, (unsigned char*)bmpData->Scan0.ToPointer(), bmpData->Stride, bmpData->Width * AssignableFormat::GetByteDepthForFormat(AssignableFormat::D3DFormatFromPixelFormat(bmpData->PixelFormat)), bmpData->Height, IsUpsideDown, Debug);
	bmp->UnlockBits(bmpData);
}

AssignableData^ BitmapFormat::GetAssignableData(AssignableFormat^ assignableFormat, Func<AssignableFormat^, AssignableData^>^ %delayedCb){
	if (assignableFormat != nullptr){
		throw gcnew NotImplementedException();
	}
	return gcnew BitmapAssignableData(GetAssignableFormat(), Bitmap, DebugInfo);
}


