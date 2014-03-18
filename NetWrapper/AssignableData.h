#pragma once

using namespace System;

#include "GPU.h"
#include "ITexture.h"

ref class ByteArrayAssignableData : AssignableData{
public:
	array<Byte>^ data;
	int start_offset;
	int pitch;

	ByteArrayAssignableData(AssignableFormat^ format, array<Byte>^ byte_array, int byte_pitch, int byte_offset, TextureDebugInfo^ debug_info) :AssignableData(format, debug_info){
		data = byte_array;
		start_offset = byte_offset;
		pitch = byte_pitch;
	}

	virtual void AssignTo(AssignableTarget^ target) override {
		pin_ptr<unsigned char> src_ptr = &data[start_offset];
		GPU::CopyRawBytesTo(target, src_ptr, pitch, Format->BytesPerPixel*Format->Width, Format->Height, IsUpsideDown, Debug);
	}
};
