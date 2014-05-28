#pragma once

using namespace System;
using namespace System::Drawing::Imaging;

#include "Logger.h"
#include "DirectXStuff.h"


namespace LodNative{
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
		virtual String^ ToString() override{
			return File + Modifiers;
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
			return Width + "x" + Height + "@" + (DirectXStuff::StringFromD3DFormat(Format));
		}
		AssignableFormat^ Copy(){
			return gcnew AssignableFormat(Width, Height, Format);
		}
		property BYTE BytesPerPixel{BYTE get(){ return DirectXStuff::GetByteDepthForFormat(Format); }}
		property BYTE BitsPerPixel{BYTE get(){ return DirectXStuff::GetBitDepthForFormat(Format); }}

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
	/*
	Texture generating this must stay alife in the lifetime of this assignableData. (so we can re-use its allocated mem)
	*/
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
}