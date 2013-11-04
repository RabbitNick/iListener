
#ifndef __GSMSTACK_H__
#define __GSMSTACK_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "interleave.h"

struct gs_ts_ctx {
	/* FIXME: later do this per each ts per each arfcn */
	unsigned char burst[4 * 58 * 2];
	int burst_count;
};

typedef struct
{
	int flags;
	int fn;
	int bsic;
	char msg[23];	/* last decoded message */

	INTERLEAVE_CTX interleave_ctx;

	//struct gs_ts_ctx ts_ctx[8];
	struct gs_ts_ctx ts_ctx[8];

//	int pcap_fd;
	//int burst_pcap_fd;
} GS_CTX;

int GS_new(GS_CTX *ctx);
int GS_process(GS_CTX *ctx, int ts, int type, const unsigned char *src);
int GS_group_process(struct gs_ts_ctx* ts_ctx, int position, unsigned char* bits116);

void out_gsmdecode(char type, int arfcn, int ts, int fn, unsigned char *data, int len);
void out_gsmdecode2(unsigned char *data, int len);

#ifdef __cplusplus
}
#endif

#endif
