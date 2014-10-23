#pragma once

using namespace System;
using namespace System::IO;

#include "Stuff.h"
//#include "../crnlib/inc/crnlib.h"
#include "../../../Visual Studio 2013/Projects/squish-1.11/squish.h"
// C:\\Users\\Faark\\Documents\\Visual Studio 2013\\Projects\\squish-1.11\\squish.h

#include "ITexture.h"
#include "BitmapFormat.h"
#include "AssignableData.h"
#include "BufferMemory.h"

namespace LodNative{
	// A wapper for raw DX data... currently uncompressed & e.g. shitty named...

	/*
	Ready for the GPU, aka Upside down (Unity) and in a proper formats
	*/
	ref class CachedTexture : IAssignableTexture{
	private:
		/*
		* Header File-Format (all BinaryReader, 13 bytes in total):
		* - Verification 
		* - mip levels? Nope, or after format. [A]RGB is 0 (aka auto). DXT is currently 1
		* int32 width
		* int32 height
		* int32 format => ATM 153 for ARGB32 or sth like that
		* byte isNormal { 1, 0 }
		*
		* Raw Pixels (looks like their byte pattern is B G R A, btw!
		*
		*/
		static const int BytesPerPixel = 4; // Todo: This is crap, lets make it dynamic to support other formats as well.
		array<Byte>^ byteData;
		bool isNormal;
		/*
		int width;
		int height;
		D3DFORMAT format;
		*/
		AssignableFormat format;
		BufferMemory::ISegment^ segm = nullptr;
		//int array_offset = 0; // todo: what the hell is this? Some old offset remains? Then kill it! Update: Got it! Its what the offset of ISegment should have been, but never worked in the first place. Merging it with ByteDataHeaderOffset, i guess
	public:
		static const int MinHeaderSize = 13; // todo: Can we do this dynamic? Or at least update it to the current value.
		int ByteDataOffset;
		property UINT Width{UINT get() override { return format.Width; }}
		property UINT Height{UINT get() override{ return format.Height; }}
		property D3DFORMAT Format{D3DFORMAT get(){ return format.Format; }}
		property bool IsNormal{bool get() override { return isNormal; }}
		array<Byte>^ GetFileBytes(){
			if (segm == nullptr){
				return byteData;
			}
			else{
				throw gcnew InvalidOperationException("Cannot get file bytes for a loaded file. (" + DebugInfo + ")");
			}
		}
		CachedTexture(BufferMemory::ISegment^ data, TextureDebugInfo^ debug_info) :IAssignableTexture(debug_info){
			if (data->SegmentLength < MinHeaderSize)
			{
				throw gcnew FormatException("Given cached image data is invalid. It doesn't even have the " + MinHeaderSize + " bytes required for a header (length is " + data->SegmentLength + " bytes)");
			}
			MemoryStream^ ms;
			BinaryReader^ bs;
			try{
				ms = data->CreateStream();
				bs = gcnew BinaryReader(ms);

				if (bs->ReadString() != "LODC"){
					throw gcnew FormatException("Given file is not a valid CacheTexture (Signature doesn't match)");
				}
				auto width = bs->ReadInt32();
				auto height = bs->ReadInt32();
				auto fmt = (D3DFORMAT)bs->ReadInt32();
				format = AssignableFormat(width, height, 0, fmt);
				isNormal = bs->ReadByte() != 0;
				ByteDataOffset = (int)ms->Position; // +data->SegmentStartsAt; // or 13 + 
				//Logger::LogText("Embeded: " + bs->ReadString());
				//ByteDataHeaderOffset = (int)ms->Position;
				Logger::LogText("Using stream offset as segment array offset: " + ByteDataOffset);
			}
			finally{
				if (bs != nullptr)
					delete bs;
				if (ms != nullptr)
					delete ms;
			}
			int expectedSize = -1;
			switch (format.Format){
			case D3DFORMAT::D3DFMT_DXT1:
			case D3DFORMAT::D3DFMT_DXT5:
				format.Levels = 1;
				expectedSize = ByteDataOffset + squish::GetStorageRequirements(format.Width, format.Height, format.Format == D3DFORMAT::D3DFMT_DXT1 ? squish::kDxt1 : squish::kDxt5);
				if ((Width % 4 != 0) || (Height % 4 != 0)){
					throw gcnew FormatException("Width & Height must be divisble by 4 for DXT images!");
				}
				break;
			case D3DFORMAT::D3DFMT_A8R8G8B8:
			case D3DFORMAT::D3DFMT_X8R8G8B8:
				expectedSize = ByteDataOffset + (format.Width * format.Height * BytesPerPixel);
				break;
			default:
				throw gcnew FormatException("Unknown or unsupported format: " + DirectXStuff::StringFromD3DFormat(format.Format));
			}
			if (expectedSize != data->SegmentLength)
			{
				throw gcnew FormatException("Invalid data size. Expected " + expectedSize + " byte, got " + data->SegmentLength + " byte (both including a " + ByteDataOffset + " byte header)");
			}
			Logger::LogText("FromMem/ Creating thumb " + format.Width + "x" + format.Height + "@" + DirectXStuff::StringFromD3DFormat(format.Format) + (isNormal ? " (NORMAL)" : " (NO NORMAL)") + ", path: " + DebugInfo->ToString());


			ByteDataOffset = data->SegmentStartsAt + ByteDataOffset;
			byteData = data->EntireBuffer;
			Log(this);
			segm = data;
		}
	private:
		CachedTexture(int data_bytes, AssignableFormat fmt, bool isNormal, TextureDebugInfo^ debug_info) : 
			IAssignableTexture(debug_info), 
			format(fmt){
			auto logText = "FromScratch/ Creating thumb " + format.Width + "x" + format.Height + "@" + DirectXStuff::StringFromD3DFormat(format.Format) + (isNormal? " (NORMAL)" : " (NO NORMAL)") + ", path: " + DebugInfo->ToString();
			Logger::LogText(logText);

			this->isNormal = isNormal;
			//this->byteData = gcnew array<Byte>(ByteDataOffset + data_bytes);

			MemoryStream^ ms;
			BinaryWriter^ bw;
			try{
				//auto tmp = gcnew MemoryStream();
				ms = gcnew MemoryStream(/*byteData*/);
				bw = gcnew BinaryWriter(ms);
				bw->Write("LODC");
				bw->Write((Int32)Width);
				bw->Write((Int32)Height);
				bw->Write((Int32)Format);
				bw->Write((Byte)(IsNormal ? 1 : 0));
				/* this was crap if (ByteHeaderOffset != ms->Position){
					throw gcnew Exception("internal bug #324916192832");
				}*/
				//bw->Write(logBytes, 0, logBytes->Length);
				//ByteDataHeaderOffset = (int)ms->Position;
				ByteDataOffset = (int)ms->Position;

				this->byteData = gcnew array<Byte>(ByteDataOffset + data_bytes);

				ms->Position = 0;
				ms->Read(this->byteData, 0, ByteDataOffset);
			}
			finally{
				if (bw != nullptr)
					delete bw;
				if (ms != nullptr)
					delete ms;
			}
		}
	public:
		static CachedTexture^ ConvertFromBitmap(BitmapFormat^ source){
			Logger::LogText("ToCache: " + source->DebugInfo->ToString());
			auto w = source->Bitmap->Width;
			auto h = source->Bitmap->Height;
			auto newThumb = gcnew CachedTexture(
				w * h * BytesPerPixel,
				AssignableFormat(
					w,
					h,
					0,
					D3DFORMAT::D3DFMT_A8R8G8B8
					//DirectXStuff::D3DFormatFromPixelFormat(source->Bitmap->PixelFormat)
				),
				source->IsNormal,
				source->DebugInfo->Modify("ToCached")
				);
			array<Byte>^ bytes = newThumb->byteData;
			int pos = newThumb->ByteDataOffset;
			Logger::LogText("ConvertingToCached(w=" + w + ",h=" + h + ",pos=" + pos + ",len=" + bytes->LongLength + ",badSize=" + ((bytes->LongLength < (pos + (w*h * 4)))) + ")");
			// Todo: With correct locking i should be able to this pretty much a memcpy?! (Stuff::CopyLine...)
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					System::Drawing::Color pixel = source->Bitmap->GetPixel(x, y);
					//Logger::LogText("Pixel " + x + "|" + y + "[@" + pos + "] = {A:" + pixel.A + ",R:" + pixel.R + ",G:" + pixel.G + ",B:" + pixel.B + "}");
					bytes[pos++] = pixel.B;
					bytes[pos++] = pixel.G;
					bytes[pos++] = pixel.R;
					bytes[pos++] = pixel.A;
				}
			}
			return Log(newThumb);
		}

		// Todo: This method has a problem with non-fitting textures. They can't use the same 0-1 texture coordinates. Probably have to resize the img to /4 first.
		static CachedTexture^ GenerateDXTCompressionFromBitmap(BitmapFormat^ source){

			/*
			auto t = gcnew String("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Kerbal Space Program\\GameData\\");
			auto s = source->DebugInfo->TexFile->Substring(t->Length)->Replace("\\", "_");
			auto o = System::IO::Path::Combine(gcnew String("C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Kerbal Space Program\\GameData\\LoadOnDemand\\Debug"), s + ".ori.png");
			auto m = System::IO::Path::Combine(gcnew String("C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Kerbal Space Program\\GameData\\LoadOnDemand\\Debug"), s + ".mod.png");
			
			source->Bitmap->Save(o);
			*/
			//Logger::LogText("DEBUG_STUFF: " + source->DebugInfo->TexFile);
			//source->Bitmap->Save()
			auto bmp = source->Bitmap;
			int w = bmp->Width;
			int h = bmp->Height;
			if ((w % 4 != 0) || (h % 4 != 0)){
				throw gcnew NotSupportedException("Width & Height must be divisible by 4 for DXT, no automatic adjustment exists so far.");
			}
			auto srcData = bmp->LockBits(Drawing::Rectangle(0, 0, w, h), ImageLockMode::ReadOnly, PixelFormat::Format32bppArgb);


			// DXT textures came out wrongly colored, hope this does correct it. (What should have been yellow was blue)
			unsigned char* line = (unsigned char*)srcData->Scan0.ToPointer();
			for (int y = 0; y < h; y++){
				unsigned char* pos = line;
				for (int x = 0; x < w; x++){
					unsigned char tmp = *(pos + 0);
					*(pos + 0) = *(pos + 2);
					*(pos + 2) = tmp;
					pos += 4;
				}
				line += srcData->Stride;
			}

			
			auto useDxt5 = source->CheckForAlphaChannelData(srcData);

			int flags = squish::kColourIterativeClusterFit | squish::kColourMetricPerceptual | squish::kWeightColourByAlpha | (useDxt5 ? squish::kDxt5 : squish::kDxt1);
			auto size = squish::GetStorageRequirements(w, h, flags);
			auto newTexture = gcnew CachedTexture(size, AssignableFormat(w, h, 1, useDxt5 ? D3DFORMAT::D3DFMT_DXT5 : D3DFORMAT::D3DFMT_DXT1), source->IsNormal, source->DebugInfo->Modify("GenerateDXTCompression"));
			pin_ptr<unsigned char> src_ptr = &newTexture->byteData[newTexture->ByteDataOffset];
			squish::CompressImage((const squish::u8*)srcData->Scan0.ToPointer(), w, h, src_ptr, flags);
			bmp->UnlockBits(srcData);
			delete source;

			/*auto dbgImg = gcnew Bitmap(w, h, PixelFormat::Format32bppArgb);
			auto dbgData = dbgImg->LockBits(Drawing::Rectangle(0, 0, w, h), ImageLockMode::ReadWrite, PixelFormat::Format32bppArgb);

			squish::DecompressImage((squish::u8*)(dbgData->Scan0.ToPointer()), w, h, src_ptr, newTexture->format.Format == D3DFORMAT::D3DFMT_DXT1 ? squish::kDxt1 : squish::kDxt5);

			dbgImg->UnlockBits(dbgData);
			dbgImg->Save(m);*/


			return newTexture;
		}
		static BitmapFormat^ ConvertToBitmap(CachedTexture^ source){
			auto format = DirectXStuff::PixelFormatFromD3DFormat(source->Format);
			auto bmp = gcnew Bitmap(source->Width, source->Height, format);
			auto bytes = source->byteData;
			int pos = source->ByteDataOffset;
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


		virtual AssignableFormat GetAssignableFormat() override{
			return format;
		}

		virtual void AssignToTarget(D3DLOCKED_RECT* trg) override{
			// GPU thread!
			//Stuff::CopyLineBuffer
			//trg->Pitch
			pin_ptr<unsigned char> src_ptr = &byteData[ByteDataOffset];
			if (format.Format == D3DFORMAT::D3DFMT_DXT1){
				auto pitch = (Width >> 2) /*blocks per "line"*/ * 8 /*block size*/;
				//Logger::LogText("DXT1:CopyLineBuffer(" + IntPtr(src_ptr) + ", " + pitch + ", " + IntPtr((unsigned char*)trg->pBits) + ", " + trg->Pitch + ", " + pitch + ", " + Height / 4 + ")");
				Stuff::CopyLineBuffer(src_ptr, pitch, (unsigned char*)trg->pBits, trg->Pitch, pitch, Height / 4);
			}else if (format.Format == D3DFORMAT::D3DFMT_DXT5){
				auto pitch = (Width >> 2) /*blocks per "line"*/ * 16 /*block size*/;
				//Logger::LogText("DXT5:CopyLineBuffer(" + IntPtr(src_ptr) + ", " + pitch + ", " + IntPtr((unsigned char*)trg->pBits) + ", " + trg->Pitch + ", " + pitch + ", " + Height / 4 + ")");
				Stuff::CopyLineBuffer(src_ptr, pitch, (unsigned char*)trg->pBits, trg->Pitch, pitch, Height / 4);
			}
			else{

				/*auto t = gcnew String("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Kerbal Space Program\\GameData\\");
				auto s = DebugInfo->TexFile->Substring(t->Length)->Replace("\\", "_");
				auto o = System::IO::Path::Combine(gcnew String("C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Kerbal Space Program\\GameData\\LoadOnDemand\\Debug"), s + "_" + DateTime::Now.Ticks + ".png");
				
				auto logBmp = gcnew Bitmap(Width, Height, PixelFormat::Format32bppArgb);
				auto logData = logBmp->LockBits(Drawing::Rectangle(0, 0, Width, Height), ImageLockMode::WriteOnly, PixelFormat::Format32bppArgb);
				*/auto pitch = BytesPerPixel * Width;
				Stuff::CopyLineBuffer(src_ptr, pitch, (unsigned char*)trg->pBits, trg->Pitch, pitch, Height);
				/*Stuff::CopyLineBuffer(src_ptr, pitch, (unsigned char*)logData->Scan0.ToPointer(), logData->Stride, pitch, Height);
				logBmp->UnlockBits(logData);
				logBmp->Save(o);*/
			}
		}
		virtual IAssignableTexture^ ConvertToAssignableFormat(AssignableFormat fmt) override{ // Work thread!
			throw gcnew NotSupportedException("Not supported in this Version. Interesting that its actually needed. Please do report this!");
		}

		/*virtual AssignableData^ GetAssignableData(AssignableFormat^ assignableFormat, Func<AssignableFormat^, AssignableData^>^ %delayedCb) override{
			if (assignableFormat != nullptr){
				Logger::LogText("DEBUG: HORRIBLE SYNC CONVERSION IN PLACE. REMOVE THAT ASAP ONCE DEBUGGING IS DONE!");
				return FormatDatabase::GetConverterJob(this)(assignableFormat);
				throw gcnew NotSupportedException("Cannot assign ThumbnailTextures to an different surface (Got " + GetAssignableFormat() + ", trg is " + assignableFormat + ")");
			}
			return gcnew ByteArrayAssignableData(GetAssignableFormat(), byteData, Width * BytesPerPixel, ByteDataOffset, DebugInfo);
		}*/

		~CachedTexture(){
			if (segm != nullptr){
				segm->Free();
			}
			byteData = nullptr;
		}


		static CachedTexture^ TryRecognizeFile(System::IO::FileInfo^ file, BufferMemory::ISegment^ data, int textureId){
			auto ex = file->Extension->ToUpperInvariant();
			if (ex == ".CACHE" || ex == ".THUMB"){
				try{
					return gcnew CachedTexture(data, gcnew TextureDebugInfo(file->FullName, textureId));
				}
				catch (Exception^){
					return nullptr;
				}
			}
			return nullptr;
		}
	};
}