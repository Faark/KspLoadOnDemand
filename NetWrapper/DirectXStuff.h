#pragma once


using namespace System;

namespace LodNative{
	ref class DirectXStuff{
	public:
		static IDirect3DTexture9* Device_CreateTextureOrThrow(IDirect3DDevice9* device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool){
			IDirect3DTexture9* texture;
			switch (device->CreateTexture(width, height, levels, usage, format, pool, &texture, NULL)){
			case D3D_OK:
				return texture;
			case D3DERR_INVALIDCALL:
				throw gcnew InvalidOperationException("CreateTexture failed (InvalidCall, W:" + width + ", H:" + height + ", L:" + levels + ", U:" + usage + ", FMT:" + StringFromD3DFormat(format) + ", P: " + StringFromD3DPool(pool));
			case D3DERR_OUTOFVIDEOMEMORY:
				throw gcnew OutOfMemoryException("CreateTexture failed (OutOfVideoMemory, W:" + width + ", H:" + height + ", L:" + levels + ", U:" + usage + ", FMT:" + StringFromD3DFormat(format) + ", P: " + StringFromD3DPool(pool));
			case E_OUTOFMEMORY:
				throw gcnew OutOfMemoryException("CreateTexture failed (OutOfSystemMemory, W:" + width + ", H:" + height + ", L:" + levels + ", U:" + usage + ", FMT:" + StringFromD3DFormat(format) + ", P: " + StringFromD3DPool(pool));
			default:
				throw gcnew NotSupportedException("CreateTexture failed with unknown result code (W:" + width + ", H:" + height + ", L:" + levels + ", U:" + usage + ", FMT:" + StringFromD3DFormat(format) + ", P: " + StringFromD3DPool(pool));
			}
		}
		static void D3DX_CheckTextureRequirementsToThrow(IDirect3DDevice9* device, UINT* pWidth, UINT* pHeight, UINT* pNumMipLevels, DWORD Usage, D3DFORMAT* pFormat, D3DPOOL Pool){
			switch (D3DXCheckTextureRequirements(device, pWidth, pHeight, pNumMipLevels, Usage, pFormat, Pool)){
			case D3D_OK:
				return;
			case D3DERR_INVALIDCALL:
				throw gcnew InvalidOperationException("D3DXCheckTextureRequirements failed (InvalidCall, W:" + (*pWidth) + ", H:" + (*pHeight) + ", L:" + (*pNumMipLevels) + ", U:" + Usage + ", FMT:" + StringFromD3DFormat(*pFormat) + ", P: " + StringFromD3DPool(Pool));
			case D3DERR_NOTAVAILABLE:
				throw gcnew NotSupportedException("D3DXCheckTextureRequirements failed (NotAvailable, W:" + (*pWidth) + ", H:" + (*pHeight) + ", L:" + (*pNumMipLevels) + ", U:" + Usage + ", FMT:" + StringFromD3DFormat(*pFormat) + ", P: " + StringFromD3DPool(Pool));
			default:
				throw gcnew NotSupportedException("D3DXCheckTextureRequirements failed with unknown result code (W:" + (*pWidth) + ", H:" + (*pHeight) + ", L:" + (*pNumMipLevels) + ", U:" + Usage + ", FMT:" + StringFromD3DFormat(*pFormat) + ", P: " + StringFromD3DPool(Pool));
			}
		}
		static void Texture_LockRectOrThrow(IDirect3DTexture9* texture, UINT level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD flags){
			switch (texture->LockRect(level, pLockedRect, pRect, flags)){
			case D3D_OK:
				return;
			case D3DERR_INVALIDCALL:
				throw gcnew InvalidOperationException("LockRect failed (InvalidCall)");
			default:
				throw gcnew NotSupportedException("LockRect failed with unknown result code");
			}
		}
		static void Texture_UnlockRectOrThrow(IDirect3DTexture9* texture, UINT level){
			switch (texture->UnlockRect(level)){
			case D3D_OK:
				return;
			case D3DERR_INVALIDCALL:
				throw gcnew InvalidOperationException("UnlockRect failed (InvalidCall)");
			default:
				throw gcnew NotSupportedException("UnlockRect failed with unknown result code");
			}
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
				throw gcnew Exception("Unrecognized format: " + StringFromD3DFormat(from));
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
		static String^ StringFromD3DPool(D3DPOOL from){
			switch (from){
			case D3DPOOL_DEFAULT:
				return "D3DPOOL_DEFAULT";
			case D3DPOOL_MANAGED:
				return "D3DPOOL_MANAGED";
			case D3DPOOL_SYSTEMMEM:
				return "D3DPOOL_SYSTEMMEM";
			case D3DPOOL_SCRATCH:
				return "D3DPOOL_SCRATCH";
			default:
				return "D3DPOOL INVALID";
			}
		};
	};
}