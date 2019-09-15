#ifndef AAC_ENCODE_H
#define AAC_ENCODE_H
#include "aacenc_lib.h"
#include "FDK_audio.h"

typedef struct tag_AAC_Encode_Context {

	HANDLE_AACENCODER pHandle;

	AACENC_InfoStruct pInfo;

	unsigned long ulPCM_Len;
}AAC_Encode_Ctx;


AACENC_ERROR init_AAC_ctx(AAC_Encode_Ctx **pCtx, unsigned int samples, CHANNEL_MODE c_mode, unsigned int bite);
AACENC_ERROR aac_encode_data(AAC_Encode_Ctx *pCtx, char *pcm_buf, signed int &pcm_len, char *frame_buf, signed int &frame_len);


#endif