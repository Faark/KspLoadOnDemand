#pragma once

#include "stdafx.h"

namespace LodNative{
	ref class Stuff{
	public:
		static void CopyLineBuffer(unsigned char* src_buffer, int src_pitch, unsigned char* trg_buffer, int trg_pitch, int line_size_in_bytes, int height){

			if (src_pitch == trg_pitch && src_pitch == line_size_in_bytes){
				memcpy(trg_buffer, src_buffer, line_size_in_bytes*height);
			}
			else{
				for (int y = 0; y < height; y++){
					memcpy(trg_buffer, src_buffer, line_size_in_bytes);
					trg_buffer += trg_pitch;
					src_buffer += src_pitch;
				}
			}
		}
		// Unity textures seem to be upside down... we should be able to handle it easily, though...
		// https://developer.oculusvr.com/forums/viewtopic.php?f=37&t=896
		static void CopyLineBufferInverted(unsigned char* src_buffer, int src_pitch, unsigned char* trg_buffer, int trg_pitch, int line_size_in_bytes, int height){

			CopyLineBuffer(src_buffer + ((height - 1)*src_pitch), -src_pitch, trg_buffer, trg_pitch, line_size_in_bytes, height);
		}

	};
}