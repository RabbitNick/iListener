/*
 * Invoke gsmstack() with any kind of burst. Automaticly decode and retrieve
 * information.
 */
#include "system.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "gsmstack.h"
#include "gsm_constants.h"
#include "interleave.h"
#include "sch.h"
#include "cch.h"

//#include "out_pcap.h"


/* encode a decoded burst (1 bit per byte) into 8-bit-per-byte */
static void burst_octify(unsigned char *dest, 
			 const unsigned char *data, int length)
{
	int bitpos = 0;

	while (bitpos < USEFUL_BITS) {
		unsigned char tbyte;
		int i; 

		tbyte = 0;
		for (i = 0; (i < 8) && (bitpos < length); i++) {
			tbyte <<= 1;
			tbyte |= data[bitpos++];
		}
		if (i < 8)
			tbyte <<= 8 - i;
		*dest++ = tbyte;
	}	
}


#if 0
static void
diff_decode(char *dst, char *src, int len)
{
	const char *end = src + len;
	unsigned char last;

	src += 3;
	last = 0;
	memset(dst, 0, 3);
	dst += 3;

	while (src < end)
	{
		*dst = !*src ^ last;
		last = *dst;
		src++;
		dst++;
	}
}
#endif

/*
 * Initialize a new GSMSTACK context.
 */
int
GS_new(GS_CTX *ctx)
{
	memset(ctx, 0, sizeof *ctx);
	interleave_init(&ctx->interleave_ctx, 456, 114);
	ctx->fn = -1;
	ctx->bsic = -1;

//	ctx->pcap_fd = open_pcap_file((char *)"tvoid.pcap");
	//if (ctx->pcap_fd < 0)
		//fprintf(stderr, "cannot open PCAP file: %s\n", strerror(errno));

//	ctx->burst_pcap_fd = open_pcap_file((char *)"tvoid-burst.pcap");
	//if (ctx->burst_pcap_fd < 0)
		//fprintf(stderr, "cannot open burst PCAP file: %s\n", strerror(errno));

	return 0;
}

#define BURST_BYTES	((USEFUL_BITS/8)+1)
/*
 * 142 bit
 */


int GS_group_process(struct gs_ts_ctx* ts_ctx, int position, unsigned char* bits116) {
	memcpy(ts_ctx->burst + 116*position, bits116, 116);
	ts_ctx->burst_count++;
	return ts_ctx->burst_count;
}





int
GS_process(GS_CTX *ctx, int ts, int type, const unsigned char *src)
{
	int fn;
	int bsic;
	int ret;
	unsigned char *data;
	int len;
	struct gs_ts_ctx *ts_ctx = &ctx->ts_ctx[ts];
	unsigned char octified[BURST_BYTES];

	memset(ctx->msg, 0, sizeof(ctx->msg));

	/* write burst to burst PCAP file */
	burst_octify(octified, src, USEFUL_BITS);
	//write_pcap_packet(ctx->burst_pcap_fd, 0 /* arfcn */, ts, ctx->fn,
	//		  1, type, octified, BURST_BYTES);

	if (type == NORMAL) {
		/* Interested in these frame numbers (cch)
 		 * 2-5, 12-15, 22-25, 23-35, 42-45
 		 * 6-9, 16-19, 26-29, 36-39, 46-49
 		 */
		/* Copy content data into new array */
		//DEBUGF("burst count %d\n", ctx->burst_count);
		memcpy(ts_ctx->burst + (116 * ts_ctx->burst_count), src, 58);
		memcpy(ts_ctx->burst + (116 * ts_ctx->burst_count) + 58, src + 58 + 26, 58);
		ts_ctx->burst_count++;
		/* Return if not enough bursts for a full gsm message */
		if (ts_ctx->burst_count < 4)
			return 0;

		ts_ctx->burst_count = 0;
		data = decode_cch(ctx, ts_ctx->burst, (unsigned int* )&len);
		if (data == NULL) {
			//DEBUGF("cannot decode fnr=0x%08x ts=%d\n", ctx->fn, ts);
			return -1;
		}
		//DEBUGF("OK TS %d, len %d\n", ts, len);

		out_gsmdecode(0, 0, ts, ctx->fn - 4, (char* )data, len);
	//	write_pcap_packet(ctx->pcap_fd, 0 /* arfcn */, ts, ctx->fn,
///				  0, NORMAL, data, len);
#if 0
		if (ctx->fn % 51 != 0) && ( (((ctx->fn % 51 + 5) % 10 == 0) || (((ctx->fn % 51) + 1) % 10 ==0) ) )
			ready = 1;
#endif
		
		return 0;
	}
}


/*
 * Output data so that it can be parsed from gsmdeocde.
 */
void
out_gsmdecode(char type, int arfcn, int ts, int fn, unsigned char *data, int len)
{
	unsigned char *end = data + len;

	/* FIXME: speed this up by first printing into an array */
	while (data < end)
		printf(" %.2x", (unsigned char)*data++);
		//printf(" %02.2x", (unsigned char)*data++);
	printf("\n");
	fflush(stdout);
}

void out_gsmdecode2(unsigned char *data, int len) {
	unsigned char *end = data + len;

	/* FIXME: speed this up by first printing into an array */
	while (data < end)
		printf(" %.2x", (unsigned char)*data++);
		//printf(" %02.2x", (unsigned char)*data++);
	printf("\n");
	fflush(stdout);
}
