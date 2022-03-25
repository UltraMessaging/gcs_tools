/*
  Copyright (C) 2005-2021, Informatica Corporation  Permission is granted to licensees to use
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
	#ifdef __TANDEM
		#include <strings.h>
	#endif
#endif
#include "replgetopt.h"
#include <lbm/lbm.h>
#include <lbm/lbmmon.h>
#include "monmodopts.h"
#include "verifymsg.h"
#include "lbm-example-util.h"

#define MIN_ALLOC_MSGLEN 25
#define DEFAULT_MAX_MESSAGES 10000000
#define DEFAULT_DELAY_B4CLOSE 5
#define MAX_MESSAGE_PROPERTIES_INTEGER_COUNT 16		/* SSRC_MAX_MSG_PROP_INT_CNT */
#define MAX_MESSAGE_PROPERTIES_NAME_LEN 7			/* ssrc_MAX_MSG_PROP_NAME_LEN */

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

/* Lines starting with double quote are extracted for UM documentation. */

const char purpose[] = "Purpose: "
"application that uses Smart Source to send to a single topic."
;

const char usage[] =
"Usage: lbmssrc [options] topic\n"
"Available options:\n"
"  -a, --available-data-space  print the length of available data space\n"
"  -b, --user-supplied-buffer  send messages using a user-supplied buffer\n"
"  -c, --config=FILE           Use LBM configuration file FILE.\n"
"                              Multiple config files are allowed.\n"
"                              Example:  '-c file1.cfg -c file2.cfg'\n"
"  -d, --delay=NUM             delay sending for NUM seconds after source creation\n"
"  -h, --help                  display this help and exit\n"
"  -i, --int-mprop=VAL,KEY     send integer message property value VAL with name KEY\n"
"  -j, --late-join=NUM         enable Late Join with specified retention buffer count\n"
"  -l, --length=NUM            send messages of NUM bytes\n"
"  -L, --linger=NUM            linger for NUM seconds before closing context\n"
"  -M, --messages=NUM          send NUM messages\n"
"  -N, --channel=NUM           send on channel NUM\n"
"  -S, --perf-stats=NUM,OT     print performance stats every NUM messages sent\n"
"                              If optional OT is given, override the default 10 usec Outlier Threshold\n"
"  -P, --pause=NUM             pause NUM milliseconds after each send\n"
"  -s, --statistics=NUM        print statistics every NUM seconds\n"
"  -v, --verbose               be verbose; add per message data\n"
"  -V, --verifiable            construct verifiable messages\n"
MONOPTS_SENDER
MONMODULEOPTS_SENDER;

const char * OptionString = "abc:d:j:hi:L:l:M:N:P:S:s:vV";
#define OPTION_MONITOR_SRC 0
#define OPTION_MONITOR_CTX 1
#define OPTION_MONITOR_TRANSPORT 2
#define OPTION_MONITOR_TRANSPORT_OPTS 3
#define OPTION_MONITOR_FORMAT 4
#define OPTION_MONITOR_FORMAT_OPTS 5
#define OPTION_MONITOR_APPID 6
const struct option OptionTable[] =
{
	{ "available-data-space", no_argument, NULL, 'a' },
	{ "config", required_argument, NULL, 'c' },
	{ "delay", required_argument, NULL, 'd' },
	{ "help", no_argument, NULL, 'h' },
	{ "int-mprop", required_argument, NULL, 'i' },
	{ "late-join", required_argument, NULL, 'j' },
	{ "length", required_argument, NULL, 'l' },
	{ "linger", required_argument, NULL, 'L' },
	{ "messages", required_argument, NULL, 'M' },
	{ "channel", required_argument, NULL, 'N' },
	{ "pause", required_argument, NULL, 'P' },
	{ "statistics", required_argument, NULL, 's' },
	{ "user-supplied-buffer", no_argument, NULL, 'b' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "verifiable", no_argument, NULL, 'V' },
	{ "monitor-src", required_argument, NULL, OPTION_MONITOR_SRC },
	{ "monitor-ctx", required_argument, NULL, OPTION_MONITOR_CTX },
	{ "monitor-transport", required_argument, NULL, OPTION_MONITOR_TRANSPORT },
	{ "monitor-transport-opts", required_argument, NULL, OPTION_MONITOR_TRANSPORT_OPTS },
	{ "monitor-format", required_argument, NULL, OPTION_MONITOR_FORMAT },
	{ "monitor-format-opts", required_argument, NULL, OPTION_MONITOR_FORMAT_OPTS },
	{ "monitor-appid", required_argument, NULL, OPTION_MONITOR_APPID },
	{ "perf-stats", required_argument, NULL, 'S' },
	{ NULL, 0, NULL, 0 }
};

struct Options {
	char transport_options_string[1024];	/* Transport Options given to lbmmon_sctl_create() */
	char format_options_string[1024];		/* Format Options given to lbmmon_sctl_create() */
	char application_id_string[1024];		/* Application ID given to lbmmon_context_monitor()	*/
	unsigned int msgs;						/* Number of messages to be sent */
	size_t msglen;							/* Length of messages to be sent */
	int latejoin_threshold; 				/* Maximum Late Join buffer count */
	int pause;								/* Pause interval between messages */
	int delay, linger;						/* Interval to linger before and after sending messages	*/
	lbm_ulong_t stats_sec;					/* Interval for dumping statistics */
	int verbose;							/* Flag to control output verbosity per message */
	int verifiable_msgs;					/* Flag to control message verification (verifymsg.h) */
	int monitor_context;					/* Flag to control context level monitoring */
	int monitor_context_ivl;				/* Interval for context level monitoring */
	int monitor_source;						/* Flag to control source level monitoring */
	int monitor_source_ivl;					/* Interval for source level monitoring */
	lbmmon_transport_func_t * transport;	/* Function pointer to chosen transport module */
	lbmmon_format_func_t * format;			/* Function pointer to chosen format module */
	char *topic;							/* The topic to be sent on */
	int perf_stats;							/* Performance stats in frequency (number of messages) */
	int perf_thresh;						/* Performance stats outlier threshold in usecs */
	long channel_number;					/* The channel (sub-topic) number to use */
	char mprop_int_keys[16][1024];			/* message property integer keys */
	lbm_int32_t mprop_int_vals[16];			/* message property integer values */
	int mprop_int_cnt;						/* message property integer count */
	int user_supplied_buffer;				/* send messages with a user supplied buffer */
	int available_data_space;				/* print the length of available data space */

};

#define TIME_FMT "%" PRIu64 ".%.9" PRIu64 " "

void current_time_hr(struct timeval *tv)
{
#if defined(_WIN32)
	LARGE_INTEGER ticks;
	static LARGE_INTEGER freq;
	static int first = 1;

	if (first) {
		QueryPerformanceFrequency(&freq);
		first = 0;
	}
	QueryPerformanceCounter(&ticks);
	tv->tv_sec = 0;
	tv->tv_usec = (1000000 * ticks.QuadPart / freq.QuadPart);
	normalize_tv(tv);
#else
	gettimeofday(tv,NULL);
#endif /* _WIN32 */
}

/* For the elapsed time, calculate and print the msgs/sec and bits/sec */
void print_bw(FILE *fp, struct timeval *tv, unsigned int msgs, unsigned long long bytes)
{
	char scale[] = {'\0', 'K', 'M', 'G'};
	int msg_scale_index = 0, bit_scale_index = 0;
	double sec = 0.0, mps = 0.0, bps = 0.0;
	double kscale = 1000.0;
	
	if (tv->tv_sec == 0 && tv->tv_usec == 0) return;/* avoid div by 0 */	
	sec = (double)tv->tv_sec + (double)tv->tv_usec / 1000000.0;
	mps = (double)msgs/sec;
	bps = ((double)(bytes<<3))/sec;
	
	while (mps >= kscale) {
		mps /= kscale;
		msg_scale_index++;
	}
	
	while (bps >= kscale) {
		bps /= kscale;
		bit_scale_index++;
	}
	
	fprintf(fp, "%.04g secs. %.04g %cmsgs/sec. %.04g %cbps\n", sec,
			mps, scale[msg_scale_index], bps, scale[bit_scale_index]);
	fflush(fp);
}

/* Print transport statistics */
void print_stats(FILE *fp, lbm_ssrc_t *ssrc)
{
	lbm_src_transport_stats_t stats;

	/* Retrieve source transport statistics */
	if (lbm_ssrc_retrieve_transport_stats(ssrc, &stats) == LBM_FAILURE) {
		fprintf(stderr, "lbm_ssrc_retrieve_transport_stats: %s\n", lbm_errmsg());
		exit(1);
	}
	switch (stats.type) {
	case LBM_TRANSPORT_STAT_TCP:
		fprintf(fp, "TCP, buffered %lu, clients %lu\n",stats.transport.tcp.bytes_buffered,
				stats.transport.tcp.num_clients);
		break;
	case LBM_TRANSPORT_STAT_LBTRM:
		fprintf(fp, "LBT-RM, sent %lu/%lu, txw %lu/%lu, naks %lu/%lu, ignored %lu/%lu, shed %lu, rxs %lu, rctlr %lu/%lu\n",
				stats.transport.lbtrm.msgs_sent, stats.transport.lbtrm.bytes_sent,
				stats.transport.lbtrm.txw_msgs, stats.transport.lbtrm.txw_bytes,
				stats.transport.lbtrm.naks_rcved, stats.transport.lbtrm.nak_pckts_rcved,
				stats.transport.lbtrm.naks_ignored, stats.transport.lbtrm.naks_rx_delay_ignored,
				stats.transport.lbtrm.naks_shed,
				stats.transport.lbtrm.rxs_sent,
				stats.transport.lbtrm.rctlr_data_msgs, stats.transport.lbtrm.rctlr_rx_msgs);
		break;
	case LBM_TRANSPORT_STAT_LBTRU:
		fprintf(fp, "LBT-RU, clients %lu, sent %lu/%lu, naks %lu/%lu, ignored %lu/%lu, shed %lu, rxs %lu\n",
				stats.transport.lbtru.num_clients,
				stats.transport.lbtru.msgs_sent, stats.transport.lbtru.bytes_sent,
				stats.transport.lbtru.naks_rcved, stats.transport.lbtru.nak_pckts_rcved,
				stats.transport.lbtru.naks_ignored, stats.transport.lbtru.naks_rx_delay_ignored,
				stats.transport.lbtru.naks_shed,
				stats.transport.lbtru.rxs_sent);
		break;
	case LBM_TRANSPORT_STAT_LBTIPC:
		fprintf(fp, "LBT-IPC, clients %lu, sent %lu/%lu\n",
				stats.transport.lbtipc.num_clients,
				stats.transport.lbtipc.msgs_sent, stats.transport.lbtipc.bytes_sent);
		break;
	case LBM_TRANSPORT_STAT_LBTSMX:
		fprintf(fp, "LBT-SMX, clients %lu, sent %lu/%lu\n",
				stats.transport.lbtsmx.num_clients,
				stats.transport.lbtsmx.msgs_sent, stats.transport.lbtsmx.bytes_sent);
		break;
	case LBM_TRANSPORT_STAT_LBTRDMA:
		fprintf(fp, "LBT-RDMA, clients %lu, sent %lu/%lu\n",
				stats.transport.lbtrdma.num_clients,
				stats.transport.lbtrdma.msgs_sent, stats.transport.lbtrdma.bytes_sent);
		break;
	default:
		break;
	}
	fflush(fp);
}

void print_help_exit(char **argv, int exit_value){
	fprintf(stderr, "%s\n%s\n%s\n%s",
		argv[0], lbm_version(), purpose, usage);
	exit(exit_value);
}

/* Logging callback */
int lbm_log_msg(int level, const char *message, void *clientd)
{
	printf("LOG Level %d: %s\n", level, message);
	return 0;
}

struct TimerControl {
	int stats_timer_id;
	lbm_ulong_t stats_msec;
	int stop_timer;
} timer_control = { -1, 0, 0 };


/* Source event handler callback (passed into lbm_ssrc_create()) */
int handle_ssrc_event(lbm_ssrc_t *ssrc, int event, void *extra_data, void *client_data)
{
	switch (event) {
	case LBM_SRC_EVENT_CONNECT:
		{
			const char *clientname = (const char *)extra_data;
			
			printf("Receiver connect [%s]\n",clientname);
		}
		break;
	case LBM_SRC_EVENT_DISCONNECT:
		{
			const char *clientname = (const char *)extra_data;
			
			printf("Receiver disconnect [%s]\n",clientname);
		}
		break;
	case LBM_SRC_EVENT_TIMESTAMP:
		{
			lbm_src_event_timestamp_info_t *info = (lbm_src_event_timestamp_info_t *)extra_data;
			struct Options *opts = (struct Options *)client_data;

			/* Print if very verbose */
			if (opts->verbose > 1) {
				fprintf(stdout, "The timestamp of the transmitted packet was " TIME_FMT " for SQN %u\n", (uint64_t)info->hr_timestamp.tv_sec,
					(uint64_t)info->hr_timestamp.tv_nsec, info->sequence_number);
			}
		}
		break;
	default:
		printf( "Unhandled source event [%d]. Refer to https://ultramessaging.github.io/currdoc/doc/example/index.html#unhandledcevents for a detailed description.\n", event);
		break;
	}
	return 0;
}

/* Timer callback to handle periodic display of source statistics */
int handle_stats_timer(lbm_context_t *ctx, const void *clientd)
{
	lbm_ssrc_t *ssrc = (lbm_ssrc_t *)clientd;

	print_stats(stdout, ssrc);
	if (!timer_control.stop_timer) {
		/* Schedule timer to call the function handle_stats_timer() to dump the current statistics */
		if ((timer_control.stats_timer_id = 
			lbm_schedule_timer(ctx, handle_stats_timer, ssrc, NULL, timer_control.stats_msec)) == -1)
		{
			fprintf(stderr, "lbm_schedule_timer: %s\n", lbm_errmsg());
			exit(1);
		}
	}
	return 0;
}

#define OUTLIER_THRESH_DEFAULT 10

void process_cmdline(int argc, char **argv,struct Options *opts)
{
	int c,errflag = 0;

	/* Set default option values */
	memset(opts, 0, sizeof(*opts));
	opts->latejoin_threshold = 0;
	opts->linger = DEFAULT_DELAY_B4CLOSE;
	opts->delay = 1;
	opts->msglen = MIN_ALLOC_MSGLEN;
	opts->msgs = DEFAULT_MAX_MESSAGES;
	opts->channel_number = -1;
	opts->transport = (lbmmon_transport_func_t *)lbmmon_transport_lbm_module();
	opts->format = (lbmmon_format_func_t *) lbmmon_format_csv_module();
	opts->perf_stats = 0;
	opts->perf_thresh = OUTLIER_THRESH_DEFAULT;
	opts->mprop_int_cnt = 0;
	opts->user_supplied_buffer = 0;
	opts->available_data_space = 0;

	/* Process the command line options, setting local variables with values */
	while ((c = getopt_long(argc, argv, OptionString, OptionTable, NULL)) != EOF) {
		switch (c) {
		case 'a':
			opts->available_data_space = 1;
			break;
		case 'b':
			opts->user_supplied_buffer = 1;
			break;
		case 'c':
			/* Initialize configuration parameters from a file. */
			if (lbm_config(optarg) == LBM_FAILURE) {
				fprintf(stderr, "lbm_config: %s\n", lbm_errmsg());
				exit(1);
			}
			break;
		case 'd':
			opts->delay = atoi(optarg);
			break;
		case 'i':
		{
			int rc;
			if (opts->mprop_int_cnt >= MAX_MESSAGE_PROPERTIES_INTEGER_COUNT) {
				fprintf(stderr, "Max number of integer properties exceeded\n");
				exit(1);
			}
			rc = sscanf(optarg, "%d,%s", &opts->mprop_int_vals[opts->mprop_int_cnt], &opts->mprop_int_keys[opts->mprop_int_cnt][0]);
			if (rc != 2) {
				fprintf(stderr, "Failed to parse int-mprop: %s\n", optarg);
				exit(1);
			}
			if (strlen(&opts->mprop_int_keys[opts->mprop_int_cnt][0]) > MAX_MESSAGE_PROPERTIES_NAME_LEN) {
				fprintf(stderr, "Integer message property key name (%s) is too long. Maximum key name length is %d characters.\n", &opts->mprop_int_keys[opts->mprop_int_cnt][0], MAX_MESSAGE_PROPERTIES_NAME_LEN);
				exit(1);
			}
			opts->mprop_int_cnt++;
		}
		break;
		case 'j':
			if (sscanf(optarg, "%d", &opts->latejoin_threshold) != 1)
				++errflag;
			break;
		case 'L':
			opts->linger = atoi(optarg);
			break;
		case 'l':
			opts->msglen = atoi(optarg);
			break;
		case 'M':
			opts->msgs = atoi(optarg);
			break;
		case 'N':
			opts->channel_number = atol(optarg);
			break;
		case 'S':
		{
			int rc;
			rc = sscanf(optarg, "%d,%d", &opts->perf_stats, &opts->perf_thresh);
			if (rc != 2) {
				opts->perf_stats = atol(optarg);
				opts->perf_thresh = OUTLIER_THRESH_DEFAULT;
			}
		}
		break;
		case 'h':
			print_help_exit(argv, 0);
		case 'P':
			opts->pause = atoi(optarg);
			break;
		case 's':
			opts->stats_sec = atoi(optarg);
			break;
		case 'v':
			opts->verbose++;
			break;
		case 'V':
			opts->verifiable_msgs = 1;
			break;
		case OPTION_MONITOR_CTX:
			opts->monitor_context = 1;
			opts->monitor_context_ivl = atoi(optarg);
			break;
		case OPTION_MONITOR_SRC:
			opts->monitor_source = 1;
			opts->monitor_source_ivl = atoi(optarg);
			break;
		case OPTION_MONITOR_TRANSPORT:
			if (optarg != NULL) {
				if (strcasecmp(optarg, "lbm") == 0) {
					opts->transport = (lbmmon_transport_func_t *)lbmmon_transport_lbm_module();
				} else if (strcasecmp(optarg, "udp") == 0) {
					opts->transport = (lbmmon_transport_func_t *)lbmmon_transport_udp_module();
				} else if (strcasecmp(optarg, "lbmsnmp") == 0) {
					opts->transport = (lbmmon_transport_func_t *)lbmmon_transport_lbmsnmp_module();
				} else {
					++errflag;
				}
			} else {
				++errflag;
			}
			break;
		case OPTION_MONITOR_TRANSPORT_OPTS:
			if (optarg != NULL) {
				strncpy(opts->transport_options_string, optarg, sizeof(opts->transport_options_string));
			}
			else {
				++errflag;
			}
			break;
		case OPTION_MONITOR_FORMAT:
			if (optarg != NULL) {
				if (strcasecmp(optarg, "csv") == 0) {
					opts->format = (lbmmon_format_func_t *)lbmmon_format_csv_module();
				} else if (strcasecmp(optarg, "pb") == 0) {
					opts->format = (lbmmon_format_func_t *)lbmmon_format_pb_module();
				} else {
					++errflag;
				}
			} else {
				++errflag;
			}
			break;
		case OPTION_MONITOR_FORMAT_OPTS:
			if (optarg != NULL) {
				strncpy(opts->format_options_string, optarg, sizeof(opts->format_options_string));
			} else {
				++errflag;
			}
			break;
		case OPTION_MONITOR_APPID:
			if (optarg != NULL) {
				strncpy(opts->application_id_string, optarg, sizeof(opts->application_id_string));
			} else {
				++errflag;
			}
			break;
		default:
			errflag++;
			break;
		}
	}
	if ((errflag != 0) || (optind == argc)) {
		/* An error occurred processing the command line - print help and exit */
		print_help_exit(argv, 1); 		
	}
	opts->topic = argv[optind];
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
	lbm_set_lbtrm_src_loss_rate(LossRate);
	lbm_set_lbtru_src_loss_rate(LossRate);
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
	lbm_set_lbtrm_src_loss_rate(LossRate);
	lbm_set_lbtru_src_loss_rate(LossRate);
}

static
void
SigUsr2Handler(int signo)
{
	LossRate = 0;
	lbm_set_lbtrm_src_loss_rate(LossRate);
	lbm_set_lbtru_src_loss_rate(LossRate);
}
#endif

void handle_elapsed_time(signed long elapsed, struct Options *opts)
{
	static int sample_count = 0;
	static signed long total_time = 0;
	static int outliers = 0;
	static signed long outlier_time = 0;
	static signed long max_outlier = 0;

	if (elapsed > opts->perf_thresh) {
		outliers++;
		outlier_time += elapsed;
		if (max_outlier < elapsed)
			max_outlier = elapsed;
	}
	total_time += elapsed;
	sample_count++;
	if (sample_count >= opts->perf_stats) {
		printf("PERFORMANCE: %d usec to send %d msgs; outliers (%d usec thresh): cnt: %d out time: %d max: %d\n",
				(int) total_time, (int) sample_count, (int) opts->perf_thresh, (int) outliers, (int) outlier_time, (int) max_outlier);
		sample_count = 0;
		total_time = 0;
		outliers = 0;
		outlier_time = 0;
		max_outlier = 0;
	}
}

int main(int argc, char **argv)
{
	struct Options options,*opts = &options;
	double secs = 0.0;
	lbm_context_t *ctx;
	lbm_topic_t *topic;
	lbm_ssrc_t *ssrc;
	lbm_src_topic_attr_t * tattr;
	lbm_context_attr_t * cattr;
	struct timeval starttv, endtv;
	int count = 0;
	unsigned long long bytes_sent = 0;
	char *message = NULL;
	char *user_supplied_buffer = NULL;
	lbmmon_sctl_t * monctl;
	lbm_ssrc_send_ex_info_t info;
	char *key_ptrs[16];
	int err;
	int msg_props = 0;
	int spectrum = 0;

	int transport;		/* transport in use (used for SMX testing) */
	size_t transize = 4;
	info.flags = 0;

#if defined(_WIN32)
	{
		WSADATA wsadata;
		int status;
		
		/* Windows socket startup code */
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

	/* Process the different options set by the command line processing */
	process_cmdline(argc,argv,opts);

	/* If set, check the requested message length is not too small */
	if (opts->verifiable_msgs != 0) {
		size_t min_msglen = minimum_verifiable_msglen();
		if (opts->msglen < min_msglen) {
			printf("Specified message length %u is too small for verifiable messages.\n", (unsigned) opts->msglen);
			printf("Setting message length to minimum (%u).\n", (unsigned) min_msglen);
			opts->msglen = min_msglen;
		}
	}
	
	/* Setup logging callback */
	if (lbm_log(lbm_log_msg, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_log: %s\n", lbm_errmsg());
		exit(1);
	}

	/* Retrieve current context settings */
	if (lbm_context_attr_create(&cattr) == LBM_FAILURE) {
 		fprintf(stderr, "lbm_context_attr_create: %s\n", lbm_errmsg());
 		exit(1);
 	}

	/* Retrieve current source topic settings */
	if (lbm_src_topic_attr_create(&tattr) == LBM_FAILURE) {
 		fprintf(stderr, "lbm_src_topic_attr_create: %s\n", lbm_errmsg());
 		exit(1);
 	}

	/* If user specified a Late Join threshold, set the value in the context attribute structure */
	if (opts->latejoin_threshold > 0) {
		if (lbm_src_topic_attr_str_setopt(tattr, "late_join", "1") != 0) {
			fprintf(stderr,"lbm_src_topic_attr_str_setopt:late_join: %s\n", lbm_errmsg());
			exit(1);
		}
		if (lbm_src_topic_attr_setopt(tattr, "smart_src_retention_buffer_count",
					&opts->latejoin_threshold, sizeof(opts->latejoin_threshold)) != 0) {
			fprintf(stderr,"lbm_src_topic_attr_setopt:smart_src_retention_buffer_count: %s\n", lbm_errmsg());
			exit(1);
		}

		printf("Enabled Late Join with message retention buffer count set to %d.\n", opts->latejoin_threshold);

	}

	/* Create LBM context (passing in context attributes) */
	if (lbm_context_create(&ctx, cattr, NULL, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_context_create: %s\n", lbm_errmsg());
		exit(1);
	}
	lbm_context_attr_delete(cattr);

#if !defined(_WIN32)
	signal(SIGHUP, SigHupHandler);
	signal(SIGUSR1, SigUsr1Handler);
	signal(SIGUSR2, SigUsr2Handler);
#endif

	/* Only LBT-RM and LBT-RU supported for Smart Source right now */
	if (lbm_src_topic_attr_getopt(tattr, "transport", &transport, &transize) == LBM_FAILURE) {
		fprintf(stderr, "lbm_src_topic_attr_getopt: %s\n", lbm_errmsg());
		exit(1);
	}
	if (!(transport == LBM_SRC_TOPIC_ATTR_TRANSPORT_LBTRM || transport == LBM_SRC_TOPIC_ATTR_TRANSPORT_LBTRU)) {
		fprintf(stderr, "WARNING: only LBT-RM and LBT-RU are supported at this time. Changing to LBT-RM.\n");
		if (lbm_src_topic_attr_str_setopt(tattr, "transport", "LBTRM") != 0) {
			fprintf(stderr, "lbm_src_topic_attr_str_setopt: %s\n", lbm_errmsg());
			exit(1);
		}
	}

	/*  Config | Cmd Line |
	 * --------|----------| Action
	 *  MP  SC |  MP  SC  |
	 * --------|----------|-------------------------------------
	 *  0   0  |  0   0   | Do Nothing
	 *  0   0  |  0   1   | Enable SC; Send SC
	 *  0   0  |  1   0   | Enable MP; Send MP
	 *  0   0  |  1   1   | Enable SC & MP; Send SC & MP
	 *  0   1  |  0   0   | Don't send SC
	 *  0   1  |  0   1   | Send SC
	 *  0   1  |  1   0   | Error example app: must provide SC #
	 *  0   1  |  1   1   | Enable MP; Send SC & MP
	 *  1   0  |  0   0   | Don't send MP
	 *  1   0  |  0   1   | Error example app: must provide SC #
	 *  1   0  |  1   0   | Send MP
	 *  1   0  |  1   1   | Enable SC; Send SC & MP
	 *  1   1  |  0   0   | Don't send SC or MP
	 *  1   1  |  0   1   | Error example app: must provide MP(s)
	 *  1   1  |  1   0   | Error example app: must provide SC #
	 *  1   1  |  1   1   | Send SC & MP
	 */
	{
		size_t msg_props_size;
		msg_props_size = sizeof(int);
		if (lbm_src_topic_attr_getopt(tattr, "smart_src_message_property_int_count", &msg_props, &msg_props_size) == LBM_FAILURE) {
			fprintf(stderr, "lbm_src_topic_attr_getopt: %s\n", lbm_errmsg());
			exit(1);
		}
	}
	{
		size_t spectrum_size;
		spectrum_size = sizeof(int);
		if (lbm_src_topic_attr_getopt(tattr, "smart_src_enable_spectrum_channel", &spectrum, &spectrum_size) == LBM_FAILURE) {
			fprintf(stderr, "lbm_src_topic_attr_getopt: %s\n", lbm_errmsg());
			exit(1);
		}
	}
	if (opts->mprop_int_cnt > 0) {
		if (msg_props < opts->mprop_int_cnt) {	/* Turn on message properties */
			fprintf(stderr, "WARNING: found Message Properties on the command line but smart_src_message_property_int_count is either not configured or too small; setting config parameter to %d.\n", opts->mprop_int_cnt);
			if (lbm_src_topic_attr_setopt(tattr, "smart_src_message_property_int_count", &(opts->mprop_int_cnt), sizeof(opts->mprop_int_cnt)) != 0) {
				fprintf(stderr, "lbm_src_topic_attr_setopt: %s\n", lbm_errmsg());
				exit(1);
			}
		}
		if (spectrum && (opts->channel_number == -1)) {
			fprintf(stderr, "ERROR: smart_src_enable_spectrum_channel is configured but Channel Number was not found on the command line. Try again with -N option.\n");
			exit(1);
		}
	}
	if (opts->channel_number != -1) {
		if (!spectrum) { /* Turn on spectrum channel */
			fprintf(stderr, "WARNING: found Channel Number on the command line but smart_src_enable_spectrum_channel is not enabled; setting config parameter to enabled.\n");
			if (lbm_src_topic_attr_str_setopt(tattr, "smart_src_enable_spectrum_channel", "1") != 0) {
				fprintf(stderr, "lbm_src_topic_attr_str_setopt: %s\n", lbm_errmsg());
				exit(1);
			}
		}
		if (msg_props && (opts->mprop_int_cnt == 0)) {
			fprintf(stderr, "ERROR: smart_src_message_property_int_count is configured but Message Properties were not found on the command line. Try again with -i option.\n");
			exit(1);
		}
	}

	/* Allocate the desired topic */
	if (lbm_src_topic_alloc(&topic, ctx, opts->topic, tattr) == LBM_FAILURE) {
		fprintf(stderr, "lbm_src_topic_alloc: %s\n", lbm_errmsg());
		exit(1);
	}

	lbm_src_topic_attr_delete(tattr);

	/*
	 * Create LBM smart source passing in the allocated topic and event
	 * handler. The smart source object is returned here in &ssrc.
	 */
	if (lbm_ssrc_create(&ssrc, ctx, topic, handle_ssrc_event, opts, NULL) == LBM_FAILURE) {
		fprintf(stderr, "lbm_ssrc_create: %s\n", lbm_errmsg());
		exit(1);
	}

	if (opts->available_data_space) {
		int length = 0;
		if (lbm_ssrc_get_available_data_space(ssrc, &length) == LBM_FAILURE) {
			fprintf(stderr, "lbm_ssrc_get_available_data_space: %s\n", lbm_errmsg());
			exit(1);
		}
		printf("The length of available data space: %d.\n", length);
	}

	/* If a statistics were requested, setup an LBM timer to the dump the statistics */
	if (opts->stats_sec > 0) {
		timer_control.stats_msec = opts->stats_sec * 1000;

		/* Schedule timer to call the function handle_stats_timer() to dump the current statistics */
		if ((timer_control.stats_timer_id = 
			lbm_schedule_timer(ctx, handle_stats_timer, ssrc, NULL, timer_control.stats_msec)) == -1)
		{
			fprintf(stderr, "lbm_schedule_timer: %s\n", lbm_errmsg());
			exit(1);
		}
	}

	/* If monitoring options were selected, setup lbmmon */
	if (opts->monitor_context || opts->monitor_source) {
		char * transport_options = NULL;
		char * format_options = NULL;
		char * application_id = NULL;

		/* lbmmon_sctl_create, lbmmon_context_monitor and lbmmon_ssrc_monitor
		 * must be set to NULL or a valid value. Use local pointers to point
		 * to the options array if a valid value was provided on the command line.
		 */
		if (strlen(opts->transport_options_string) > 0) {
			transport_options = opts->transport_options_string;
		}
		if (strlen(opts->format_options_string) > 0) {
			format_options = opts->format_options_string;
		}
		if (strlen(opts->application_id_string) > 0) {
			application_id = opts->application_id_string;
		}

		/* Create the source monitor controller based on requested options */
		if (lbmmon_sctl_create(&monctl, opts->format, format_options, opts->transport, transport_options) == -1) {
			fprintf(stderr, "lbmmon_sctl_create() failed, %s\n", lbmmon_errmsg());
			exit(1);
		}

		/* Register the source/context for monitoring */
		if (opts->monitor_context) {
			if (lbmmon_context_monitor(monctl, ctx, application_id, opts->monitor_context_ivl) == -1) {
				fprintf(stderr, "lbmmon_context_monitor() failed, %s\n", lbmmon_errmsg());
				exit(1);
			}
		} else {
			if (lbmmon_ssrc_monitor(monctl, ssrc, application_id, opts->monitor_source_ivl) == -1) {
				fprintf(stderr, "lbmmon_ssrc_monitor() failed, %s\n", lbmmon_errmsg());
				exit(1);
			}
		}
	}

	if (opts->perf_stats) {
		printf("Displaying performance measurements every %d messages with an outlier threshold of %d usecs\n", opts->perf_stats, opts->perf_thresh);
	}

	/* Give the system a chance to cleanly initialize.
	 * When using LBT-RM, this allows topic resolution to occur and
	 * existing receivers to be aware of this new smart source.
	 */
	if (opts->delay > 0) {
		printf("Will start sending in %d second%s...\n", opts->delay, ((opts->delay > 1) ? "s" : ""));
		SLEEP_SEC(opts->delay);
	}
	/* */
	if (opts->channel_number >= 0) {
		printf("Sending on channel %ld\n", opts->channel_number);
		info.flags = LBM_SSRC_SEND_EX_FLAG_CHANNEL;
		info.channel = opts->channel_number;
	}
	if (opts->mprop_int_cnt > 0) {
		int i;
		printf("Sending integer message properties:\n");
		for (i = 0; i < opts->mprop_int_cnt; ++i) {
			printf("\tproperty #%d: %s = %d\n", i + 1, opts->mprop_int_keys[i], opts->mprop_int_vals[i]);
			key_ptrs[i] = opts->mprop_int_keys[i];
		}
		info.flags |= LBM_SSRC_SEND_EX_FLAG_PROPERTIES;
		info.mprop_int_cnt = opts->mprop_int_cnt;
		info.mprop_int_keys = key_ptrs;
		info.mprop_int_vals = opts->mprop_int_vals;
	}
	if (opts->user_supplied_buffer) {
		info.flags |= LBM_SSRC_SEND_EX_FLAG_USER_SUPPLIED_BUFFER;
		if (opts->msglen > 0) {
			user_supplied_buffer = (char *)malloc(opts->msglen);
			memset(user_supplied_buffer, 0, opts->msglen);
			info.usr_supplied_buffer = user_supplied_buffer;
			snprintf(user_supplied_buffer, opts->msglen, "User supplied buffer message... ");
		} else {
			info.usr_supplied_buffer = (char *)(&user_supplied_buffer);	/* initializing to a non-null address for zero-length message case */
		}
	}

	/* Start sending messages to whomever is listening */
	printf("lbmssrc - Sending %u messages of size %u bytes to topic [%s]\n",
		   opts->msgs, (unsigned)opts->msglen, opts->topic);
	current_tv(&starttv); /* Store the start time */
	if (lbm_ssrc_buff_get(ssrc, &message, 0) == LBM_FAILURE) {
		fprintf(stderr, "lbm_ssrc_buff_get: %s\n", lbm_errmsg());
		exit(1);
	}
	for (count = 0; count < opts->msgs; ) {

		/* Create a dummy message to send */
		if (opts->verifiable_msgs) {
				construct_verifiable_msg((char *)message, opts->msglen);
		} else {
			/* Only put data in message if verbose since this is performance sensitive */
			if (opts->verbose)
				sprintf((char *)message, "message %u", count);
		}

		if (opts->perf_stats) {
			struct timeval begin, end;
			signed long elapsed;

			current_time_hr(&begin);

			/* Send message using allocated source */
			err = lbm_ssrc_send_ex(ssrc, message, opts->msglen, 0, &info);

			current_time_hr(&end);
			elapsed = ((signed long) (end.tv_sec - begin.tv_sec)) * 1000000;
			elapsed += ((signed long) (end.tv_usec - begin.tv_usec));
			handle_elapsed_time(elapsed, opts);
		}
		else {
			/* Send message using allocated source */
			err = lbm_ssrc_send_ex(ssrc, message, opts->msglen, 0, &info);
			/* Uncomment to increment/update the message property integer values with each send 
			if (opts->mprop_int_cnt > 0) {
				int i;
				for (i = 0; i < opts->mprop_int_cnt; ++i) {
					opts->mprop_int_vals[i] += 1;
				}
				info.flags |= LBM_SSRC_SEND_EX_FLAG_UPDATE_PROPERTY_VALUES;
			}
			*/
		}

		if ( err == LBM_FAILURE) {
			/* Unexpected error occurred */
			fprintf(stderr, "lbm_ssrc_send_ex: %s\n", lbm_errmsg());
			exit(1);
		}
		bytes_sent += (unsigned long long) opts->msglen;
		count++;

		/* The user requested to pause between each packet, do so */
		if (opts->pause > 0) {
			SLEEP_MSEC(opts->pause);
		}
	}

	/* Calculate the time it took to send the messages and dump */
	current_tv(&endtv);
	endtv.tv_sec -= starttv.tv_sec;
	endtv.tv_usec -= starttv.tv_usec;
	normalize_tv(&endtv);
	secs = (double)endtv.tv_sec + (double)endtv.tv_usec / 1000000.0;
	printf("Sent %u messages of size %u bytes in %.04g seconds.\n",
			count, (unsigned)opts->msglen, secs);
	print_bw(stdout, &endtv, count, bytes_sent);

	/* Stop rescheduling the stats timer */
	timer_control.stop_timer = 1;

	if (lbm_ssrc_buff_put(ssrc, message) == LBM_FAILURE) {
		fprintf(stderr, "lbm_ssrc_buff_put: %s\n", lbm_errmsg());
		exit(1);
	}

	/*
	 * Sleep for a bit so that batching gets out all the queued messages,
	 * if any.  If we just exit, then some messages may not have been sent by
	 * TCP yet.
	 */
	if(opts->stats_sec > 0 && opts->stats_sec > opts->linger) {
		printf("Delaying to catch last stats timer... \n");
		SLEEP_SEC((opts->stats_sec - opts->linger) + 1);
	}
	else
		print_stats(stdout, ssrc);
	if (opts->linger > 0) {
		printf("Lingering for %d seconds...\n", opts->linger);
		SLEEP_SEC(opts->linger);
	}

	/* If the user requested monitoring, unregister the monitors etc  */
	if (opts->monitor_context || opts->monitor_source) {
		if (opts->monitor_context) {
			if (lbmmon_context_unmonitor(monctl, ctx) == -1) {
				fprintf(stderr, "lbmmon_context_unmonitor() failed\n");
				exit(1);
			}
		} else {
			if (lbmmon_ssrc_unmonitor(monctl, ssrc) == -1) {
				fprintf(stderr, "lbmmon_ssrc_unmonitor() failed\n");
				exit(1);
			}
		}
		if (lbmmon_sctl_destroy(monctl) == -1) {
			fprintf(stderr, "lbmmon_sctl_destoy() failed()\n");
			exit(1);
		}
	}

	if (opts->user_supplied_buffer && (opts->msglen > 0)) {
		free(user_supplied_buffer);
	}

	/* Deallocate smart source and LBM context */
	printf("Deleting smart source\n");
	lbm_ssrc_delete(ssrc);
	ssrc = NULL;

	printf("Deleting context\n");
	lbm_context_delete(ctx);
	ctx = NULL;

	return 0;
}

