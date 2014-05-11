#pragma once

using namespace System;
using namespace System::IO;

#include "ITexture.h"
#include "BitmapFormat.h"
#include "AssignableData.h"
#include "BufferMemory.h"

namespace LodNative{
	// A wapper for raw DX data... currently uncompressed & e.g. shitty named...
	ref class ThumbnailTexture : IAssignableTexture{
	private:
		/*
		* Header File-Format (all BinaryReader, 13 bytes in total):
		* int32 width
		* int32 height
		* int32 format => ATM 153 for ARGB32 or sth like that
		* byte isNormal { 1, 0 }
		*
		* Raw Pixels (looks like their byte pattern is B G R A, btw!
		*
		*/
		const int BytesPerPixel = 4;
		array<Byte>^ byteData;
		bool isNormal;
		int width;
		int height;
		D3DFORMAT format;
		BufferMemory::ISegment^ segm = nullptr;
		int array_offset = 0;
	public:
		static const int ByteDataHeaderOffset = 13;
		property int Width{int get(){ return width; }}
		property int Height{int get(){ return height; }}
		property D3DFORMAT Format{D3DFORMAT get(){ return format; }}
		property bool IsNormal{bool get(){ return isNormal; }}
		array<Byte>^ GetFileBytes(){
			return byteData;
		}
		ThumbnailTexture(BufferMemory::ISegment^ data, bool has_to_be_normal, TextureDebugInfo^ debug_info) :IAssignableTexture(debug_info){
			segm = data;
			if (data->SegmentLength < ByteDataHeaderOffset)
			{
				throw gcnew FormatException("Given thumbnail data is invalid. It doesn't even have the " + ByteDataHeaderOffset + " bytes for the header (length: " + data->SegmentLength + ")");
			}
			MemoryStream^ ms;
			BinaryReader^ bs;
			try{
				ms = data->CreateStream();
				bs = gcnew BinaryReader(ms);

				width = bs->ReadInt32();
				height = bs->ReadInt32();
				format = (D3DFORMAT)bs->ReadInt32();
				isNormal = bs->ReadByte() != 0;
			}
			finally{
				if (bs != nullptr)
					delete bs;
				if (ms != nullptr)
					delete ms;
			}
			if (has_to_be_normal && !IsNormal) // Todo: Is has_to_be != isNormal reason enough?
			{
				throw gcnew Exception("Thumb has to be a normal but isn!");
			}
			if (format != D3DFORMAT::D3DFMT_A8R8G8B8)
				throw gcnew NotSupportedException("Thumbnails currently have to be in ARGB32 format.");

			int expectedSize = ByteDataHeaderOffset + (width * height * BytesPerPixel);
			if (expectedSize != data->SegmentLength)
			{
				throw gcnew FormatException("Invalid data size. Expected " + expectedSize + " byte, got " + data->SegmentLength + " byte (both including a " + ByteDataHeaderOffset + " byte header)");
			}

			byteData = data->EntireBuffer;
			Log(this);
		}
	private:
		ThumbnailTexture(int thumb_width, int thumb_height, D3DFORMAT thumb_format, bool is_normal, TextureDebugInfo^ debug_info) : IAssignableTexture(debug_info){
			if (thumb_format != D3DFORMAT::D3DFMT_A8R8G8B8){
				throw gcnew NotSupportedException("Thumbnails currently have to be in ARGB32 format.");
			}
			width = thumb_width;
			height = thumb_height;
			isNormal = is_normal;
			byteData = gcnew array<Byte>(ByteDataHeaderOffset + (Width * Height * BytesPerPixel));
			format = thumb_format;


			MemoryStream^ ms;
			BinaryWriter^ bw;
			try{
				ms = gcnew MemoryStream(byteData);
				bw = gcnew BinaryWriter(ms);
				bw->Write((Int32)Width);
				bw->Write((Int32)Height);
				bw->Write((Int32)Format);
				bw->Write((Byte)(IsNormal ? 1 : 0));
			}
			finally{
				if (bw != nullptr)
					delete bw;
				if (ms != nullptr)
					delete ms;
			}
		}
	public:
		static ThumbnailTexture^ ConvertFromBitmap(BitmapFormat^ source){

			auto newThumb = gcnew ThumbnailTexture(
				source->Bitmap->Width,
				source->Bitmap->Height,
				AssignableFormat::D3DFormatFromPixelFormat(source->Bitmap->PixelFormat),
				source->IsNormal,
				source->DebugInfo->Modify("ToThumb")
				);
			array<Byte>^ bytes = newThumb->byteData;
			int pos = newThumb->array_offset + newThumb->ByteDataHeaderOffset;
			// Todo: With correct locking i should be able to this pretty much a memcpy?!
			for (int y = 0; y < source->Bitmap->Height; y++)
			{
				for (int x = 0; x < source->Bitmap->Width; x++)
				{
					System::Drawing::Color pixel = source->Bitmap->GetPixel(x, y);
					bytes[pos++] = pixel.B;
					bytes[pos++] = pixel.G;
					bytes[pos++] = pixel.R;
					bytes[pos++] = pixel.A;
				}
			}
			return Log(newThumb);
		}
		static ThumbnailTexture^ ConvertFromBitmap(BitmapFormat^ source, int width, int height, D3DFORMAT format){
			/*
			Todo: I skipped this "recommendation" check for now and just ignore the format... it would make sense, though, to create them in the designed format!
			if (format != D3DFORMAT::D3DFMT_A8R8G8B8){
			throw gcnew NotSupportedException("TextureFormat has to be ARGB32 atm. Requested format is:" + AssignableFormat::StringFomD3DFormat(format));
			}*/
			// Todo: First thing we do is venting the alpha channel, since parts seem to ignore it anyway even with junk in it but it makes colors go lost on processing... Find out what this breaks and how to prevent it from breaking/ any better solution

			if (!source->IsNormal){
				source = source->SetAlpha(255);
			}
			return ConvertFromBitmap(source->Resize(width, height));
		}
		static BitmapFormat^ ConvertToBitmap(ThumbnailTexture^ source){
			auto format = AssignableFormat::PixelFormatFromD3DFormat(source->Format);
			auto bmp = gcnew Bitmap(source->Width, source->Height, format);
			auto bytes = source->byteData;
			int pos = source->array_offset + source->ByteDataHeaderOffset;
			/*auto sb = gcnew System::Text::StringBuilder();
			Logger::LogText("Start con to bmp");
			sb->Append("Raw Data: {");
			for (int x = 0; x < bytes->Length; x++){
			sb->Append(bytes[x])->Append(",");
			}
			sb->Append("done}");
			Logger::LogText(sb->ToString());*/
			for (int y = 0; y < bmp->Height; y++)
			{
				//Logger::LogText("new line");
				for (int x = 0; x < bmp->Width; x++)
				{
					Byte b = bytes[pos++];
					Byte g = bytes[pos++];
					Byte r = bytes[pos++];
					Byte a = bytes[pos++];
					auto color = Drawing::Color::FromArgb(a, r, g, b);
					//Logger::LogText("Writing color: " + color.ToString());
					bmp->SetPixel(x, y, color);
				}
			}
			//Logger::LogText("con done");
			return gcnew BitmapFormat(bmp, source->IsNormal, source->DebugInfo->Modify("ToBitmap"));
		}


		virtual AssignableFormat^ GetAssignableFormat() override{
			return gcnew AssignableFormat(Width, Height, Format);
		}
		virtual AssignableData^ GetAssignableData(AssignableFormat^ assignableFormat, Func<AssignableFormat^, AssignableData^>^ %delayedCb) override{
			if (assignableFormat != nullptr){
				Logger::LogText("DEBUG: HORRIBLE SYNC CONVERSION IN PLACE. REMOVE THAT ASAP ONCE DEBUGGING IS DONE!");
				return FormatDatabase::GetConverterJob(this)(assignableFormat);
				throw gcnew NotSupportedException("Cannot assign ThumbnailTextures to an different surface (Got " + GetAssignableFormat() + ", trg is " + assignableFormat + ")");
			}
			return gcnew ByteArrayAssignableData(GetAssignableFormat(), byteData, Width * BytesPerPixel, array_offset + ByteDataHeaderOffset, DebugInfo);
		}

		~ThumbnailTexture(){
			if (segm != nullptr){
				segm->Free();
			}
		}
	};
}