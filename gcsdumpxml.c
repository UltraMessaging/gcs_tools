/* 
 gcsdumpxml.c : Code that illustrates how to dump object XML configuration using the C-API 

  Copyright (c) 2005-2022 Informatica Corporation  Permission is granted to licensees to use
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <sys/timeb.h>
#endif
#include "replgetopt.h"
#include <lbm/lbm.h>

int lbm_log_msg(int level, const char *message, void *clientd) {
	/* Filter deprecated message warnings for things like OTR and retransmit_request_generation_interval */
        if (strncmp (message, "Core-5688-27: WARNING: receiver config variable otr_request_duration is deprecated. Use otr_request_message_timeout instead", 14) == 0) {
#if DEBUG
        	printf("LOG Level %d: %s\n", level, message);
#endif
        } else {
        	printf("LOG Level %d: %s\n", level, message);
	}

        return 0;
}


void dump_utility(lbm_config_option_t * configs_option_ptr, int entries)
{
  int idx;
  printf("\n ------Start ------ \n");
  for(idx = 0; idx < entries; ++idx) {
      printf("%s %s %s\n",  configs_option_ptr[idx].type, configs_option_ptr[idx].oname, configs_option_ptr[idx].val);
  }
  printf(" ------ End ------ \n");
  free(configs_option_ptr);
}

void Dump_context_attr_xml(const char *xml_context_name)
{
  lbm_config_option_t *configs_option_ptr;
  int entries;
  lbm_context_attr_t *ctx_attr_ptr;

  if( LBM_FAILURE == lbm_context_attr_create_from_xml (&ctx_attr_ptr, xml_context_name) )
  {
    fprintf(stderr, "lbm_context_attr_create_from_xml error: %s\n", lbm_errmsg() );
    return;
  }

  entries = lbm_context_attr_option_size();
  configs_option_ptr = malloc(entries * sizeof(lbm_config_option_t));
  if (LBM_FAILURE == lbm_context_attr_dump(ctx_attr_ptr, &entries, configs_option_ptr))
  {
    fprintf(stderr, "lbm_context_attr_dump error: %s\n", lbm_errmsg() );
    return;
  }

  printf("\n Dumping context attributes \n");
  dump_utility(configs_option_ptr, entries);
}

void Dump_src_attr_xml(const char *xml_context_name, const char *topicname)
{
  lbm_config_option_t *configs_option_ptr;
  int entries;
  lbm_src_topic_attr_t *src_attr_ptr;

  if( LBM_FAILURE == lbm_src_topic_attr_create_from_xml (&src_attr_ptr, xml_context_name, topicname) )
  {
    fprintf(stderr, "lbm_src_topic_attr_create_from_xml error: %s\n", lbm_errmsg() );
    return;
  }

  entries = lbm_src_topic_attr_option_size();
  configs_option_ptr = malloc(entries * sizeof(lbm_config_option_t));

  if (LBM_FAILURE == lbm_src_topic_attr_dump(src_attr_ptr, &entries, configs_option_ptr))
  {
    fprintf(stderr, "lbm_src_topic_dump error: %s\n", lbm_errmsg() );
    return;
  }

  dump_utility(configs_option_ptr, entries);
}

void Dump_rcv_attr_xml(const char *xml_context_name, const char *topicname)
{
  lbm_config_option_t *configs_option_ptr;
  int entries;
  lbm_rcv_topic_attr_t *rcv_attr_ptr;

  if( LBM_FAILURE == lbm_rcv_topic_attr_create_from_xml (&rcv_attr_ptr, xml_context_name, topicname) )
  {
    fprintf(stderr, "lbm_rcv_topic_attr_create_from_xml error: %s\n", lbm_errmsg() );
    return;
  }

  entries = lbm_rcv_topic_attr_option_size();
  configs_option_ptr = malloc(entries * sizeof(lbm_config_option_t));

  if (LBM_FAILURE == lbm_rcv_topic_attr_dump(rcv_attr_ptr, &entries, configs_option_ptr))
  {
    fprintf(stderr, "lbm_rcv_topic_attr_dump error: %s\n", lbm_errmsg() );
    return;
  }
  dump_utility(configs_option_ptr, entries);
}

void Dump_evq_attr_xml(const char *event_queue_name)
{
  lbm_config_option_t *configs_option_ptr;
  int entries;
  lbm_event_queue_attr_t *evq_attr_ptr;

  if( LBM_FAILURE == lbm_event_queue_attr_create_from_xml (&evq_attr_ptr, event_queue_name) )
  {
    fprintf(stderr, "lbm_event_queue_attr_create_from_xml error: %s\n", lbm_errmsg() );
    return;
  }
  entries = lbm_event_queue_attr_option_size();
  configs_option_ptr = malloc(entries * sizeof(lbm_config_option_t));

  if (LBM_FAILURE == lbm_event_queue_attr_dump(evq_attr_ptr, &entries, configs_option_ptr))
  {
    fprintf(stderr, "lbm_event_queue_attr_dump error: %s\n", lbm_errmsg() );
    return;
  }
  dump_utility(configs_option_ptr, entries);
}


void Dump_wrcv_attr_xml(const char* xml_context_name, const char *  pattern, int pattern_type)
{
  lbm_config_option_t *configs_option_ptr;
  int entries;
  lbm_wildcard_rcv_attr_t *wattr_ptr;
  

  if( LBM_FAILURE ==  lbm_wildcard_rcv_attr_create_from_xml (&wattr_ptr, xml_context_name, pattern, pattern_type ) )
  {
    fprintf(stderr, "lbm_wildcard_rcv_attr_create_from_xml error: %s\n", lbm_errmsg() );
    return;
  }
  
  entries = lbm_wildcard_rcv_attr_option_size();
  configs_option_ptr = malloc(entries * sizeof(lbm_config_option_t));
  
  if( LBM_FAILURE == lbm_wildcard_rcv_attr_dump(wattr_ptr, &entries, configs_option_ptr))
  {
    fprintf(stderr, "lbm_wildcard_rcv_attr_dump error: %s\n", lbm_errmsg() );
    return;
  }
  dump_utility(configs_option_ptr, entries);
  lbm_wildcard_rcv_attr_delete(wattr_ptr);
}


void Dump_hfx_attr_xml(const char * my_hf_topic){
  lbm_config_option_t *configs_option_ptr;
  int entries;
  lbm_hfx_attr_t *hfx_attr_ptr;
  
  if( LBM_FAILURE == lbm_hfx_attr_create_from_xml(&hfx_attr_ptr, my_hf_topic) )
  {
    fprintf(stderr, "lbm_hfx_attr_create error: %s\n", lbm_errmsg() );
    return;
  }

  entries = lbm_hfx_attr_option_size();
  configs_option_ptr = malloc(entries * sizeof(lbm_config_option_t));

  if( LBM_FAILURE == lbm_hfx_attr_dump(hfx_attr_ptr, &entries, configs_option_ptr))
  {
    fprintf(stderr, "lbm_hfx_attr_dump error: %s\n", lbm_errmsg() );
    return;
  }
  dump_utility(configs_option_ptr, entries);
}

 

int rcv_callback(lbm_rcv_t *rcv, lbm_msg_t *msg, void *clientd){
 printf("\n Receiver callback placeholder");
 return 0;
}


const char usage[] =
"Usage:  [-a xml_appname] [-c context_name] [-s source_topic] [-r receive_topic] [-e evq_name] [-w wildward_topic] [-h hfx_topic] [-x xml_config] [-H] \n"
"Available options:\n"
"  -a, --xml_appname        specify the application name \n"
"  -c, --xml_context_name   specify the context name \n"
"  -s, --xml_src_topic      specify the source topic \n"
"  -r, --xml_rcv_topic      specify the receiver topic \n"
"  -e, --xml_evq_name       specify the event_queue name \n"
"  -w, --xml_wrcv_pattern   specify the wildcard receiver pattern \n"
"  -h, --xml_hfx_topic      specify the hfx name \n"
"  -x, --xml_config=FILE    Use LBM configuration XML FILE.\n"
"  -H, --help               display  help information \n"
;


const struct option OptionTable[] = {
        { "xml_appname", required_argument, NULL, 'a' },
        { "xml_context_name", required_argument, NULL, 'c' },
        { "xml_src_topic", required_argument, NULL, 's' },
        { "xml_rcv_topic", required_argument, NULL, 'r' },
        { "xml_evq_name", required_argument, NULL, 'e' },
        { "xml_hfx_topic", required_argument, NULL, 'h' },
        { "help", no_argument, NULL, 'H' },
        { "xml_wrcv_pattern", required_argument, NULL, 'w' },
        { "xml_config", required_argument, NULL, 'x' },
        { NULL, 0, NULL, 0 }
};

/* Max name size (arbitrary) */
#define MAX_NAME_SIZE 1000

struct Options {
        char xml_context_name[MAX_NAME_SIZE];   
        char xml_src_topic[MAX_NAME_SIZE];      
        char xml_rcv_topic[MAX_NAME_SIZE];     
        char xml_evq_name[MAX_NAME_SIZE];    
        char xml_wrcv_pattern[MAX_NAME_SIZE];  
        char xml_hfx_topic[MAX_NAME_SIZE];  
        char xml_config[MAX_NAME_SIZE];   
        char xml_appname[MAX_NAME_SIZE];   
} options;

const char * OptionString = "a:c:s:r:e:h:Hw:x:";

void process_cmdline(int argc, char **argv,struct Options *opts)
{
        int c,errflag = 0;


        /* Set default option values */
        memset(opts, 0, sizeof(*opts));

	while ((c = getopt_long(argc, argv, OptionString, OptionTable, NULL)) != EOF) {
                switch (c) {
                case 'a':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_appname, optarg, sizeof(opts->xml_appname));
                        }
                        break;
                case 'c':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_context_name, optarg, sizeof(opts->xml_context_name));
                        }

                        break;
                case 's':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_src_topic, optarg, sizeof(opts->xml_src_topic));
                        }

                        break;
                case 'r':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_rcv_topic, optarg, sizeof(opts->xml_rcv_topic));
                        }

                        break;
                case 'e':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_evq_name, optarg, sizeof(opts->xml_evq_name));
                        }

                        break;
                case 'h':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_hfx_topic, optarg, sizeof(opts->xml_hfx_topic));
                        }

                        break;
                case 'H':
                	fprintf(stderr, "%s\n%s\n%s", argv[0], lbm_version(), usage);
			exit(0);
                        break;
                case 'w':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_wrcv_pattern, optarg, sizeof(opts->xml_wrcv_pattern));
                        }

                        break;
                case 'x':
                        if (optarg != NULL)
                        {
                                strncpy(opts->xml_config, optarg, sizeof(opts->xml_wrcv_pattern));
                        }
                        break;
                default:
                        errflag++;
                	fprintf(stderr, " ERROR! \n%s\n%s", lbm_version(), usage);
			exit(1);
                        break;
                }

	}

}



int main(int argc, char **argv)
{
  struct Options *opts = &options;
  char * xml_config_env_check = NULL;
  int pattern_type=LBM_WILDCARD_RCV_PATTERN_TYPE_PCRE;
  char * xml_context_name;   
  char * xml_src_topic;
  char * xml_rcv_topic;
  char * xml_evq_name;
  char * xml_wrcv_pattern;
  char * xml_hfx_topic;
  char * xml_config;
  char * xml_appname;

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
#endif
  
  /* Print help if no arguments provided */
  if (argc == 1) {
       fprintf(stderr, "%s\n%s\n%s", argv[0], lbm_version(), usage);
       exit(1);
  }

  /* Exit if env is set to pre-load an XML file */
  if ((xml_config_env_check = getenv("LBM_XML_CONFIG_FILENAME")) != NULL) {
	fprintf(stderr, "\n ERROR!: Please unset LBM_XML_CONFIG_FILENAME so that an XML file: can be loaded \n" );
	exit(1);
  }
  if ((xml_config_env_check = getenv("LBM_UMM_INFO")) != NULL) {
	fprintf(stderr, "\n ERROR!: Please unset LBM_UMM_INFO so that an XML file: can be loaded \n" );
	exit(1);
  }
  
  /* Initialize logging callback */
  if (lbm_log(lbm_log_msg, NULL) == LBM_FAILURE) {
  	fprintf(stderr, "lbm_log: %s\n", lbm_errmsg());
	exit(1);
  }

  /* Process command line options */
  process_cmdline(argc, argv, opts);

  xml_context_name=opts->xml_context_name ? opts->xml_context_name : "";   
  xml_src_topic=opts->xml_src_topic ? opts->xml_src_topic : "";
  xml_rcv_topic=opts->xml_rcv_topic ? opts->xml_rcv_topic : "";
  xml_evq_name=opts->xml_evq_name ? opts->xml_evq_name : "";
  xml_wrcv_pattern=opts->xml_wrcv_pattern ? opts->xml_wrcv_pattern : "";
  xml_hfx_topic=opts->xml_hfx_topic ? opts->xml_hfx_topic : "";
  xml_config=opts->xml_config ? opts->xml_config : "";
  xml_appname=opts->xml_appname ? opts->xml_appname : "";

  /* Initialize configuration parameters from a file. */
  if (lbm_config_xml_file(xml_config, xml_appname ) == LBM_FAILURE) {
  	fprintf(stderr, "Couldn't load lbm_config_xml_file: appname: %s xml_config: %s : Error: %s\n", xml_appname, xml_config, lbm_errmsg());
  	exit(1);
  }

  printf("\n xml_context_name: %s, xml_src_topic: %s, xml_rcv_topic: %s, xml_evq_name: %s, xml_wrcv_pattern: %s, xml_hfx_topic: %s, pattern_type %i )\n ", xml_context_name, xml_src_topic, xml_rcv_topic, xml_evq_name, xml_wrcv_pattern, xml_hfx_topic, pattern_type ); 

  Dump_context_attr_xml(xml_context_name);
  Dump_src_attr_xml(xml_context_name,xml_src_topic);
  Dump_rcv_attr_xml(xml_context_name,xml_rcv_topic);
  Dump_evq_attr_xml(xml_evq_name);
  Dump_wrcv_attr_xml(xml_context_name, xml_wrcv_pattern, pattern_type);
  Dump_hfx_attr_xml(xml_hfx_topic);

  return 0;
}
