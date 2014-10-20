#pragma once

using namespace System;

#include "Stuff.h"
#include "ITexture.h"
#include "BitmapFormat.h"
#include "AssignableData.h"

namespace LodNative{
	ref class MBMTexture : IAssignableTexture{

		array<Byte>^ RawData;
		bool isNormal;
		UINT width;
		UINT height;
	public:
		property UINT Width { UINT get() override { return width; } }
		property UINT Height { UINT get() override { return height; } }
		property bool IsNormal{ bool get() override { return isNormal; } }

		MBMTexture(BufferMemory::ISegment^ data, TextureDebugInfo^ tdi) :IAssignableTexture(tdi) {
			System::IO::MemoryStream^ ms = nullptr;
			System::IO::BinaryReader^ bs = nullptr;
			int data_offset = 0;
			D3DFORMAT dataFormat;
			try{
				ms = data->CreateStream();
				bs = gcnew System::IO::BinaryReader(ms);
				bs->ReadString();
				width = bs->ReadInt32();
				height = bs->ReadInt32();
				isNormal = bs->ReadInt32() == 1;
				int bitPerPixel = bs->ReadInt32();
				switch (bitPerPixel){
				case 32:
					dataFormat = D3DFORMAT::D3DFMT_A8R8G8B8;
					break;
				case 24:
					dataFormat = D3DFORMAT::D3DFMT_R8G8B8;
					break;
				}
				data_offset = (int)ms->Position;
				if (data->SegmentLength != ((Width * Height * DirectXStuff::GetByteDepthForFormat(dataFormat)) + data_offset))
				{
					throw gcnew FormatException("Invalid data size. Expected " + ((Width * Height * DirectXStuff::GetByteDepthForFormat(dataFormat)) + data_offset) + " byte, got " + data->SegmentLength + " byte (both including a " + data_offset + " byte header)");
				}
			}
			finally{
				if (bs != nullptr)
					delete bs;
				if (ms != nullptr)
					delete ms;
			}
			RawData = gcnew array<Byte>(Width * Height * 4);

			pin_ptr<unsigned char> srcPinPtr = &(data->EntireBuffer[data->SegmentStartsAt + data_offset]);
			pin_ptr<unsigned char> trgPinPtr = &RawData[0];
			unsigned char* srcPtr = srcPinPtr;
			unsigned char* trgPtr = trgPinPtr;
			int remainingPixelCount = Width * Height;

			switch (dataFormat){
			case D3DFORMAT::D3DFMT_A8R8G8B8:
				while (remainingPixelCount > 0){
					auto r = (*srcPtr);
					srcPtr++;
					auto g = (*srcPtr);
					srcPtr++;
					auto b = (*srcPtr);
					srcPtr++;
					auto a = (*srcPtr);
					srcPtr++;
					(*trgPtr) = b;
					trgPtr++;
					(*trgPtr) = g;
					trgPtr++;
					(*trgPtr) = r;
					trgPtr++;
					(*trgPtr) = a;
					trgPtr++;
					remainingPixelCount--;
				}
				break;
			case D3DFORMAT::D3DFMT_R8G8B8:
				while (remainingPixelCount > 0){
					auto r = (*srcPtr);
					srcPtr++;
					auto g = (*srcPtr);
					srcPtr++;
					auto b = (*srcPtr);
					srcPtr++;
					(*trgPtr) = b;
					trgPtr++;
					(*trgPtr) = g;
					trgPtr++;
					(*trgPtr) = r;
					trgPtr++;
					(*trgPtr) = 255;
					trgPtr++;
					remainingPixelCount--;
				}
				break;
			}
			data->Free();
		}

		virtual AssignableFormat GetAssignableFormat() override{
			return AssignableFormat(Width, Height, 0, DirectXStuff::D3DFormatFromPixelFormat(PixelFormat::Format32bppArgb));
		}
		virtual void AssignToTarget(D3DLOCKED_RECT* trg) override{
			// GPU thread!
			pin_ptr<unsigned char> src_ptr = &RawData[0];
			auto pitch = 4 * Width;
			Stuff::CopyLineBuffer(src_ptr, pitch, (unsigned char*)trg->pBits, trg->Pitch, pitch, Height);
		}
		virtual IAssignableTexture^ ConvertToAssignableFormat(AssignableFormat fmt) override{
			// Work thread!
			throw gcnew NotSupportedException("Not supported in this Version. Interesting that its actually needed. Please do report this!");
		}


		/*
		virtual AssignableFormat^ GetAssignableFormat() override{
			return gcnew AssignableFormat(Width, Height, D3DFORMAT::D3DFMT_A8R8G8B8);
		}
		virtual AssignableData^ GetAssignableData(AssignableFormat ^ fmt, System::Func<AssignableFormat ^, AssignableData ^> ^% delayedCb) override{
			if (fmt != nullptr){
				throw gcnew NotSupportedException("Cannot convert to other formats, atm (requested: " + fmt + ")");
			}
			auto tmp = gcnew ByteArrayAssignableData(GetAssignableFormat(), RawData, Width * 4, 0, DebugInfo);
			tmp->IsUpsideDown = true;
			return tmp;
		}
		*/

		BitmapFormat^ ToBitmap(){
			auto w = Width;
			auto h = Height;
			auto fmt = PixelFormat::Format32bppArgb;
			auto bmp = gcnew Bitmap(w, h, fmt);
			auto bmpData = bmp->LockBits(Drawing::Rectangle(0, 0, w, h), ImageLockMode::WriteOnly, fmt);
			pin_ptr<unsigned char> srcPinPtr = &RawData[0];

			int lineSize = Width * 4;
			// inverted?! Guess that means MBMs are already upside down? Lets just copy it for now...
			// GPU::CopyBufferInverted(srcPinPtr, lineSize, (unsigned char*)bmpData->Scan0.ToPointer(), bmpData->Stride, lineSize, h);
			Stuff::CopyLineBuffer(srcPinPtr, lineSize, (unsigned char*)bmpData->Scan0.ToPointer(), bmpData->Stride, lineSize, h);

			bmp->UnlockBits(bmpData);
			auto isN = IsNormal;
			auto di = DebugInfo;
			delete this;
			return gcnew BitmapFormat(bmp, isN, di->Modify("ToBitmap"));
		}
		static BitmapFormat^ ConvertToBitmap(MBMTexture^ from){
			return from->ToBitmap();
		}
		static MBMTexture^ TryRecognizeFile(System::IO::FileInfo^ file, BufferMemory::ISegment^ data, int textureId)
		{
			if (file->Extension->ToUpper() == ".MBM")
			{
				return gcnew MBMTexture(data, gcnew TextureDebugInfo(file->FullName, textureId));
			}
			return nullptr;
		}

	};
}