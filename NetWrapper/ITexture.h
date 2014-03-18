#pragma once

using namespace System;
using namespace System::Drawing::Imaging;

#include "Logger.h"


ref class TextureDebugInfo{
private:
	static int ActiveIndex = 0;
	TextureDebugInfo(){}
public:
	String^ LogFileBaseName;
	String^ File;
	String^ Modifiers;
	String^ LastChange;

	static String^ ToFileName(String^ text){
		for each(auto illegal in System::IO::Path::GetInvalidFileNameChars()){
			text = text->Replace(illegal.ToString(), "");
		}
		return text;
	}

	TextureDebugInfo(String^ f){

		LogFileBaseName = ToFileName("LodDebug_" + DateTime::Now.ToString()) + "_" + System::Threading::Interlocked::Increment(ActiveIndex);
		LastChange = "";
		File = f;
		Modifiers = "";
	}
	TextureDebugInfo^ Modify(String^ text){
		auto newInfo = gcnew TextureDebugInfo();
		newInfo->File = File;
		newInfo->LastChange = text;
		newInfo->Modifiers = Modifiers + "-" + text;
		newInfo->LogFileBaseName = LogFileBaseName;
		return newInfo;
	}
};
ref class ITextureBase abstract{
internal:
	[ThreadStatic]
	static bool dontLogConverts;
public:
	TextureDebugInfo^ DebugInfo;
	ITextureBase(TextureDebugInfo^ debug_info);
	generic <class T> where T: ITextureBase static T Log(T texture);
	generic <class T> where T: ITextureBase T ConvertTo();
};
ref class AssignableFormat{
private:
	int width;
	int height;
	D3DFORMAT format;
public:
	property int Width{int get(){ return width; }}
	property int Height{int get(){ return height; }}
	property D3DFORMAT Format { D3DFORMAT get(){ return format; }}
	AssignableFormat(int width, int height, D3DFORMAT format){
		this->width = width;
		this->height = height;
		this->format = format;
	}

	static BYTE GetByteDepthForFormat(D3DFORMAT fmtFormat){
		return (BYTE)ceil(GetBitDepthForFormat(fmtFormat) / 8.0);
	}
	static BYTE GetBitDepthForFormat(D3DFORMAT fmtFormat)
	{
		switch (fmtFormat)
		{
		case D3DFMT_DXT1:
			return 4;
		case D3DFMT_R3G3B2:
		case D3DFMT_A8:
		case D3DFMT_P8:
		case D3DFMT_L8:
		case D3DFMT_A4L4:
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:
			return 8;
		case D3DFMT_X4R4G4B4:
		case D3DFMT_A4R4G4B4:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_R5G6B5:
		case D3DFMT_A8R3G3B2:
		case D3DFMT_A8P8:
		case D3DFMT_A8L8:
		case D3DFMT_V8U8:
		case D3DFMT_L6V5U5:
		case D3DFMT_D16_LOCKABLE:
		case D3DFMT_D15S1:
		case D3DFMT_D16:
		case D3DFMT_L16:
		case D3DFMT_INDEX16:
		case D3DFMT_CxV8U8:
		case D3DFMT_G8R8_G8B8:
		case D3DFMT_R8G8_B8G8:
		case D3DFMT_R16F:
			return 16;
		case D3DFMT_R8G8B8:
			return 24;
		case D3DFMT_A2W10V10U10:
		case D3DFMT_A2B10G10R10:
		case D3DFMT_A2R10G10B10:
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8L8V8U8:
		case D3DFMT_Q8W8V8U8:
		case D3DFMT_V16U16:
			//case D3DFMT_W11V11U10: // Dropped in DX9.0
		case D3DFMT_UYVY:
		case D3DFMT_YUY2:
		case D3DFMT_G16R16:
		case D3DFMT_D32:
		case D3DFMT_D24S8:
		case D3DFMT_D24X8:
		case D3DFMT_D24X4S4:
		case D3DFMT_D24FS8:
		case D3DFMT_D32F_LOCKABLE:
		case D3DFMT_INDEX32:
		case D3DFMT_MULTI2_ARGB8:
		case D3DFMT_G16R16F:
		case D3DFMT_R32F:
			return 32;
		case D3DFMT_A16B16G16R16:
		case D3DFMT_Q16W16V16U16:
		case D3DFMT_A16B16G16R16F:
		case D3DFMT_G32R32F:
			return 64;
		case D3DFMT_A32B32G32R32F:
			return 128; // !
		}
		return 0;
	}


	static PixelFormat PixelFormatFromD3DFormat(D3DFORMAT from){
		switch (from){
		case D3DFORMAT::D3DFMT_A8R8G8B8:
			return PixelFormat::Format32bppArgb;
		case D3DFORMAT::D3DFMT_X8R8G8B8:
			return PixelFormat::Format32bppRgb;
		default:
			throw gcnew Exception("Unrecognized format: "+ StringFromD3DFormat(from));
		}
	}
	static D3DFORMAT D3DFormatFromPixelFormat(PixelFormat from){
		switch (from){
		case PixelFormat::Format32bppArgb:
			return D3DFORMAT::D3DFMT_A8R8G8B8;
		case PixelFormat::Format32bppRgb:
			return D3DFORMAT::D3DFMT_X8R8G8B8;
		default:
			throw gcnew Exception("Unrecognized format: " + from.ToString());
		}
	}
	static String^ StringFromD3DFormat(D3DFORMAT from){
		switch (from){
		case D3DFMT_UNKNOWN: return "D3DFMT_UNKNOWN";
		case D3DFMT_R8G8B8: return "D3DFMT_R8G8B8";
		case D3DFMT_A8R8G8B8:return "D3DFMT_A8R8G8B8";
		case D3DFMT_X8R8G8B8:return "D3DFMT_X8R8G8B8";
		case D3DFMT_R5G6B5:return "D3DFMT_R5G6B5";
		case D3DFMT_X1R5G5B5:return "D3DFMT_X1R5G5B5";
		case D3DFMT_A1R5G5B5:return "D3DFMT_A1R5G5B5";
		case D3DFMT_A4R4G4B4:return "D3DFMT_A4R4G4B4";
		case D3DFMT_R3G3B2:return "D3DFMT_R3G3B2";
		case D3DFMT_A8:return "D3DFMT_A8";
		case D3DFMT_A8R3G3B2:return "D3DFMT_A8R3G3B2";
		case D3DFMT_X4R4G4B4:return "D3DFMT_X4R4G4B4";
		case D3DFMT_A2B10G10R10:return "D3DFMT_A2B10G10R10";
		case D3DFMT_A8B8G8R8:return "D3DFMT_A8B8G8R8";
		case D3DFMT_X8B8G8R8:return "D3DFMT_X8B8G8R8";
		case D3DFMT_G16R16: return "D3DFMT_G16R16";
		case D3DFMT_A2R10G10B10:return "D3DFMT_A2R10G10B10";
		case D3DFMT_A16B16G16R16:return "D3DFMT_A16B16G16R16";

		case D3DFMT_A8P8:return "D3DFMT_A8P8";
		case D3DFMT_P8:return "D3DFMT_P8";

		case D3DFMT_L8:return "D3DFMT_L8";
		case D3DFMT_A8L8:return "D3DFMT_A8L8";
		case D3DFMT_A4L4:return "D3DFMT_A4L4";

		case D3DFMT_V8U8:return "D3DFMT_V8U8";
		case D3DFMT_L6V5U5:return "D3DFMT_L6V5U5";
		case D3DFMT_X8L8V8U8:return "D3DFMT_X8L8V8U8";
		case D3DFMT_Q8W8V8U8:return "D3DFMT_Q8W8V8U8";
		case D3DFMT_V16U16:return "D3DFMT_V16U16";
		case D3DFMT_A2W10V10U10:return "D3DFMT_A2W10V10U10";

		case D3DFMT_UYVY:return "D3DFMT_UYVY";
		case D3DFMT_R8G8_B8G8: return "D3DFMT_R8G8_B8G8";
		case D3DFMT_YUY2:return "D3DFMT_YUY2";
		case D3DFMT_G8R8_G8B8:return "D3DFMT_G8R8_G8B8";
		case D3DFMT_DXT1:return "D3DFMT_DXT1";
		case D3DFMT_DXT2:return "D3DFMT_DXT2";
		case D3DFMT_DXT3:return "D3DFMT_DXT3";
		case D3DFMT_DXT4:return "D3DFMT_DXT4";
		case D3DFMT_DXT5:return "D3DFMT_DXT5";

		case D3DFMT_D16_LOCKABLE:return "D3DFMT_D16_LOCKABLE";
		case D3DFMT_D32:return "D3DFMT_D32";
		case D3DFMT_D15S1:return "D3DFMT_D15S1";
		case D3DFMT_D24S8:return "D3DFMT_D24S8";
		case D3DFMT_D24X8:return "D3DFMT_D24X8";
		case D3DFMT_D24X4S4:return "D3DFMT_D24X4S4";
		case D3DFMT_D16:return "D3DFMT_D16";


		case D3DFMT_D32F_LOCKABLE:return "D3DFMT_D32F_LOCKABLE";
		case D3DFMT_D24FS8:return "D3DFMT_D24FS8";


		case D3DFMT_D32_LOCKABLE:return "D3DFMT_D32_LOCKABLE";
		case D3DFMT_S8_LOCKABLE:return "D3DFMT_S8_LOCKABLE";

		case D3DFMT_L16:return "D3DFMT_L16";

		case D3DFMT_VERTEXDATA:return "D3DFMT_VERTEXDATA";
		case D3DFMT_INDEX16:return "D3DFMT_INDEX16";
		case D3DFMT_INDEX32:return "D3DFMT_INDEX32";

		case D3DFMT_Q16W16V16U16:return "D3DFMT_Q16W16V16U16";

		case D3DFMT_MULTI2_ARGB8:return "D3DFMT_MULTI2_ARGB8";

		case D3DFMT_R16F:return "D3DFMT_R16F";
		case D3DFMT_G16R16F:return "D3DFMT_G16R16F";
		case D3DFMT_A16B16G16R16F:return "D3DFMT_A16B16G16R16F";

		case D3DFMT_R32F:return "D3DFMT_R32F";
		case D3DFMT_G32R32F:return "D3DFMT_G32R32F";
		case D3DFMT_A32B32G32R32F:return "D3DFMT_A32B32G32R32F";

		case D3DFMT_CxV8U8:return "D3DFMT_CxV8U8";

		case D3DFMT_A1:return "D3DFMT_A1";

		case D3DFMT_A2B10G10R10_XR_BIAS:return "D3DFMT_A2B10G10R10_XR_BIAS";

		case D3DFMT_BINARYBUFFER:return "D3DFMT_BINARYBUFFER";

		case D3DFMT_FORCE_DWORD:return "D3DFMT_FORCE_DWORD";

		default:
			return ("UNKNOWN_FORMAT_" + ((int)from).ToString());
		}
	}

	//AssignableFormat(){}
	/*AssignableFormat(AssignableFormat% af){}

	static bool operator==(AssignableFormat a, AssignableFormat b)
	{
		return a.Width == b.Width && a.Height == b.Height && a.Format == b.Format;
	}
	static bool operator!=(AssignableFormat a, AssignableFormat b)
	{
		return a.Width != b.Width || a.Height != b.Height || a.Format != b.Format;
	}
	*/
	bool SameAs(AssignableFormat^ other){
		auto isSame = (Width == other->Width) && (Height == other->Height) && (Format == other->Format);
		Logger::LogText((isSame ? "We got a AF match" : "No AF match") + ", a: " + this + ", b: " + other);
		return isSame;
	}
	String^ ToString() override{
		return Width + "x" + Height + "@" + (StringFromD3DFormat(Format));
	}
	AssignableFormat^ Copy(){
		return gcnew AssignableFormat(Width, Height, Format);
	}
	property BYTE BytesPerPixel{BYTE get(){ return GetByteDepthForFormat(Format); }}
	property BYTE BitsPerPixel{BYTE get(){ return GetBitDepthForFormat(Format); }}

};
ref class AssignableTarget{
public:
	property AssignableFormat^ Format;
	property int Pitch;
	property unsigned char* Bits;
	property bool Inverted;
	AssignableTarget(AssignableFormat^ format, D3DLOCKED_RECT* locked_rect, bool inverted){
		Format = format;
		Pitch = locked_rect->Pitch;
		Bits = (unsigned char*)locked_rect->pBits;
		Inverted = inverted;
	}
};
ref class AssignableData abstract{
	AssignableFormat^ fmt;
public:
	property TextureDebugInfo^ Debug;
	property bool IsUpsideDown;
	property AssignableFormat^ Format{ AssignableFormat^ get(){ return fmt; } }
	AssignableData(AssignableFormat^ format, TextureDebugInfo^ debug_info){
		IsUpsideDown = false;
		fmt = format;
		Debug = debug_info;
	}

	virtual void AssignTo(AssignableTarget^ target) abstract;
};

ref class IAssignableTexture abstract : ITextureBase {
public:
	virtual AssignableFormat^ GetAssignableFormat() abstract;
	virtual AssignableData^ GetAssignableData(AssignableFormat^ assignableFormat, Func<AssignableFormat^, AssignableData^>^ %delayedCb) abstract;
	IAssignableTexture(TextureDebugInfo^ tdi) :ITextureBase(tdi){}
};
