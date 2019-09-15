#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "aac_encode.h"

using namespace std;
static AACENC_ERROR erraac;

#define IN_FILE_NAME "buweishui.pcm"
#define OUT_FILE_NAME "buweishui.aac"


#define AAC_HANDLE(x) \
	erraac = (x); \
	if (erraac != AACENC_OK) { \
		cout << "err: " << fdx_aac_err(erraac) << endl;; \
		goto fail; \
	} \

#define MY_CALL(x, y) \
	y = (x); \
	if (errno != 0){ \
		printf("syscall error %d, %s", errno, strerror(errno)); \
		return -1; \
	}	 



int getfilesize(const char* fname)
{
	struct stat statbuf;
	if (stat(fname, &statbuf) == 0)
		return statbuf.st_size;
	return -1;
}



static const char *fdx_aac_err(AACENC_ERROR erraac)
{
	switch (erraac) {
		case AACENC_OK: return "No error";			  
		case AACENC_INVALID_HANDLE: return "Invalid handle";
		case AACENC_MEMORY_ERROR: return "Memory allocation error";
		case AACENC_UNSUPPORTED_PARAMETER: return "Unsupported parameter";
		case AACENC_INVALID_CONFIG: return "Invalid config";
		case AACENC_INIT_ERROR: return "Initialization error";
		case AACENC_INIT_AAC_ERROR: return "AAC library initialization error";
		case AACENC_INIT_SBR_ERROR: return "SBR library initialization error";
		case AACENC_INIT_TP_ERROR: return "Transport library initialization error";
		case AACENC_INIT_META_ERROR: return "Metadata library initialization error";
		case AACENC_ENCODE_ERROR: return "Encoding error";
		case AACENC_ENCODE_EOF: return "End of file";
		default: return "Unknown error";
	}
}

AACENC_ERROR init_AAC_ctx(AAC_Encode_Ctx **pCtx, unsigned int samples, CHANNEL_MODE c_mode, unsigned int bite)
{
	*pCtx = (AAC_Encode_Ctx*)malloc(sizeof(AAC_Encode_Ctx));
	if (!*pCtx) {
		return AACENC_MEMORY_ERROR;
	}

	AAC_Encode_Ctx *p_tmp_ctx = *pCtx;

	AAC_HANDLE(aacEncOpen(&p_tmp_ctx->pHandle, 0x0, 2));

	AAC_HANDLE(aacEncoder_SetParam(p_tmp_ctx->pHandle, AACENC_AOT, AOT_SBR));
	AAC_HANDLE(aacEncoder_SetParam(p_tmp_ctx->pHandle, AACENC_SAMPLERATE, samples));
	AAC_HANDLE(aacEncoder_SetParam(p_tmp_ctx->pHandle, AACENC_CHANNELMODE, c_mode));
	AAC_HANDLE(aacEncoder_SetParam(p_tmp_ctx->pHandle, AACENC_BITRATE, bite));
	AAC_HANDLE(aacEncoder_SetParam(p_tmp_ctx->pHandle, AACENC_TRANSMUX, TT_MP4_ADTS));

	AAC_HANDLE(aacEncEncode(p_tmp_ctx->pHandle, NULL, NULL, NULL, NULL));
	AAC_HANDLE(aacEncInfo(p_tmp_ctx->pHandle, &p_tmp_ctx->pInfo));

	p_tmp_ctx->ulPCM_Len = p_tmp_ctx->pInfo.frameLength * p_tmp_ctx->pInfo.inputChannels * 2;
	printf("pcm len = %lu\n", p_tmp_ctx->ulPCM_Len);
	return AACENC_OK;
fail:
	if (p_tmp_ctx && p_tmp_ctx->pHandle)
	{
		if (aacEncClose(&p_tmp_ctx->pHandle) != AACENC_OK)
		{
			printf("aacEncClose failed\n");
		}
	}
	if (p_tmp_ctx) {
		free(p_tmp_ctx);
		return 	AACENC_INVALID_HANDLE;
	}
}

AACENC_ERROR aac_encode_data(AAC_Encode_Ctx *pCtx, char *pcm_buf, signed int &pcm_len, char *frame_buf, signed int &frame_len)
{
	if (!pCtx) return AACENC_MEMORY_ERROR;

	AACENC_BufDesc in_buf = { 0 };
	AACENC_BufDesc out_buf = { 0 };
	AACENC_InArgs inargs = { 0 };
	AACENC_OutArgs outargs = { 0 };

	inargs.numInSamples = pCtx->pInfo.frameLength * pCtx->pInfo.inputChannels;

	// in buf
	int		in_identifier = IN_AUDIO_DATA;
	int		in_elem_size = 2;
	in_buf.numBufs = 1;
	in_buf.bufs = (void**)&pcm_buf;
	in_buf.bufferIdentifiers = &in_identifier;
	in_buf.bufSizes = &pcm_len;
	in_buf.bufElSizes = &in_elem_size;

	// out buf
	int	out_identifier = OUT_BITSTREAM_DATA;
	int	out_elem_size = 1;
	out_buf.numBufs = 1;
	out_buf.bufs = (void**)&frame_buf;
	out_buf.bufferIdentifiers = &out_identifier;
	out_buf.bufSizes = &frame_len;		
	out_buf.bufElSizes = &out_elem_size;

	AAC_HANDLE(aacEncEncode(pCtx->pHandle, &in_buf, &out_buf, &inargs, &outargs));
	frame_len = outargs.numOutBytes;

	return AACENC_OK;
fail:
	return erraac;
}

int main()
{
	AAC_Encode_Ctx *aac_ctx = NULL;
	AACENC_ERROR aacerr = init_AAC_ctx(&aac_ctx, 48000, MODE_2, 32000);
	if (AACENC_OK != aacerr) {
		cout << "error appear" << endl;
		return 0;
	}
	int in_fd, out_fd;
	MY_CALL(open(IN_FILE_NAME, O_RDWR), in_fd);
	MY_CALL(open(OUT_FILE_NAME, O_RDWR | O_CREAT, 777), out_fd);

	char *pcm_buf = new char[aac_ctx->ulPCM_Len];
	char *frame = new char[1024 * 8];
	int	frame_len = 1024 * 8;

	unsigned int fileSize = getfilesize(IN_FILE_NAME);	   

	int read_len = 0;												   
	int read_count = 0;

	while (1) {
		memset(pcm_buf, 0, aac_ctx->ulPCM_Len);
		memset(frame, 0, 1024 * 8);
		frame_len = 1024 * 8;
		read_len = read(in_fd, pcm_buf, aac_ctx->ulPCM_Len);
		if (read_len < 0) {
			cout << strerror(errno) << endl;
			break;
		}

		if (!read_len) break;

		if (read_len < aac_ctx->ulPCM_Len) {
			memset(pcm_buf + read_len, 0, aac_ctx->ulPCM_Len - read_len);
		}

		if (aac_encode_data(aac_ctx, pcm_buf, read_len, frame, frame_len) == AACENC_OK) {

			write(out_fd, frame, frame_len);
			read_count += read_len;
			double process = read_count / (1.0 * fileSize) * 100;
			printf("encode progress = %0.2f%%, read_len = %d, frame_len = %d\n", process, read_len, frame_len);

		} 
	}

	close(in_fd);
	close(out_fd);
	delete[] pcm_buf;
	delete[] frame;
	aacEncClose(&aac_ctx->pHandle);
	free(aac_ctx);
	
	return 0;
}


