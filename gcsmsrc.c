/*
"gcsmsrc.c: Send messages on multiple topics, optionally by multiple threads.
"  Topic names generated as a root  followed by a dot, followed by an integer.
"  By default, the first topic created will be '29west.example.multi.0'

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
	#define snprintf _snprintf
#else
	#include <unistd.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <signal.h>
	#include <sys/time.h>
	#if defined(__TANDEM)
		#include <strings.h>
		#if defined(HAVE_TANDEM_SPT)
			#include <spthread.h>
		#else
			#include <pthread.h>
		#endif
	#else
		#include <pthread.h>
	#endif
#endif
#include "replgetopt.h"
#include <lbm/lbm.h>
#include <lbm/lbmmon.h>
#include "monmodopts.h"
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

const char Purpose[] = "Purpose: Send messages on multiple topics, optionally by multiple threads.\n"
							  "Topic names generated as a root  followed by a dot, followed by an integer.\n"
							  "By default, the first topic created will be '29west.example.multi.0'";
const char Usage[] =
"Usage: %s [options]\n"
"Available options:\n"
"  -b, --batch=NUM           send messages in batch sizes of NUM between each pause\n"
"  -c, --config=FILE         Use LBM configuration file FILE.\n"
"                            Multiple config files are allowed.\n"
"                            Example:  '-c file1.cfg -c file2.cfg'\n"
"                            NOTE: For XML config files, use the -X and -Y options\n"
"  -d, --delay=NUM           delay sending for delay seconds after source creation\n"
"  -h, --help                display this help and exit\n"
"  -i, --initial-topic=NUM   use NUM as initial topic number [0]\n"
"  -j, --late-join=NUM       enable Late Join with specified retention buffer size (in bytes)\n"
"  -l, --length=NUM          send messages of length NUM bytes\n"
"  -L, --linger=NUM          linger for NUM seconds after done\n"
"  -M, --messages=NUM        send maximum of NUM messages\n"
"  -P, --pause=NUM           pause NUM milliseconds after each send\n"
"  -r, --root=STRING         use topic names with root of STRING [29west.example.multi]\n"
"  -R, --rate=[UM]DATA/RETR  Set transport type to LBT-R[UM], set data rate limit to\n"
"                            DATA bits per second, and set retransmit rate limit to\n"
"                            RETR bits per second.  For both limits, the optional\n"
"                            k, m, and g suffixes may be used.  For example,\n"
"                            '-R 1m/500k' is the same as '-R 1000000/500000'\n"
"  -s, --statistics=NUM      print stats every NUM seconds\n"
"      --context-stats       include context stats with -s option\n"
"  -S, --sources=NUM         use NUM sources\n"
"  -T, --threads=NUM         use NUM threads\n"
"  -v, --verbose             be verbose\n"
"  -X, --xml-config=FILE     Use UM XML configuration FILE\n"
"  -Y, --xml-appname=APP     Use UM XML APP application name\n"
;

const char * OptionString = "b:c:d:hi:j:l:L:M:P:r:R:s:S:T:vX:Y:";
#define OPTION_CONTEXT_STATS 1
const struct option OptionTable[] =
{
	{ "batch", required_argument, NULL, 'b' },
	{ "config", required_argument, NULL, 'c' },
	{ "delay", required_argument, NULL, 'd' },
	{ "help", no_argument, NULL, 'h' },
	{ "initial-topic", required_argument, NULL, 'i' },
	{ "late-join", required_argument, NULL, 'j' },
	{ "length", required_argument, NULL, 'l' },
	{ "linger", required_argument, NULL, 'L' },
	{ "messages", required_argument, NULL, 'M' },
	{ "pause", required_argument, NULL, 'P' },
	{ "root", required_argument, NULL, 'r' },
	{ "rate", required_argument, NULL, 'R' },
	{ "statistics", required_argument, NULL, 's' },
	{ "sources", required_argument, NULL, 'S' },
	{ "threads", required_argument, NULL, 'T' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "xml-config", required_argument, NULL, 'X' },
	{ "xml-appname", required_argument, NULL, 'Y' },
	{ "context-stats", no_argument, NULL, OPTION_CONTEXT_STATS },
	{ NULL, 0, NULL, 0 }
};

#define MAX_MSG_SZ 3000000
#define MIN_ALLOC_MSGLEN 25
#define DEFAULT_MAX_MESSAGES 10000000
#define MAX_NUM_SRCS 1000001
#define DEFAULT_NUM_SRCS 100
#define DEFAULT_NUM_THREADS 1
#define DEFAULT_TOPIC_ROOT "29west.example.multi"
#define DEFAULT_LINGER_SECONDS 1
#define DEFAULT_INITIAL_TOPIC_NUMBER 0
#define DEFAULT_MAX_NUM_TRANSPORTS 100

struct Options {
	int context_stats;	/* Flag to include context stats */
	char xml_config[256];	/* XML Configuration file */
	char xml_appname[256]; 	/* Application name reference in the XML file */
} options;
struct Options *opts = &options;

/* global options */
int verbose = 0;
int delay = 1;

/* Source event handler (passed into lbm_src_create()) */
int handle_src_event(lbm_src_t *src, int event, void *ed, void *cd)
{
	switch (event) {
	case LBM_SRC_EVENT_CONNECT:
		/*
		 * Indicates that a receiver has connected to the
		 * source (topic)
		 */
		{
			const char *clientname = (const char *)ed;

			if (verbose)
				printf("Receiver connect [%s]\n",clientname);
		}
		break;
	case LBM_SRC_EVENT_DISCONNECT:
		/*
		 * Indicates that a receiver has disconnected from the
		 * source (topic)
		 */
		{
			const char *clientname = (const char *)ed;

			if (verbose)
				printf("Receiver disconnect [%s]\n",clientname);
		}
		break;
	default:
		printf("Unknown source event %d\n", event);
		break;
	}
	return 0;
}

lbm_event_queue_t *evq = NULL;
lbm_src_t **srcs = NULL;
size_t msglen = MIN_ALLOC_MSGLEN;
int num_thrds = DEFAULT_NUM_THREADS;
int num_srcs = DEFAULT_NUM_SRCS;
int totalmsgsleft = DEFAULT_MAX_MESSAGES;
int msecpause = 0;
int batchsz = 1;
int stats_timer_id = -1, done_sending = 0;
lbm_ulong_t stats_sec = 0;

/* Print transport statistics */
void print_stats(FILE *fp, lbm_src_transport_stats_t *stats)
{
	fprintf(fp, "[%s]", stats->source);
	switch (stats->type) {
	case LBM_TRANSPORT_STAT_TCP:
		fprintf(fp, " buffered %lu, clients %lu\n", stats->transport.tcp.bytes_buffered,
				stats->transport.tcp.num_clients);
		break;
	case LBM_TRANSPORT_STAT_LBTRM:
		fprintf(fp, " sent %lu/%lu, txw %lu/%lu, naks %lu/%lu, ignored %lu/%lu, shed %lu, rxs %lu, rctlr %lu/%lu\n",
				stats->transport.lbtrm.msgs_sent, stats->transport.lbtrm.bytes_sent,
				stats->transport.lbtrm.txw_msgs, stats->transport.lbtrm.txw_bytes,
				stats->transport.lbtrm.naks_rcved, stats->transport.lbtrm.nak_pckts_rcved,
				stats->transport.lbtrm.naks_ignored, stats->transport.lbtrm.naks_rx_delay_ignored,
				stats->transport.lbtrm.naks_shed,
				stats->transport.lbtrm.rxs_sent,
				stats->transport.lbtrm.rctlr_data_msgs, stats->transport.lbtrm.rctlr_rx_msgs);
		break;
	case LBM_TRANSPORT_STAT_LBTRU:
		fprintf(fp, " clients %lu, sent %lu/%lu, naks %lu/%lu, ignored %lu/%lu, shed %lu, rxs %lu\n",
				stats->transport.lbtru.num_clients,
				stats->transport.lbtru.msgs_sent, stats->transport.lbtru.bytes_sent,
				stats->transport.lbtru.naks_rcved, stats->transport.lbtru.nak_pckts_rcved,
				stats->transport.lbtru.naks_ignored, stats->transport.lbtru.naks_rx_delay_ignored,
				stats->transport.lbtru.naks_shed,
				stats->transport.lbtru.rxs_sent);
		break;
	case LBM_TRANSPORT_STAT_LBTIPC:
		fprintf(fp, " clients %lu, sent %lu/%lu\n",
				stats->transport.lbtipc.num_clients,
				stats->transport.lbtipc.msgs_sent, stats->transport.lbtipc.bytes_sent);
		break;
	case LBM_TRANSPORT_STAT_LBTSMX:
		fprintf(fp, " clients %lu, sent %lu/%lu\n",
				stats->transport.lbtsmx.num_clients,
				stats->transport.lbtsmx.msgs_sent, stats->transport.lbtsmx.bytes_sent);
		break;
	case LBM_TRANSPORT_STAT_LBTRDMA:
		fprintf(fp, " clients %lu, sent %lu/%lu\n",
				stats->transport.lbtrdma.num_clients,
				stats->transport.lbtrdma.msgs_sent, stats->transport.lbtrdma.bytes_sent);
		break;
	default:
		break;
	}


	fflush(fp);
}

/* Timer callback to handle periodic display of source statistics */
int handle_stats_timer(lbm_context_t *ctx, const void *clientd)
{
	lbm_src_transport_stats_t stats[DEFAULT_MAX_NUM_TRANSPORTS];
	int num_transports = DEFAULT_MAX_NUM_TRANSPORTS;
	lbm_context_stats_t ctx_stats;

	if (lbm_context_retrieve_src_transport_stats(ctx, &num_transports, stats) != LBM_FAILURE) {
		int scount = 0;

		for (scount = 0; scount < num_transports; scount++) {
			fprintf(stdout, "stats %u/%u:", scount+1, num_transports);
			print_stats(stdout, &stats[scount]);
		}


	} else {
		fprintf(stderr, "lbm_context_retrieve_src_transport_stats: %s\n", lbm_errmsg());
	}

	if (opts->context_stats){ /* context stats */ 
		lbm_context_retrieve_stats(ctx, &ctx_stats);
		printf("CONTEXT_STATS, tr_rcv_topics:%lu, tr_sent (%lu/%lu),tr_rcved (%lu/%lu), dropped(%lu/%lu/%lu), send_failed:%lu, blocked(%lu/%lu/%lu/%lu) \n",
		 ctx_stats.tr_rcv_topics, ctx_stats.tr_dgrams_sent, ctx_stats.tr_bytes_sent,  ctx_stats.tr_dgrams_rcved, ctx_stats.tr_bytes_rcved,
		 ctx_stats.tr_dgrams_dropped_ver, ctx_stats.tr_dgrams_dropped_type, ctx_stats.tr_dgrams_dropped_malformed, ctx_stats.tr_dgrams_send_failed,
		 ctx_stats.send_would_block, ctx_stats.send_blocked, ctx_stats.resp_blocked, ctx_stats.resp_would_block);
	}

	if (!done_sending) {
		if ((stats_timer_id = lbm_schedule_timer(ctx, handle_stats_timer, ctx, NULL, (stats_sec * 1000))) == -1) {
			fprintf(stderr, "lbm_schedule_timer: %s\n", lbm_errmsg());
			exit(1);
		}
	}
	return 0;
}

#if defined(_WIN32)
#  define MAX_NUM_THREADS 16
int thrdidxs[MAX_NUM_THREADS];
int msgsleft[MAX_NUM_THREADS];
#else
#  define MAX_NUM_THREADS 16
int thrdidxs[MAX_NUM_THREADS];
int msgsleft[MAX_NUM_THREADS];
#endif /* _WIN32 */

/*
 * Per thread sending loop
 */
#if defined(_WIN32)
DWORD WINAPI sending_thread_main(void *arg)
#else
void *sending_thread_main(void *arg)
#endif /* _WIN32 */
{
	int i = 0, thrdidx = *((int *)arg);
	int n = 0;
	char *message = NULL;

#if defined(_WIN32)
	if (thrdidx > 0) {
		/* The following line is only needed for static Windows library use */
		lbm_win32_static_thread_attach();
	}
#endif /* _WIN32 */
	/* if message buffer is too small, then the sprintf will cause issues. So, allocate with a min size */
	if (msglen < MIN_ALLOC_MSGLEN) {
		message = malloc(MIN_ALLOC_MSGLEN);
	} else {
		message = malloc(msglen);
	}
	if (message == NULL) {
		fprintf(stderr, "could not allocate message buffer of size %u bytes\n",(unsigned int)msglen);
		exit(1);
	}
	memset(message, 0, msglen);

	/*
	 * Send to each source in turn until we have sent the max number
	 * of messages total.
	 */
	while (msgsleft[thrdidx] > 0) {
		for (i = thrdidx; i < num_srcs; i += num_thrds) {
			if (lbm_src_send(srcs[i], message, msglen, 0) == LBM_FAILURE) {
				fprintf(stderr, "lbm_src_send: %s\n", lbm_errmsg());
				exit(1);
			}
			n++;
			if (--msgsleft[thrdidx] == 0)
				break;
			if (n >= batchsz && msecpause) {
				n = 0;
				SLEEP_MSEC(msecpause);
			}
		}
	}
	free(message);
#if defined(_WIN32)
	if (thrdidx > 0) {
		/* The following line is only needed for static Windows library use */
		lbm_win32_static_thread_detach();
		ExitThread(0);
	}
	return 0;
#else
	if (thrdidx > 0)
		pthread_exit(NULL);
	return NULL;
#endif /* _WIN32 */
}

int main(int argc, char **argv)
{
	lbm_context_t *ctx;
	lbm_topic_t *topic = NULL;
	lbm_src_topic_attr_t * tattr;
	lbm_context_attr_t * cattr;
	char topicname[LBM_MSG_MAX_TOPIC_LEN];
	char topicroot[80] = DEFAULT_TOPIC_ROOT;
	int c, i = 0, errflag = 0, linger = DEFAULT_LINGER_SECONDS,
	initial_topic_number = DEFAULT_INITIAL_TOPIC_NUMBER;
	unsigned long int latejoin_threshold = 0; 	/* Maximum Late Join buffer size, in bytes */
	lbm_uint64_t rm_rate = 0, rm_retrans = 0;
	char rm_protocol = 'M';
	char * xml_config_env_check = NULL;
#if defined(_WIN32)
	HANDLE wthrdh[MAX_NUM_THREADS];
	DWORD wthrdids[MAX_NUM_THREADS];
#else
	pthread_t pthids[MAX_NUM_THREADS];
#endif /* _WIN32 */

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

	memset(opts, 0, sizeof(*opts));

	/* Process the command line options, setting local/global variables with values */
	while ((c = getopt_long(argc, argv, OptionString, OptionTable, NULL)) != EOF)
	{
		switch (c)
		{
			case 'b':
				batchsz = atoi(optarg);
				break;
			case 'c':
				/* Initialize configuration parameters from a file. */
				if (lbm_config(optarg) == LBM_FAILURE) {
					fprintf(stderr, "lbm_config: %s\n", lbm_errmsg());
					exit(1);
				}
				break;
			case 'd':
				delay = atoi(optarg);
				break;
			case 'j':
				if (sscanf(optarg, "%lu", &latejoin_threshold) != 1)
					++errflag;
				break;
			case 'i':
				initial_topic_number = atoi(optarg);
				break;
			case 'l':
				msglen = atoi(optarg);
				break;
			case 'L':
				linger = atoi(optarg);
				break;
			case 'M':
				totalmsgsleft = atoi(optarg);
				break;
			case 'h':
				fprintf(stderr, "%s\n%s\n", lbm_version(), Purpose);
				fprintf(stderr, Usage, argv[0]);
				exit(0);
			case 'P':
				msecpause = atoi(optarg);
				break;
			case 'r':
				strncpy(topicroot, optarg, sizeof(topicroot));
				break;
			case 'R':
				errflag += parse_rate(optarg, &rm_protocol, &rm_rate, &rm_retrans);
				break;
			case 's':
				stats_sec = atoi(optarg);
				break;
			case 'S':
				num_srcs = atoi(optarg);
				if (num_srcs > MAX_NUM_SRCS)
				{
					fprintf(stderr, "Too many sources specified. Max number of sources is %d.\n", MAX_NUM_SRCS);
					errflag++;
				}
				break;
			case 'T':
				num_thrds = atoi(optarg);
				if (num_thrds > MAX_NUM_THREADS)
				{
					fprintf(stderr, "Too many threads specified. Max number of threads is %d.\n", MAX_NUM_THREADS);
					errflag++;
				}
				break;
			case 'v':
				verbose++;
				break;
			case 'X':
				if (optarg != NULL) {
					strncpy(opts->xml_config, optarg, (sizeof(opts->xml_config)-1));
					opts->xml_config[(sizeof(opts->xml_config)-1)]='\0';
				} else {
					errflag++;
				}
				break;
			case 'Y':
				if (optarg != NULL) {
					strncpy(opts->xml_appname, optarg, (sizeof(opts->xml_appname)-1));
					opts->xml_appname[(sizeof(opts->xml_appname)-1)]='\0';
				} else {
					errflag++;
				}
				break;
			case OPTION_CONTEXT_STATS:
				opts->context_stats = 1;
				break;
			default:
				errflag++;
				break;
		}
	}
	if (errflag != 0)
	{
		fprintf(stderr, "%s\n", lbm_version());
		fprintf(stderr, Usage, argv[0]);
		exit(1);
	}
	if (num_thrds > num_srcs) {
		fprintf(stderr, "Number of threads must be less than or equal to number of sources.\n");
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

	/* Retrieve current context settings */
	if (lbm_context_attr_create(&cattr) == LBM_FAILURE) {
 		fprintf(stderr, "lbm_context_attr_create: %s\n", lbm_errmsg());
 		exit(1);
 	}

	if (lbm_src_topic_attr_create (&tattr) == LBM_FAILURE) { 
		fprintf(stderr, "lbm_src_topic_attr_create: %s\n", lbm_errmsg());
		exit(1);
	}

	{
		/* Retrieve the context name and fill the context and topic attribute objects
		 * with specified configuration
		 */
		char ctx_name[256];
		size_t ctx_name_len = sizeof(ctx_name);
		if (lbm_context_attr_str_getopt(cattr, "context_name", ctx_name, &ctx_name_len) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_attr_str_getopt - context_name: %s\n", lbm_errmsg());
			exit(1);
		}
		/* fill context attribute object */
		if (lbm_context_attr_set_from_xml(cattr, ctx_name) == LBM_FAILURE) {
			fprintf(stderr, "lbm_context_attr_set_from_xml - context_name: %s\n", lbm_errmsg());
			exit(1);
		}

	}

	/* If the user specified a rate control, set the transport to LBM-RM in the topic attribute structure and
	 * and the requested rate control values in the context attribute structure
	 */
 	if (rm_rate != 0) {
 		printf("Sending with LBT-R%c data rate limit %" PRIu64 ", retransmission rate limit %" PRIu64 "\n", 
			rm_protocol,rm_rate, rm_retrans);
		/* Set transport attribute to LBT-RM */
		switch(rm_protocol) {
		case 'M':
 			if (lbm_src_topic_attr_str_setopt(tattr, "transport", "LBTRM") != 0) {
 				fprintf(stderr, "lbm_src_topic_str_setopt:transport: %s\n", lbm_errmsg());
 				exit(1);
 			}
			/* Set LBT-RM data rate attribute */
 			if (lbm_context_attr_setopt(cattr, "transport_lbtrm_data_rate_limit", &rm_rate, sizeof(rm_rate)) != 0) {
 				fprintf(stderr, "lbm_context_attr_setopt:transport_lbtrm_data_rate_limit: %s\n", lbm_errmsg());
 				exit(1);
 			}
			/* Set LBT-RM retransmission rate attribute */
 			if (lbm_context_attr_setopt(cattr, "transport_lbtrm_retransmit_rate_limit", &rm_retrans, sizeof(rm_retrans)) != 0) {
 				fprintf(stderr, "lbm_context_attr_setopt:transport_lbtrm_retransmit_rate_limit: %s\n", lbm_errmsg());
 				exit(1);
 			}
			break;
		case 'U':
 			if (lbm_src_topic_attr_str_setopt(tattr, "transport", "LBTRU") != 0) {
 				fprintf(stderr, "lbm_src_topic_str_setopt:transport: %s\n", lbm_errmsg());
 				exit(1);
 			}
			/* Set LBT-RU data rate attribute */
 			if (lbm_context_attr_setopt(cattr, "transport_lbtru_data_rate_limit", &rm_rate, sizeof(rm_rate)) != 0) {
 				fprintf(stderr, "lbm_context_attr_setopt:transport_lbtru_data_rate_limit: %s\n", lbm_errmsg());
 				exit(1);
 			}
			/* Set LBT-RU retransmission rate attribute */
 			if (lbm_context_attr_setopt(cattr, "transport_lbtru_retransmit_rate_limit", &rm_retrans, sizeof(rm_retrans)) != 0) {
 				fprintf(stderr, "lbm_context_attr_setopt:transport_lbtru_retransmit_rate_limit: %s\n", lbm_errmsg());
 				exit(1);
 			}
			break;
		}
 	}
	/* Create LBM context (passing in context attributes) */
	if (lbm_context_create(&ctx, cattr, NULL, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_context_create: %s\n", lbm_errmsg());
		exit(1);
	}
	lbm_context_attr_delete(cattr);

	if (latejoin_threshold > 0)
	{
		if (lbm_src_topic_attr_str_setopt(tattr, "late_join", "1") != 0) {
			fprintf(stderr,"lbm_src_topic_attr_str_setopt:late_join: %s\n", lbm_errmsg());
			exit(1);
		}
		if (lbm_src_topic_attr_setopt(tattr, "retransmit_retention_size_threshold",
					&latejoin_threshold, sizeof(latejoin_threshold)) != 0) {
			fprintf(stderr,"lbm_src_topic_attr_setopt:retransmit_retention_size_threshold: %s\n", lbm_errmsg());
			exit(1);
		}
		printf("Enabled Late Join with message retention threshold set to %lu bytes.\n", latejoin_threshold);
	}

	if ((srcs = malloc(sizeof(lbm_src_t *) * MAX_NUM_SRCS)) == NULL) {
		fprintf(stderr, "could not allocate sources array\n");
		exit(1);
	}

	/* Create all the sources */
	printf("Creating %d sources\n", num_srcs);
	for (i = 0; i < num_srcs; i++) {
		/* If create LOTS of srcs at full speed, it's pretty hard
		 * on topic resolution.  Space it out just a little bit. */
		if (i % 2000 == 0) {
			SLEEP_MSEC(100);
		}
		sprintf(topicname, "%s.%d", topicroot, (i + initial_topic_number));
		topic = NULL;
		/* First allocate the desired topic */
		if (lbm_src_topic_alloc(&topic, ctx, topicname, tattr) == LBM_FAILURE) {
			fprintf(stderr, "lbm_src_topic_alloc: %s\n", lbm_errmsg());
			exit(1);
		}
		/*
		 * Create LBM source passing in the allocated topic and event
		 * handler. The source object is returned here in &(srcs[i]).
		 */
		if (lbm_src_create(&(srcs[i]), ctx, topic, handle_src_event, NULL, NULL) == LBM_FAILURE) {
			fprintf(stderr, "lbm_src_create: %s\n", lbm_errmsg());
			exit(1);
		}
		
		if (i > 1 && (i % 1000) == 0)
			printf("Created %d sources\n", i);
	}
	lbm_src_topic_attr_delete(tattr);

	if (delay > 0) {
		printf("Delaying sending for %d second%s...\n", delay, ((delay > 1) ? "s" : ""));
		SLEEP_SEC(delay);
	}
	printf("Created %d Sources. Will start sending data now.\n",num_srcs);

	printf("Using %d threads to send %u messages of size %u bytes (%u messages per thread).\n",
		   num_thrds, totalmsgsleft, (unsigned int)msglen, totalmsgsleft / num_thrds);

	/* Divide sending load amongst available threads */
	for (i = 1; i < num_thrds; i++) {
#if defined(_WIN32)
		thrdidxs[i] = i;
		msgsleft[i] = totalmsgsleft / num_thrds;
		if ((wthrdh[i] = CreateThread(NULL, 0, sending_thread_main, &(thrdidxs[i]), 0,
									  &(wthrdids[i]))) == NULL) {
			fprintf(stderr, "could not create thread\n");
			exit(1);
		}
#else
		thrdidxs[i] = i;
		msgsleft[i] = totalmsgsleft / num_thrds;
		if (pthread_create(&(pthids[i]), NULL, sending_thread_main, &(thrdidxs[i])) != 0) {
			fprintf(stderr, "could not spawn thread\n");
			exit(1);
		}
#endif /* _WIN32 */
	}
	thrdidxs[0] = 0;
	msgsleft[0] = totalmsgsleft / num_thrds;

	/* If a statistics were requested, setup an LBM timer to the dump the statistics */
	if (stats_sec > 0) {
		/* Schedule time to handle statistics display. */
		if ((stats_timer_id = lbm_schedule_timer(ctx, handle_stats_timer, ctx, NULL, (stats_sec * 1000))) == -1) {
			fprintf(stderr, "lbm_schedule_timer: %s\n", lbm_errmsg());
			exit(1);
		}
	}
	sending_thread_main(&(thrdidxs[0]));

	printf("Done sending on thread 0. Waiting for any other threads to finish.\n");

	/* Join the created threads */
	for (i = 1; i < num_thrds; i++) {
#if defined(_WIN32)
		printf("Waiting on thread %d\n", i);
		WaitForSingleObject(wthrdh[i], INFINITE);
		printf("Thread %d done\n", i);
#else
		printf("Joining thread %d\n", i);
		pthread_join(pthids[i], NULL);
		printf("Joined thread %d\n", i);
#endif /* _WIN32 */
	}
	done_sending = 1;

	/* we do this before the linger, so some things may be in batching, etc. and not show up in stats. */
	handle_stats_timer(ctx, ctx);

	/*
	 * Linger allows transport to complete data transfer and recovery before
	 * context is deleted and socket is torn down.
	 */
	printf("Lingering for %d seconds...\n",linger);
	SLEEP_SEC(linger);

	printf("Deleting sources....\n");
	for (i = 0; i < num_srcs; i++) {
		lbm_src_delete(srcs[i]);
		srcs[i] = NULL;
		if (i > 1 && (i % 1000) == 0)
			printf("Deleted %d sources\n",i);
	}
	lbm_context_delete(ctx);
	ctx = NULL;

	/* Free the source array and message buffer used for sending */
	free(srcs);
	return 0;
}

