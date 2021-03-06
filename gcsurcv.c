/*
"gcsurcv.c: application that receives messages from a given topic
"  (single receiver). Understands UME.

  Copyright (c) 2005,2022 Informatica Corporation  Permission is granted to licensees to use
  or alter this software for any purpose, including commercial applications,
  according to the terms laid out in the Software License Agreement.

  This source code example is provided by Informatica for educational
  and evaluation purposes only.

  THE SOFTWARE IS PROVIDED "AS IS" AND INFORMATICA DISCLAIMS ALL WARRANTIES
  EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF
  NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR
  PURPOSE.  INFORMATICA DOES NOT WARRANT THAT USE OF THE SOFTWARE WILL BE
  UNINTERRUPTED OR ERROR-FREE.  INFORMATICA SHALL NOT, UNDER ANY CIRCUMSTANCES, BE
  LIABLE TO LICENSEE FOR LOST PROFITS, CONSEQUENTIAL, INCIDENTAL, SPECIAL OR
  INDIRECT DAMAGES ARISING OUT OF OR RELATED TO THIS AGREEMENT OR THE
  TRANSACTIONS CONTEMPLATED HEREUNDER, EVEN IF INFORMATICA HAS BEEN APPRISED OF
  THE LIKELIHOOD OF SUCH DAMAGES.
*/

#ifdef __VOS__
#define _POSIX_C_SOURCE 200112L
#include <sys/time.h>
#include <pthread.h>
#endif
#if defined(__TANDEM) && defined(HAVE_TANDEM_SPT)
	#include <ktdmtyp.h>
	#include <spthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <sys/timeb.h>
	#define strcasecmp stricmp
#else
	#include <unistd.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <signal.h>
	#include <sys/time.h>
	#if defined(__TANDEM)
		#include <strings.h>
	#endif
#endif
#include "replgetopt.h"
#include <lbm/lbm.h>
#include <lbm/lbmmon.h>
#include "monmodopts.h"
#include "verifymsg.h"
#include "lbm-example-util.h"

#if defined(_WIN32)
#   define SLEEP_SEC(x) Sleep((x)*1000)
#   define SLEEP_MSEC(x) Sleep(x)
#else
#   define SLEEP_SEC(x) sleep(x)
#   define SLEEP_MSEC(x) \
		do{ \
			if ((x) >= 1000){ \
				sleep((x) / 1000); \
				usleep((x) % 1000 * 1000); \
			} \
			else{ \
				usleep((x)*1000); \
			} \
		}while (0)
#endif /* _WIN32 */

const char Purpose[] = "Purpose: Receive messages on a single topic.";
const char Usage[] =
"Usage: %s [options] topic\n"
"Available options:\n"
"  -A, --ascii                 display messages as ASCII text (-A -A for newlines after each msg)\n"
"  -c, --config=FILE           use FILE as LBM configuration file\n"
"  -D, --deregister=NUM        Deregister the receiver after receiving NUM messages\n"
"  -E, --exit                  exit after source ends\n"
"  -e, --explicit-ack=N        send an Explicit ACK every N messages\n"
"  -h, --help                  display this help and exit\n"
"  --max-sources=num           allow num sources (for statistics gathering purposes)\n"
"  -i, --regid-offset=offset   use offset to calculate Registration ID\n"
"                              (as source registration ID + offset)\n"
"                              offset of 0 forces creation of regid by store\n"
"  -N, --seqnum=X              display recovery sequence number info and set low seqnum to low+X\n"
"  -r, --msgs=NUM              delete receiver after NUM messages\n"
"      --session-id=NUM        Use NUM as a Session ID rather than using a Registration ID\n"
"                              (regid-offset will be ignored)\n"
"  -s, --statistics=NUM        print statistics every NUM seconds, along with bandwidth\n"
"  -S, --stop                  exit after source ends, print throughput summary\n"
"  -u, --uregid=num            set User settable Registration ID to num for context\n"
"  -v, --verbose               be verbose about incoming messages\n"
"                              (-v -v = be even more verbose)\n"
"  -V, --verify                verify message contents\n"
"  -x, --no-exit-on-reg-error  don't exit on registration error (default is to exit)\n"
"  -X, --xml-config=FILE     Use UM XML configuration FILE\n"
"  -Y, --xml-appname=APP     Use UM XML APP application name\n"
;

const char * OptionString = "Ac:D:Ee:hi:N:r:s:SU:u:vVxX:Y:";
#define OPTION_MAX_SOURCES 7
#define OPTION_SESSION_ID 8
const struct option OptionTable[] =
{
	{ "ascii", no_argument, NULL, 'A' },
	{ "config", required_argument, NULL, 'c' },
	{ "deregister", required_argument, NULL, 'D' },
	{ "exit", no_argument, NULL, 'E' },
	{ "explicit-ack", required_argument, NULL, 'e' },
	{ "help", no_argument, NULL, 'h' },
	{ "regid-offset", required_argument, NULL, 'i' },
	{ "seqnum", required_argument, NULL, 'N' },
	{ "msgs", required_argument, NULL, 'r' },
	{ "statistics", required_argument, NULL, 's' },
	{ "stop", no_argument, NULL, 'S' },
	{ "session-id", required_argument, NULL, OPTION_SESSION_ID },
	{ "uregid", required_argument, NULL, 'u' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "verify", no_argument, NULL, 'V' },
	{ "no-exit-on-reg-error", no_argument, NULL, 'x' },
	{ "max-sources", required_argument, NULL, OPTION_MAX_SOURCES },
	{ "xml-config", required_argument, NULL, 'X' },
	{ "xml-appname", required_argument, NULL, 'Y' },
	{ NULL, 0, NULL, 0 }
};

struct Options {
	int ascii;          /* Flag to display messages as ASCII text */
	int end_on_end;     /* Flag to end program when source stops sending */
	int exack;          /* Number of messages between Explicit ACKs */
	int max_sources;    /* Maximum number of sources (for statistics) */
	int regid_offset;   /* Offset for calculating registration IDs */
	int seqnum_info;    /* Flag to enable display of sequence numbers */
	int seqnum_offset;  /* Offset for low end of sequence number range */
	int msg_limit;      /* Limit on number of messages to receive */
	int stats_ivl;      /* Interval for dumping statistics, in seconds */
	int summary;        /* Flag to display a summary when source stops */
	lbm_uint_t uregid;  /* User registration ID */
	int verbose;        /* Flag to control program verbosity */
	int verify;         /* Flag to control message verification (verifymsg.h) */
	int exit_on_reg_error; /* Flag to control whether app exits on registration error */
	char conffname[256]; /* Configuration filename */

	char *topic;                            /* The topic on which messages will be received */
	int deregister;
	char session_id[25];
	char xml_config[256];
	char xml_appname[256];
} options;


#define DEFAULT_MAX_NUM_SRCS 10000
#define DEFAULT_NUM_SRCS 10

struct timeval cur_tv;
int msg_count = 0;
int rx_msg_count = 0;
int total_msg_count = 0;
int stotal_msg_count = 0;
int subtotal_msg_count = 0;
int byte_count = 0;
#if defined(_WIN32)
signed __int64 total_byte_count = 0;
#else
unsigned long long total_byte_count = 0;
#endif /* _WIN32 */
int unrec_count = 0;
int total_unrec_count = 0;
int burst_loss = 0;
int close_recv = 0;
struct timeval data_start_tv;
struct timeval data_end_tv;

lbm_uint_t expected_sqn = 0;
char saved_source[LBM_MSG_MAX_SOURCE_LEN] = "";
lbm_ulong_t lost = 0, last_lost = 0;
lbm_rcv_transport_stats_t *stats = NULL;
int nstats;

/*
 * For the elapsed time, calculate and print the msgs/sec, bits/sec, and
 * loss stats
 */
void print_bw(FILE *fp, struct timeval *tv, int msgs, int bytes, int unrec, lbm_ulong_t lost, int rx_msgs)
{
	double sec = 0.0, mps = 0.0, bps = 0.0;
	double kscale = 1000.0, mscale = 1000000.0;
	char mgscale = 'K', bscale = 'K';

	if (tv->tv_sec == 0 && tv->tv_usec == 0) return;/* avoid div by 0 */
	sec = (double)tv->tv_sec + (double)tv->tv_usec / 1000000.0;
	mps = (double)msgs/sec;
	bps = (double)bytes*8/sec;
	if (mps <= mscale) {
		mgscale = 'K';
		mps /= kscale;
	} else {
		mgscale = 'M';
		mps /= mscale;
	}
	if (bps <= mscale) {
		bscale = 'K';
		bps /= kscale;
	} else {
		bscale = 'M';
		bps /= mscale;
	}


	if (rx_msgs != 0)
		fprintf(fp, "%-5.4g secs.  %-5.4g %cmsgs/sec.  %-5.4g %cbps [RX: %d]",
				sec, mps, mgscale, bps, bscale, rx_msgs);
	else
		fprintf(fp, "%-5.4g secs.  %-5.4g %cmsgs/sec.  %-5.4g %cbps",
				sec, mps, mgscale, bps, bscale);
	if (lost != 0 || unrec != 0 || burst_loss != 0) {
		fprintf(fp, " [%lu pkts lost, %u msgs unrecovered, %d bursts]",
				lost, unrec, burst_loss);
	}
	fprintf(fp, "\n");
	burst_loss = 0;
}

/* Print transport statistics */
void print_stats(FILE *fp, lbm_rcv_transport_stats_t *stats, int nstats, lbm_context_t *ctx)
{
	int i;
	lbm_context_stats_t ctx_stats;

	for (i = 0; i < nstats; i++)
	{
		switch (stats[i].type) {
		case LBM_TRANSPORT_STAT_TCP:
			fprintf(fp, " [%s], received %lu, LBM %lu/%lu/%lu\n",
					stats[i].source,
					stats[i].transport.tcp.bytes_rcved,
					stats[i].transport.tcp.lbm_msgs_rcved,
					stats[i].transport.tcp.lbm_msgs_no_topic_rcved,
					stats[i].transport.tcp.lbm_reqs_rcved);
			break;
		case LBM_TRANSPORT_STAT_LBTRM:
			{
				char stmstr[256] = "", txstr[256] = "";

				if (stats[i].transport.lbtrm.nak_tx_max > 0) {
					/* we usually don't use sprintf, but should be OK here for the moment. */
					sprintf(stmstr, ", nak stm %lu/%lu/%lu",
							stats[i].transport.lbtrm.nak_stm_min, stats[i].transport.lbtrm.nak_stm_mean,
							stats[i].transport.lbtrm.nak_stm_max);
					sprintf(txstr, ", nak tx %lu/%lu/%lu",
							stats[i].transport.lbtrm.nak_tx_min, stats[i].transport.lbtrm.nak_tx_mean,
							stats[i].transport.lbtrm.nak_tx_max);
				}
				fprintf(fp, " [%s], received %lu/%lu, dups %lu, loss %lu, naks %lu/%lu, ncfs %lu-%lu-%lu-%lu, unrec %lu/%lu%s%s\n",
						stats[i].source,
						stats[i].transport.lbtrm.msgs_rcved, stats[i].transport.lbtrm.bytes_rcved,
						stats[i].transport.lbtrm.duplicate_data,
						stats[i].transport.lbtrm.lost,
						stats[i].transport.lbtrm.naks_sent, stats[i].transport.lbtrm.nak_pckts_sent,
						stats[i].transport.lbtrm.ncfs_ignored, stats[i].transport.lbtrm.ncfs_shed,
						stats[i].transport.lbtrm.ncfs_rx_delay, stats[i].transport.lbtrm.ncfs_unknown,
						stats[i].transport.lbtrm.unrecovered_txw,
						stats[i].transport.lbtrm.unrecovered_tmo,
						stmstr, txstr);
			}
			break;
		case LBM_TRANSPORT_STAT_LBTRU:
			{
				char stmstr[256] = "", txstr[256] = "";

				if (stats[i].transport.lbtru.nak_tx_max > 0) {
					/* we usually don't use sprintf, but should be OK here for the moment. */
					sprintf(stmstr, ", nak stm %lu/%lu/%lu",
							stats[i].transport.lbtru.nak_stm_min, stats[i].transport.lbtru.nak_stm_mean,
							stats[i].transport.lbtru.nak_stm_max);
					sprintf(txstr, ", nak tx %lu/%lu/%lu",
							stats[i].transport.lbtru.nak_tx_min, stats[i].transport.lbtru.nak_tx_mean,
							stats[i].transport.lbtru.nak_tx_max);
				}
				fprintf(fp, " [%s], LBM %lu/%lu/%lu, received %lu/%lu, dups %lu, loss %lu, naks %lu/%lu, ncfs %lu-%lu-%lu-%lu, unrec %lu/%lu%s%s\n",
						stats[i].source,
						stats[i].transport.lbtru.lbm_msgs_rcved,
						stats[i].transport.lbtru.lbm_msgs_no_topic_rcved,
						stats[i].transport.lbtru.lbm_reqs_rcved,
						stats[i].transport.lbtru.msgs_rcved, stats[i].transport.lbtru.bytes_rcved,
						stats[i].transport.lbtru.duplicate_data,
						stats[i].transport.lbtru.lost,
						stats[i].transport.lbtru.naks_sent, stats[i].transport.lbtru.nak_pckts_sent,
						stats[i].transport.lbtru.ncfs_ignored, stats[i].transport.lbtru.ncfs_shed,
						stats[i].transport.lbtru.ncfs_rx_delay, stats[i].transport.lbtru.ncfs_unknown,
						stats[i].transport.lbtru.unrecovered_txw,
						stats[i].transport.lbtru.unrecovered_tmo,
						stmstr, txstr);
			}
			break;
		case LBM_TRANSPORT_STAT_LBTIPC:
			{
				fprintf(fp, " [%s] Received %lu msgs/%lu bytes. "
						"%lu LBM msgs, %lu no topics, %lu requests.\n",
						stats[i].source,
						stats[i].transport.lbtipc.msgs_rcved,
						stats[i].transport.lbtipc.bytes_rcved,
						stats[i].transport.lbtipc.lbm_msgs_rcved,
						stats[i].transport.lbtipc.lbm_msgs_no_topic_rcved,
						stats[i].transport.lbtipc.lbm_reqs_rcved);
			}
			break;
		case LBM_TRANSPORT_STAT_LBTRDMA:
			{
				fprintf(fp, " [%s] Received %lu msgs/%lu bytes. "
						"%lu LBM msgs, %lu no topics, %lu requests.\n",
						stats[i].source,
						stats[i].transport.lbtrdma.msgs_rcved,
						stats[i].transport.lbtrdma.bytes_rcved,
						stats[i].transport.lbtrdma.lbm_msgs_rcved,
						stats[i].transport.lbtrdma.lbm_msgs_no_topic_rcved,
						stats[i].transport.lbtrdma.lbm_reqs_rcved);
			}
			break;
		default:
			break;
		}
	}

	{
		lbm_context_retrieve_stats(ctx, &ctx_stats);
		printf("CONTEXT_STATS, tr_rcv_topics:%lu, tr_sent (%lu/%lu),tr_rcved (%lu/%lu), dropped(%lu/%lu/%lu), send_failed:%lu, blocked(%lu/%lu/%lu/%lu) \n",
		 ctx_stats.tr_rcv_topics, ctx_stats.tr_dgrams_sent, ctx_stats.tr_bytes_sent,  ctx_stats.tr_dgrams_rcved, ctx_stats.tr_bytes_rcved,
		 ctx_stats.tr_dgrams_dropped_ver, ctx_stats.tr_dgrams_dropped_type, ctx_stats.tr_dgrams_dropped_malformed, ctx_stats.tr_dgrams_send_failed,
		 ctx_stats.send_would_block, ctx_stats.send_blocked, ctx_stats.resp_blocked, ctx_stats.resp_would_block);
	}

	fflush(fp);
}

/* Utility to print the contents of a buffer in hex/ASCII format */
void dump(const char *buffer, int size)
{
	int i,j;
	unsigned char c;
	char textver[20];

	for (i=0;i<(size >> 4);i++) {
		for (j=0;j<16;j++) {
			c = buffer[(i << 4)+j];
			printf("%02x ",c);
			textver[j] = ((c<0x20)||(c>0x7e))?'.':c;
		}
		textver[j] = 0;
		printf("\t%s\n",textver);
	}
	for (i=0;i<size%16;i++) {
		c = buffer[size-size%16+i];
		printf("%02x ",c);
		textver[i] = ((c<0x20)||(c>0x7e))?'.':c;
	}
	for (i=size%16;i<16;i++) {
		printf("   ");
		textver[i] = ' ';
	}
	textver[i] = 0;
	printf("\t%s\n",textver);
}

void print_tv(struct timeval *tv){
/*
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64];

	nowtime = tv->tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, 64, "%Y-%m-%d:%H.%M.%S", nowtm);
	printf("[%s.%06d]: ", tmbuf, (int) tv->tv_usec);
*/
	printf("[@%lu.%06lu]", (unsigned long)tv->tv_sec, tv->tv_usec);
}
/* Logging handler passed into lbm_log() */
int lbm_log_msg(int level, const char *message, void *clientd)
{
	int newline = 1;

	current_tv (&cur_tv);
	print_tv (&cur_tv);

	if (message[strlen(message)-1] == '\n')
		newline = 0;

	if (newline)
		printf("LOG Level %d: %s\n", level, message);
	else
		printf("LOG Level %d: %s", level, message);
	return 0;
}

/* callback for setting the RegID based on extended info */
lbm_uint_t ume_rcv_regid_ex(lbm_ume_rcv_regid_ex_func_info_t *info, void *clientd)
{
	struct Options *opts = &options;
	lbm_uint_t regid = info->src_registration_id + opts->regid_offset;

	if (opts->verbose)
		printf("Store %u: %s [%s][%u] Flags %x. Requesting regid: %u (CD %p)\n", info->store_index, info->store, info->source,
			info->src_registration_id, info->flags, regid, info->source_clientd);
	return regid;
}

/* callback for sequence number information. Can be used to set low seqnum as well as display info */
int ume_rcv_seqnum_ex(lbm_ume_rcv_recovery_info_ex_func_info_t *info, void *clientd)
{
	struct Options *opts = &options;
	lbm_uint_t new_lo = info->low_sequence_number + opts->seqnum_offset;

	printf("[%s] SQNs Low %x (will set to %x), Low rxreqmax %x, High %x (CD %p) ", info->source, info->low_sequence_number,
		new_lo, info->low_rxreq_max_sequence_number, info->high_sequence_number, info->source_clientd);
	if (info->flags & LBM_UME_RCV_RECOVERY_INFO_EX_FLAG_SRC_SID) {
		printf("Src Session ID 0x%" PRIx64 " ", info->src_session_id);
	}
	printf("\n");
	info->low_sequence_number = new_lo;
	return 0;
}

/*
 * Handler for immediate messages directed to NULL topic
 * (passed into lbm_context_rcv_immediate_msgs()
 */
int rcv_handle_immediate_msg(lbm_context_t *ctx, lbm_msg_t *msg, void *clientd)
{
	struct Options *opts = &options;

	switch (msg->type) {
	case LBM_MSG_DATA:
		/* Data message received */
		msg_count++;
		total_msg_count++;
		subtotal_msg_count++;
		byte_count += msg->len;
		if (opts->ascii) {
			int n = msg->len;
			const char *p = msg->data;
			while (n--)
			{
				putchar(*p++);
			}
			if (opts->ascii > 1) putchar('\n');
		}
		if (opts->verbose) {
			printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
			printf("IM [%s][%u], %lu bytes\n", msg->source,
					msg->sequence_number, (unsigned long)msg->len);
			if (opts->verbose > 1)
				dump(msg->data, msg->len);
		}
		break;
	case LBM_MSG_REQUEST:
		/* Request message received (no response processed here) */
		msg_count++;
		total_msg_count++;
		subtotal_msg_count++;
		byte_count += msg->len;
		if (opts->ascii) {
			int n = msg->len;
			const char *p = msg->data;
			while (n--)
			{
				putchar(*p++);
			}
			if (opts->ascii > 1) putchar('\n');
		}
		if (opts->verbose) {
			printf("IM Request [%s][%u], %lu bytes\n", msg->source,
					msg->sequence_number, (unsigned long)msg->len);
			if (opts->verbose > 1)
				dump(msg->data, msg->len);
		}
		break;
	default:
		printf("Unknown immediate message lbm_msg_t type %x [%s]\n", msg->type, msg->source);
		break;
	}
	/* LBM automatically deletes the lbm_msg_t object unless we retain it. */
	return 0;
}

/* Received message handler (passed into lbm_rcv_create()) */
int rcv_handle_msg(lbm_rcv_t *rcv, lbm_msg_t *msg, void *clientd)
{
	struct Options *opts = &options;

	switch (msg->type) {
	case LBM_MSG_DATA:
		/* Data message received */
		if (subtotal_msg_count == 0) {
			/*
			 * Squirrel away source information so we know what
			 * source we are receiving from, although this example
			 * make no real effort in ensure that all messages
			 * received are from this source.
			 */
			strncpy(saved_source, msg->source, sizeof(saved_source));
		}

		(stotal_msg_count == 0) ? current_tv (&data_start_tv) : current_tv(&data_end_tv);

		msg_count++;
		total_msg_count++;
		stotal_msg_count++;
		subtotal_msg_count++;
		byte_count += msg->len;
		total_byte_count += msg->len;
		if (opts->deregister > 0) {
			opts->deregister--;
		}
		if (msg->flags & LBM_MSG_FLAG_RETRANSMIT)
			rx_msg_count++;

		if (opts->ascii) {
			int n = msg->len;
			const char *p = msg->data;
			while (n--)
			{
				putchar(*p++);
			}
			if (opts->ascii > 1) putchar('\n');
		}
		if (opts->verbose) {
 			printf("[%s][%s][%d]%s%s, %lu bytes\n", msg->topic_name, msg->source, msg->sequence_number,
					((msg->flags & LBM_MSG_FLAG_UME_RETRANSMIT) ? "-RX-" : ""),
					((msg->flags & LBM_MSG_FLAG_OTR) ? "-OTR-" : ""), (unsigned long)msg->len);
			if (opts->verbose > 1)
				dump(msg->data, msg->len);
		}
		if (opts->verify) {
			int rc = verify_msg(msg->data, msg->len, opts->verbose);
			if (rc == 0)
			{
				printf("Message sqn %x does not verify!\n", msg->sequence_number);
			}
			else if (rc == -1)
			{
				fprintf(stderr, "Message sqn %x is not a verifiable message.\n", msg->sequence_number);
				fprintf(stderr, "Use -V option on source and restart receiver.\n");
				exit(1);
			}
			else
			{
				if (opts->verbose)
				{
					printf("Message sqn %x verifies\n", msg->sequence_number);
				}
			}
		}
		if (opts->exack > 0) {
			if ((msg->sequence_number % opts->exack) == 0) {
				if (opts->verbose)
					printf(" Sending Explicit ACK\n");
				if (lbm_msg_ume_send_explicit_ack(msg) == -1) {
					fprintf(stderr, "lbm_msg_ume_send_explicit_ack:%s\n", lbm_errmsg());
				}
			}
		}
		break;
	case LBM_MSG_UNRECOVERABLE_LOSS:
		unrec_count++;
		total_unrec_count++;
		if (opts->verbose) {
			printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
			printf("[%s][%s][%x], LOST\n", msg->topic_name, msg->source, msg->sequence_number);
		}
		break;
	case LBM_MSG_UNRECOVERABLE_LOSS_BURST:
		burst_loss++;
		if (opts->verbose) {
			printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
			printf("[%s][%s][%x], LOST BURST\n", msg->topic_name, msg->source, msg->sequence_number);
		}
		break;
	case LBM_MSG_REQUEST:
		/* Request message received (no response processed here) */
		(stotal_msg_count == 0) ? current_tv (&data_start_tv) : current_tv(&data_end_tv);
		msg_count++;
		total_msg_count++;
		stotal_msg_count++;
		subtotal_msg_count++;
		byte_count += msg->len;
		total_byte_count += msg->len;
		if (opts->verbose) {
			printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
			printf("[%s][%s][%u], Request\n",
					msg->topic_name, msg->source, msg->sequence_number);
		}
		break;
	case LBM_MSG_BOS:
		printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
		printf("[%s][%s], Beginning of Transport Session\n", msg->topic_name, msg->source);
		break;
	case LBM_MSG_EOS:
		printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
		printf("[%s][%s], End of Transport Session\n", msg->topic_name, msg->source);
		subtotal_msg_count = 0;

		/* When verifying sequence numbers, multiple sources or EOS and new sources will cause
		 * the verification to fail as we don't track the numbers on a per source basis.
		 */
		if (opts->end_on_end)
			close_recv = 1;
		break;
	case LBM_MSG_NO_SOURCE_NOTIFICATION:
		printf("[%s], no sources found for topic\n", msg->topic_name);
		break;
	case LBM_MSG_UME_REGISTRATION_ERROR:
		printf("[%s][%s] UME registration error: %s\n", msg->topic_name, msg->source, msg->data);
		if (opts->exit_on_reg_error) {
			exit(0);
		}
		break;
	case LBM_MSG_UME_REGISTRATION_SUCCESS:
		{
			lbm_msg_ume_registration_t *reg = (lbm_msg_ume_registration_t *)(msg->data);

			printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
			printf("[%s][%s] UME registration successful. SrcRegID %u RcvRegID %u\n",
					msg->topic_name, msg->source, reg->src_registration_id, reg->rcv_registration_id);
		}
		break;
	case LBM_MSG_UME_REGISTRATION_SUCCESS_EX:
		{
			lbm_msg_ume_registration_ex_t *reg = (lbm_msg_ume_registration_ex_t *)(msg->data);

			printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
			printf("[%s][%s] store %u: %s UME registration successful. SrcRegID %u RcvRegID %u. Flags %x ",
				msg->topic_name, msg->source, reg->store_index, reg->store,
				reg->src_registration_id, reg->rcv_registration_id, reg->flags);
			if (reg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_OLD)
				printf("OLD[SQN %x] ", reg->sequence_number);
			if (reg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_NOCACHE)
				printf("NOCACHE ");
			if (reg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_RPP)
				printf("RPP ");
			if (reg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_SRC_SID)
				printf("Src Session ID 0x%" PRIx64 " ", reg->src_session_id);
			printf("\n");
		}
		break;
	case LBM_MSG_UME_REGISTRATION_COMPLETE_EX:
		{
			lbm_msg_ume_registration_complete_ex_t *reg = (lbm_msg_ume_registration_complete_ex_t *)(msg->data);

			printf("[%s][%s] UME registration complete. SQN %x. Flags %x ",
				msg->topic_name, msg->source, reg->sequence_number, reg->flags);
			if (reg->flags & LBM_MSG_UME_REGISTRATION_COMPLETE_EX_FLAG_QUORUM)
				printf("QUORUM ");
			if (reg->flags & LBM_MSG_UME_REGISTRATION_COMPLETE_EX_FLAG_RXREQMAX)
				printf("RXREQMAX ");
			if (reg->flags & LBM_MSG_UME_REGISTRATION_COMPLETE_EX_FLAG_SRC_SID)
				printf("Src Session ID 0x%" PRIx64 " ", reg->src_session_id);
			printf("\n");
		}
		break;
	case LBM_MSG_UME_DEREGISTRATION_SUCCESS_EX:
		{
			lbm_msg_ume_deregistration_ex_t *dereg = (lbm_msg_ume_deregistration_ex_t *)(msg->data);

			printf("[%s][%s] store %u: %s UME deregistration successful. SrcRegID %u RcvRegID %u. Flags %x ",
					msg->topic_name, msg->source, dereg->store_index, dereg->store,
					dereg->src_registration_id, dereg->rcv_registration_id, dereg->flags);
			if (dereg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_OLD)
				printf("OLD[SQN %x] ", dereg->sequence_number);
			if (dereg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_NOCACHE)
				printf("NOCACHE ");
			if (dereg->flags & LBM_MSG_UME_REGISTRATION_SUCCESS_EX_FLAG_RPP)
				printf("RPP ");
			printf("\n");
		}
	 	break;
	case LBM_MSG_UME_DEREGISTRATION_COMPLETE_EX:
		{
				printf("[%s][%s] UME deregistration complete.\n",
					msg->topic_name, msg->source);
		}
		break;
	case LBM_MSG_UME_REGISTRATION_CHANGE:
		printf("[@%lu.%06lu]", (unsigned long)msg->tsp.tv_sec, msg->tsp.tv_usec);
		printf("[%s][%s] UME registration change: %s\n", msg->topic_name, msg->source, msg->data);
		break;
	default:
		printf("Unknown lbm_msg_t type %x [%s][%s]\n", msg->type, msg->topic_name, msg->source);
		break;
	}
	/* LBM automatically deletes the lbm_msg_t object unless we retain it. */
	return 0;
}

#if !defined(_WIN32)
static int LossRate = 0;

static
void
SigHupHandler(int signo)
{
	if (LossRate >= 100)
	{
		return;
	}
	LossRate += 5;
	if (LossRate > 100)
	{
		LossRate = 100;
	}
	lbm_set_lbtrm_loss_rate(LossRate);
	lbm_set_lbtru_loss_rate(LossRate);
}

static
void
SigUsr1Handler(int signo)
{
	if (LossRate >= 100)
	{
		return;
	}
	LossRate += 10;
	if (LossRate > 100)
	{
		LossRate = 100;
	}
	lbm_set_lbtrm_loss_rate(LossRate);
	lbm_set_lbtru_loss_rate(LossRate);
}

static
void
SigUsr2Handler(int signo)
{
	LossRate = 0;
	lbm_set_lbtrm_loss_rate(LossRate);
	lbm_set_lbtru_loss_rate(LossRate);
}
#endif

#ifdef __VOS__
/* set round-robin scheduling policy for calling thread */
void set_rr_scheduling()
{
	pthread_t thread;
	int e,policy;
	struct sched_param param;

	thread = pthread_self(); /* get calling thread, i.e. main thread */
	pthread_getschedparam(thread, &policy, &param); /* get parameters */

	policy = SCHED_RR;
	e = pthread_setschedparam(thread, policy, &param);

	if(e != 0)
	{
		fprintf(stderr,
		  "failed to set round-robin thread scheduling policy.\n");
		exit(1);
	}
}
#endif

void process_cmdline(int argc, char **argv, struct Options *opts)
{
	int c, errflag = 0;

	/* Set default option values */
	memset(opts, 0, sizeof(*opts));
	opts->regid_offset = 1000;
	opts->max_sources = DEFAULT_NUM_SRCS;
	opts->conffname[0] = '\0';
	opts->exit_on_reg_error = 1;
	opts->deregister = -1;
	opts->xml_config[256] = '\0';

	/* Process the command line options, setting local variables with values */
	while ((c = getopt_long(argc, argv, OptionString, OptionTable, NULL)) != EOF)
	{
		switch (c)
		{
			case 'A':
				opts->ascii++;
				break;
			case 'c':
				strncpy(opts->conffname, optarg, sizeof(opts->conffname));
				break;
			case 'D':
				opts->deregister = atoi(optarg);
				break;
			case 'E':
				opts->end_on_end = 1;
				break;
			case 'e':
				opts->exack = atoi(optarg);
				break;
			case 'h':
				fprintf(stderr, "%s\n%s\n", lbm_version(), Purpose);
				fprintf(stderr, Usage, argv[0]);
				exit(0);
			case 'i':
				opts->regid_offset = atoi(optarg);
				break;
			case 'N':
				opts->seqnum_info = 1;
				opts->seqnum_offset = atoi(optarg);
				break;
			case 'r':
				opts->msg_limit = atoi(optarg);
				break;
			case 's':
				opts->stats_ivl = atoi(optarg);
				break;
			case 'S':
				opts->end_on_end = 1;
				opts->summary = 1;
				break;
			case OPTION_SESSION_ID:
				if(optarg != NULL) 
				{
					strncpy(opts->session_id, optarg, sizeof(opts->session_id));
				} 
				else 
				{
					errflag++;
				}
				break;
			case 'u':
				opts->uregid = atoi(optarg);
				break;
			case 'v':
				opts->verbose++;
				break;
			case 'V':
				opts->verify = 1;
				break;
			case 'x':
				opts->exit_on_reg_error = 0;
				break;
			default:
				errflag++;
				break;
		}
	}

	if ((errflag != 0) || (optind == argc))
	{
		/* An error occurred processing the command line - dump the LBM version, usage and exit */
		fprintf(stderr, "%s\n", lbm_version());
		fprintf(stderr, Usage, argv[0]);
		exit(1);
	}

	/* command line option processing complete at this point */
	opts->topic = argv[optind];
}

int main(int argc, char **argv)
{
	struct Options *opts = &options; /* filled by process_cmdline */
	int opmode, opmode_seq = 0; /* flag for sequential mode - from config file */
	lbm_context_t *ctx;
	lbm_topic_t *topic;
	lbm_rcv_t *rcv;
	lbm_context_attr_t * ctx_attr;
	lbm_rcv_topic_attr_t * rcv_attr;
	unsigned short int request_port;
	int request_port_bound;
	size_t optlen;
	lbm_ipv4_address_mask_t unicast_target_iface;
	struct in_addr inaddr;
	struct timeval stattv;

	double total_time = 0.0;
	double total_mps = 0.0;
	double total_bps = 0.0;
	lbm_ulong_t lost_tmp;
	int have_stats = 0, set_nstats;
	int i;
	char * xml_config_env_check = NULL;

#ifdef __VOS__
	set_rr_scheduling();	/* set round-robin thread scheduling policy */
#endif

#if defined(_WIN32)
	{
		WSADATA wsadata;
		int status;

		/* Windows socket setup code */
		if ((status = WSAStartup(MAKEWORD(2,2),&wsadata)) != 0) {
			fprintf(stderr,"%s: WSA startup error - %d\n",argv[0],status);
			exit(1);
		}
	}
#else
	/*
	 * Ignore SIGPIPE on UNIXes which can occur when writing to a socket
	 * with only one open end point.
	 */
	signal(SIGPIPE, SIG_IGN);
#endif /* _WIN32 */


	/* Process the different options set by the command line */
	process_cmdline(argc,argv,opts);

	nstats = opts->max_sources;
	/* Allocate array for statistics */
	stats = (lbm_rcv_transport_stats_t *)malloc(nstats * sizeof(lbm_rcv_transport_stats_t));
	if (stats == NULL)
	{
		fprintf(stderr, "can't allocate statistics array\n");
		exit(1);
	}

	/* Load LBM/UME configuration from file (if provided) */
	if (opts->conffname[0] != '\0') {
		if (lbm_config(opts->conffname) == LBM_FAILURE) {
			fprintf(stderr, "lbm_config: %s\n", lbm_errmsg());
			exit(1);
		}
	}

	/* Initialize logging callback */
	if (lbm_log(lbm_log_msg, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_log: %s\n", lbm_errmsg());
		exit(1);
	}
	if(opts->xml_config[0] != '\0'){
		/* Exit if env is set to pre-load an XML file */
		if ((xml_config_env_check = getenv("LBM_XML_CONFIG_FILENAME")) != NULL) {
			fprintf(stderr, "\n ERROR!: Please unset LBM_XML_CONFIG_FILENAME so that an XML file can be loaded \n" );
			exit(1);
		}
		if ((xml_config_env_check = getenv("LBM_UMM_INFO")) != NULL) {
			fprintf(stderr, "\n ERROR!: Please unset LBM_UMM_INFO so that an XML file can be loaded \n" );
			exit(1);
		}
		/* Initialize configuration parameters from an XML file. */
		if (lbm_config_xml_file(opts->xml_config, (const char *) opts->xml_appname ) == LBM_FAILURE) {
			fprintf(stderr, "Couldn't load lbm_config_xml_file: appname: %s xml_config: %s : Error: %s\n",
			opts->xml_appname, opts->xml_config, lbm_errmsg());
			exit(1);
		}
	}
	/* Retrieve default / configuration-modified context settings */
	if (lbm_context_attr_create(&ctx_attr) == LBM_FAILURE) {
		fprintf(stderr, "lbm_context_attr_create: %s\n", lbm_errmsg());
		exit(1);
	}
	{
		/*
		 * Since we are manually validating attributes, retrieve any XML configuration
		 * attributes set for this context.
		 */
		char ctx_name[256];
		size_t ctx_name_len = sizeof(ctx_name);
		if (lbm_context_attr_str_getopt(ctx_attr, "context_name", ctx_name, &ctx_name_len) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_attr_str_getopt - context_name: %s\n", lbm_errmsg());
			exit(1);
		}
		if (lbm_context_attr_set_from_xml(ctx_attr, ctx_name) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_attr_set_from_xml - context_name: %s\n", lbm_errmsg());
			exit(1);
		}
		/* init rcv topic attributes */
		if (lbm_rcv_topic_attr_create_from_xml (&rcv_attr, ctx_name, opts->topic)){
			fprintf(stderr, "lbm_rcv_topic_attr_create: %s\n", lbm_errmsg());
			exit(1);
		}

	}	
	/* Retrieve operational mode setting from context attribute structure */
	optlen = sizeof(opmode);
	if (lbm_context_attr_getopt(ctx_attr, "operational_mode", &opmode, &optlen) == LBM_FAILURE) {
		fprintf(stderr,"lbm_context_attr_str_getopt(operational_mode): %s\n", lbm_errmsg());
		exit(1);
	}
	if (opmode == LBM_CTX_ATTR_OP_SEQUENTIAL) {
		/*
		 * Operational mode is set to "sequential" meaning that LBM
		 * processing will not be handled in a separate thread. (See below.)
		 */
		opmode_seq = 1;
		printf("Sequential mode enabled.\n");
	}
	if (opts->uregid != 0) {
		if (lbm_context_attr_setopt(ctx_attr, "ume_user_receiver_registration_id",
					&opts->uregid, sizeof(opts->uregid)) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_attr_setopt(ume_user_receiver_registration_id): %s\n", lbm_errmsg());
			exit(1);
		}
	}

	/* Create LBM context according to given attribute structure */
	if (lbm_context_create(&ctx, ctx_attr, NULL, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_context_create: %s\n", lbm_errmsg());
		exit(1);
	}
	lbm_context_attr_delete(ctx_attr);

	/*
	 * Check settings to determine the TCP target for immediate messages.
	 * It might be appropriate to communicate this back to the source
	 * as a message.
	 */
	optlen = sizeof(request_port_bound);
	if (lbm_context_getopt(ctx,
				"request_tcp_bind_request_port",
				&request_port_bound,
				&optlen) == LBM_FAILURE)
	{
		fprintf(stderr, "lbm_context_getopt(request_tcp_bind_request_port): %s\n",
				lbm_errmsg());
		exit(1);
	}
	if (request_port_bound == 1) {
		optlen = sizeof(request_port);
		if (lbm_context_getopt(ctx,
				       "request_tcp_port",
				       &request_port,
				       &optlen) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_getopt(request_tcp_port): %s\n",
					lbm_errmsg());
			exit(1);
		}
		optlen = sizeof(unicast_target_iface);
		if (lbm_context_getopt(ctx,
				       "request_tcp_interface",
				       &unicast_target_iface,
				       &optlen) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_getopt(request_tcp_interface): %s\n",
					lbm_errmsg());
			exit(1);
		}
		/* if the request_tcp_interface is INADDR_ANY, get one we know is good. */
		if(unicast_target_iface.addr == INADDR_ANY) {
			if (lbm_context_getopt(ctx,
					       "resolver_multicast_interface",
					       &unicast_target_iface,
					       &optlen) == LBM_FAILURE) {
				fprintf(stderr, "lbm_context_getopt(resolver_multicast_interface): %s\n",
						lbm_errmsg());
				exit(1);
			}
		}
		inaddr.s_addr = unicast_target_iface.addr;
		printf("Immediate messaging target: TCP:%s:%d\n", inet_ntoa(inaddr),
			   ntohs(request_port));
	} else {
		printf("Request port binding disabled, no immediate messaging target.\n");
	}
#if !defined(_WIN32)
	signal(SIGHUP, SigHupHandler);
	signal(SIGUSR1, SigUsr1Handler);
	signal(SIGUSR2, SigUsr2Handler);
#endif

	/* Initialize immediate message handler (for topicless immediate sends) */
	if (lbm_context_rcv_immediate_msgs(ctx, rcv_handle_immediate_msg, NULL, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_context_rcv_immediate_msgs: %s\n", lbm_errmsg());
		exit(1);
	}

	/* set up using RegID if set */
	if (opts->session_id[0] != '\0') {
		if(lbm_rcv_topic_attr_str_setopt(rcv_attr, "ume_session_id", opts->session_id) == LBM_FAILURE) {
			fprintf(stderr, "lbm_rcv_topic_attr_str_setopt:ume_session_id: %s\n", lbm_errmsg());
			exit(1);
		}
	}

	{
		lbm_uint64_t sid;
		lbm_uint64_t ctx_sid;
		size_t sid_sz;

		sid_sz = sizeof(sid);
		if(lbm_rcv_topic_attr_getopt(rcv_attr, "ume_session_id", &sid, &sid_sz) == LBM_FAILURE) {
			fprintf(stderr, "lbm_rcv_topic_attr_getopt:ume_session_id: %s\n", lbm_errmsg());
			exit(1);
		}

		sid_sz = sizeof(ctx_sid);
		if(lbm_context_getopt(ctx, "ume_session_id", &ctx_sid, &sid_sz) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_getopt:ume_session_id: %s\n", lbm_errmsg());
			exit(1);
		}

		if(sid != 0) {
			/* Receiver-specific session IDs will override context-wide Session IDs, so if one is set, it will be used. */
			printf("Using 0x%"PRIx64" for receiver session ID.  Any registration IDs specified will be ignored.\n", sid);
		} else if (ctx_sid != 0) {
			printf("Using 0x%"PRIx64" for context session ID.  Any registration IDs specified will be ignored.\n", ctx_sid);
		} else if (opts->regid_offset != 0) {

			/* If neither a context-wide Session ID nor a receiver Session ID are set, the application can pick a Registration
			 * ID when it subscribes to a UMP source
			 */
			lbm_ume_rcv_regid_ex_func_t id;

			id.func = ume_rcv_regid_ex;
			id.clientd = NULL;

			if (lbm_rcv_topic_attr_setopt(rcv_attr, "ume_registration_extended_function", &id, sizeof(id)) == LBM_FAILURE) {
				fprintf(stderr, "lbm_rcv_topic_attr_setopt:ume_registration_extended_function: %s\n", lbm_errmsg());
				exit(1);
			}
			printf("Will use RegID offset %u.\n", opts->regid_offset);
		
		} else {
			/* The default is to have the store assign a Registration ID */
			printf("Will not use specific RegID.\n");
		}

	}
	/* set up using seqnum info callback */
	if (opts->seqnum_info) {
		lbm_ume_rcv_recovery_info_ex_func_t cb;

		cb.func = ume_rcv_seqnum_ex;
		cb.clientd = NULL;

		if (lbm_rcv_topic_attr_setopt(rcv_attr, "ume_recovery_sequence_number_info_function", &cb, sizeof(cb)) == LBM_FAILURE) {
			fprintf(stderr, "lbm_rcv_topic_attr_setopt:ume_recovery_sequence_number_info_function: %s\n", lbm_errmsg());
			exit(1);
		}
		printf("Will use seqnum info with low offset %u.\n", opts->seqnum_offset);
	}

	/* if using Explicit ACKs, turn off normal ACKing behavior */
	if (opts->exack > 0) {
		int on = 1;

		if (lbm_rcv_topic_attr_setopt(rcv_attr, "ume_explicit_ack_only", &on, sizeof(on)) == LBM_FAILURE) {
			fprintf(stderr, "lbm_rcv_topic_attr_setopt:ume_explicit_ack_only: %s\n", lbm_errmsg());
			exit(1);
		}
	}

	/* Lookup desired topic */
	if (lbm_rcv_topic_lookup(&topic, ctx, opts->topic, rcv_attr) == LBM_FAILURE) {
		fprintf(stderr, "lbm_rcv_topic_lookup: %s\n", lbm_errmsg());
		exit(1);
	}
	lbm_rcv_topic_attr_delete(rcv_attr);

#if !defined(_WIN32)
	signal(SIGUSR1, SigUsr1Handler);
	signal(SIGUSR2, SigUsr2Handler);
#endif

	/*
	 * Create receiver passing in the looked up topic info and the message
	 * handler callback.
	 */
	if (lbm_rcv_create(&rcv, ctx, topic, rcv_handle_msg, NULL, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_rcv_create: %s\n", lbm_errmsg());
		exit(1);
	}

	if ( opts->stats_ivl ) {
		current_tv ( &stattv );
		stattv.tv_sec += opts->stats_ivl;
	}
	while (1) {
		struct timeval starttv, endtv;
		int flPrintStats = 0;

		current_tv(&starttv);

		if (opmode_seq) {
			/*
			 * Run LBM context processing for 1 second if sequential
			 * mode is enabled.
			 */
			if (lbm_context_process_events(ctx, 1000) == LBM_FAILURE) {
				fprintf(stderr, "lbm_context_process_events: %s\n", lbm_errmsg());
				exit(1);
			}
		} else {
			/*
			 * Otherwise, just sleep for 1 second. LBM processing is
			 * done in its own thread.
			 */
			SLEEP_SEC(1);
		}

		/* Retrieve receiver stats */
		have_stats = 0;
		while (!have_stats){
			set_nstats = nstats;
			if (lbm_rcv_retrieve_all_transport_stats(rcv, &set_nstats, stats) == LBM_FAILURE){
				/* Double the number of stats passed to the API to be retrieved */
				/* Do so until we retrieve stats successfully or hit the max limit */
				nstats *= 2;
				if (nstats > DEFAULT_MAX_NUM_SRCS){
					fprintf(stderr, "Cannot retrieve all receiver stats (%s). Maximum number of sources = %d.\n",
							lbm_errmsg(), DEFAULT_MAX_NUM_SRCS);
					exit(1);
				}
				stats = (lbm_rcv_transport_stats_t *)realloc(stats,  nstats * sizeof(lbm_rcv_transport_stats_t));
				if (stats == NULL){
					fprintf(stderr, "Cannot reallocate statistics array\n");
					exit(1);
				}
			}
			else{
				have_stats = 1;
			}
		}

		lost = 0;
		for (i = 0; i < set_nstats; i++)
		{
			switch (stats[i].type) {
			case LBM_TRANSPORT_STAT_LBTRM:
				lost += stats[i].transport.lbtrm.lost;
				break;
			case LBM_TRANSPORT_STAT_LBTRU:
				lost += stats[i].transport.lbtru.lost;
				break;
			}
		}
		lost_tmp = lost;
		if (last_lost <= lost)
			lost -= last_lost;
		else
			lost = 0;
		last_lost = lost_tmp;

		current_tv(&endtv);

		if ( opts->stats_ivl )
			flPrintStats = ( ( endtv.tv_sec > stattv.tv_sec ) ||
							 ( endtv.tv_sec == stattv.tv_sec && endtv.tv_usec >= stattv.tv_usec ) ) ? 1 : 0;

		endtv.tv_sec -= starttv.tv_sec;
		endtv.tv_usec -= starttv.tv_usec;
		normalize_tv(&endtv);

		if (!opts->ascii)
			print_bw(stdout, &endtv, msg_count,
					byte_count, unrec_count, lost, rx_msg_count);
		if ( flPrintStats ) {
 			print_stats(stdout, stats, set_nstats, ctx);
			current_tv ( &stattv );
			stattv.tv_sec += opts->stats_ivl;
		}

		if ((opts->msg_limit > 0 && total_msg_count >= opts->msg_limit) || close_recv) {
			/*
			 * Get rid of receiver if we've received all we we
			 * wanted or if the sender has already gone away.
			 */
			lbm_rcv_delete(rcv);
			rcv = NULL;
		}
		msg_count = 0;
		rx_msg_count = 0;
		byte_count = 0;
		unrec_count = 0;
		if (rcv == NULL)
			break;

		if (opts->deregister == 0) {
			lbm_rcv_ume_deregister(rcv);
		}
	}

	if (opts->summary) {
		total_time = ((double)data_end_tv.tv_sec + (double)data_end_tv.tv_usec / 1000000.0)
						- ((double)data_start_tv.tv_sec + (double)data_start_tv.tv_usec / 1000000.0);
		printf ("\nTotal time        : %-5.4g sec\n", total_time);
		printf ("Messages received : %u\n", stotal_msg_count);
#if defined(_WIN32)
		printf ("Bytes received    : %I64d\n", total_byte_count);
#else
		printf ("Bytes received    : %lld\n", total_byte_count);
#endif
		if (total_time > 0) {
			total_mps = (double)total_msg_count/total_time;
			total_bps = (double)total_byte_count*8/total_time;
			printf ("Avg. throughput   : %-5.4g Kmsgs/sec, %-5.4g Mbps\n\n", total_mps/1000.0, total_bps/1000000.0);
		}

	}
	else
		printf("Quitting.... received %u messages\n", total_msg_count);


	SLEEP_SEC(5);

	/* Delete LBM context (not strictly necessary in this example) */
	lbm_context_delete(ctx);
	if (stats != NULL)
		free(stats);
	return 0;
}

