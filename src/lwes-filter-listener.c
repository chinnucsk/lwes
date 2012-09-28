/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
 *                                                                      *
 * Licensed under the New BSD License (the "License"); you may not use  *
 * this file except in compliance with the License.  Unless required    *
 * by applicable law or agreed to in writing, software distributed      *
 * under the License is distributed on an "AS IS" BASIS, WITHOUT        *
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.     *
 * See the License for the specific language governing permissions and  *
 * limitations under the License. See accompanying LICENSE file.        *
 *======================================================================*/

#define _GNU_SOURCE
#include "lwes_listener.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

/* prototypes */
static void signal_handler(int sig);

/* global variable used to indicate what signal (if any) has been caught */
static volatile int done = 0;

static const char help[] =
  "lwes-filter-listener [options]"                                     "\n"
  ""                                                                   "\n"
  "  where options are:"                                               "\n"
  ""                                                                   "\n"
  "    -m [one argument]"                                              "\n"
  "       The multicast ip address to listen on."                      "\n"
  "       (default: 224.1.1.11)"                                       "\n"
  ""                                                                   "\n"
  "    -p [one argument]"                                              "\n"
  "       The ip port to listen on."                                   "\n"
  "       (default: 12345)"                                            "\n"
  ""                                                                   "\n"
  "    -i [one argument]"                                              "\n"
  "       The interface to listen on."                                 "\n"
  "       (default: 0.0.0.0)"                                          "\n"
  ""                                                                   "\n"
  "    -e [comma separated list]"                                      "\n"
  "       The list of events to print out."                            "\n"
  ""                                                                   "\n"
  "    -a [comma separated k=v pairs]"                                 "\n"
  "       Key=value pairs to check before printing the event."         "\n"
  ""                                                                   "\n"
  "    -h"                                                             "\n"
  "         show this message"                                         "\n"
  ""                                                                   "\n"
  "  arguments are specified as -option value or -optionvalue"         "\n"
  ""                                                                   "\n";

static int
lwes_U_INT_16_to_stream
  (LWES_U_INT_16 a_uint16,
   FILE *stream)
{
  return fprintf (stream,"%hu",a_uint16);
}

static int
lwes_INT_16_to_stream
  (LWES_INT_16 an_int16,
   FILE *stream)
{
  return fprintf (stream,"%hd",an_int16);
}

static int
lwes_U_INT_32_to_stream
  (LWES_U_INT_32 a_uint32,
   FILE *stream)
{
  return fprintf (stream,"%u",a_uint32);
}

static int
lwes_INT_32_to_stream
  (LWES_INT_32 an_int32,
   FILE *stream)
{
  return fprintf (stream,"%d",an_int32);
}

static int
lwes_U_INT_64_to_stream
  (LWES_U_INT_64 a_uint64,
   FILE *stream)
{
  return fprintf (stream,"%llu",a_uint64);
}

static int
lwes_INT_64_to_stream
  (LWES_INT_64 an_int64,
   FILE *stream)
{
  return fprintf (stream,"%lld",an_int64);
}

static int
lwes_BOOLEAN_to_stream
  (LWES_BOOLEAN a_boolean,
   FILE *stream)
{
  return fprintf (stream,"%s",((a_boolean==1)?"true":"false"));
}

static int
lwes_IP_ADDR_to_stream
  (LWES_IP_ADDR an_ipaddr,
   FILE *stream)
{
  return fprintf (stream,"%s",inet_ntoa (an_ipaddr));
}

static int
lwes_SHORT_STRING_to_stream
  (LWES_SHORT_STRING a_string,
   FILE *stream)
{
  return fprintf (stream,"%s",a_string);
}

static int
lwes_LONG_STRING_to_stream
  (LWES_LONG_STRING a_string,
   FILE *stream)
{
  return fprintf (stream,"%s",a_string);
}

static int
lwes_event_attribute_to_stream
  (struct lwes_event_attribute *attribute,
   FILE *stream)
{
  if (attribute->type == LWES_U_INT_16_TOKEN)
  {
    lwes_U_INT_16_to_stream (*((LWES_U_INT_16 *)attribute->value),stream);
  }
  else if (attribute->type == LWES_INT_16_TOKEN)
  {
    lwes_INT_16_to_stream (*((LWES_INT_16 *)attribute->value),stream);
  }
  else if (attribute->type == LWES_U_INT_32_TOKEN)
  {
    lwes_U_INT_32_to_stream (*((LWES_U_INT_32 *)attribute->value),stream);
  }
  else if (attribute->type == LWES_INT_32_TOKEN)
  {
    lwes_INT_32_to_stream (*((LWES_INT_32 *)attribute->value),stream);
  }
  else if (attribute->type == LWES_U_INT_64_TOKEN)
  {
    lwes_U_INT_64_to_stream (*((LWES_U_INT_64 *)attribute->value),stream);
  }
  else if (attribute->type == LWES_INT_64_TOKEN)
  {
    lwes_INT_64_to_stream (*((LWES_INT_64 *)attribute->value),stream);
  }
  else if (attribute->type == LWES_BOOLEAN_TOKEN)
  {
    lwes_BOOLEAN_to_stream (*((LWES_BOOLEAN *)attribute->value),stream);
  }
  else if (attribute->type == LWES_IP_ADDR_TOKEN)
  {
    lwes_IP_ADDR_to_stream (*((LWES_IP_ADDR *)attribute->value),stream);
  }
  else if (attribute->type == LWES_STRING_TOKEN)
  {
    lwes_LONG_STRING_to_stream ((LWES_LONG_STRING)attribute->value,stream);
  }
  else
  {
    /* should really do something here, but not sure what */
  }
  return 0;
}


static int
lwes_U_INT_16_to_string
  (char **strp,
   LWES_U_INT_16 a_uint16)
{
  return asprintf (strp, "%hu",a_uint16);
}

static int
lwes_INT_16_to_string
  (char **strp,
   LWES_INT_16 an_int16)
{
  return asprintf (strp, "%hd",an_int16);
}

static int
lwes_U_INT_32_to_string
  (char **strp,
   LWES_U_INT_32 a_uint32)
{
  return asprintf (strp, "%u",a_uint32);
}

static int
lwes_INT_32_to_string
  (char **strp,
   LWES_INT_32 an_int32)
{
  return asprintf (strp, "%d",an_int32);
}

static int
lwes_U_INT_64_to_string
  (char **strp,
   LWES_U_INT_64 a_uint64)
{
  return asprintf (strp, "%llu",a_uint64);
}

static int
lwes_INT_64_to_string
  (char **strp,
   LWES_INT_64 an_int64)
{
  return asprintf (strp, "%lld",an_int64);
}

static int
lwes_BOOLEAN_to_string
  (char **strp,
   LWES_BOOLEAN a_boolean)
{
  return asprintf (strp, "%s",((a_boolean==1)?"true":"false"));
}

static int
lwes_IP_ADDR_to_string
  (char **strp,
   LWES_IP_ADDR an_ipaddr)
{
  return asprintf (strp, "%s",inet_ntoa (an_ipaddr));
}

static int
lwes_LONG_STRING_to_string
  (char **strp,
   LWES_LONG_STRING a_string)
{
  return asprintf (strp, "%s",a_string);
}

static int
lwes_event_attribute_to_string
  (char **strp,
   struct lwes_event_attribute *attribute)
{
  if (attribute->type == LWES_U_INT_16_TOKEN)
  {
    return lwes_U_INT_16_to_string (strp, *((LWES_U_INT_16 *)attribute->value));
  }
  else if (attribute->type == LWES_INT_16_TOKEN)
  {
    return lwes_INT_16_to_string (strp, *((LWES_INT_16 *)attribute->value));
  }
  else if (attribute->type == LWES_U_INT_32_TOKEN)
  {
    return lwes_U_INT_32_to_string (strp, *((LWES_U_INT_32 *)attribute->value));
  }
  else if (attribute->type == LWES_INT_32_TOKEN)
  {
    return lwes_INT_32_to_string (strp, *((LWES_INT_32 *)attribute->value));
  }
  else if (attribute->type == LWES_U_INT_64_TOKEN)
  {
    return lwes_U_INT_64_to_string (strp, *((LWES_U_INT_64 *)attribute->value));
  }
  else if (attribute->type == LWES_INT_64_TOKEN)
  {
    return lwes_INT_64_to_string (strp, *((LWES_INT_64 *)attribute->value));
  }
  else if (attribute->type == LWES_BOOLEAN_TOKEN)
  {
    return lwes_BOOLEAN_to_string (strp, *((LWES_BOOLEAN *)attribute->value));
  }
  else if (attribute->type == LWES_IP_ADDR_TOKEN)
  {
    return lwes_IP_ADDR_to_string (strp, *((LWES_IP_ADDR *)attribute->value));
  }
  else if (attribute->type == LWES_STRING_TOKEN)
  {
    return lwes_LONG_STRING_to_string (strp, (LWES_LONG_STRING)attribute->value);
  }
  else
  {
    /* should really do something here, but not sure what */
    return -1;
  }
}


static int
lwes_event_to_stream
  (struct lwes_event *event,
   FILE *stream,
   const char **event_names,
   char **attr_list)
{
  struct lwes_event_attribute  *tmp;
  struct lwes_hash_enumeration  e;

  /* if event_names was provided, ensure that this event is in the list */
  if (event_names) {
    int match = 0;
    while (event_names && *event_names) {
      if (strcmp(event->eventName, *event_names) == 0) {
        match = 1;
        break;
      }
      ++event_names;
    }
    if (! match) {
      return 0;
    }
  }

  /* if attr_list was provided, ensure that the fields match */
  if (attr_list) {
    int match  = 1;
    
    while (*attr_list && match) {
      char* formatted;
      struct lwes_event_attribute *value =
        (struct lwes_event_attribute *) lwes_hash_get (event->attributes,
                                                       attr_list[0]);
      
      if (! value) {
        return 0; /* filter field was not set */
      }

      /* Format this field as a string, and compare to expectations. */
      int ret = lwes_event_attribute_to_string (&formatted, value);
      if (ret <= 0
	  || formatted == NULL) {
	fprintf (stderr, "Internal error while formatting a field (result=%d)!\n", ret);
	exit(1);
      } else {
	if (strcmp(attr_list[1], formatted) != 0) {
	  match = 0;
	}
	free (formatted);
      }

      attr_list += 2;
    }
    
    if (! match) {
      return 0;
    }
  }

  lwes_SHORT_STRING_to_stream (event->eventName,stream);
  fprintf (stream,"[");
  fflush (stream);
  lwes_U_INT_16_to_stream (event->number_of_attributes,stream);
  fprintf (stream,"]");
  fflush (stream);
  fprintf (stream," {");
  fflush (stream);

  if (lwes_hash_keys (event->attributes, &e)) {
      while (lwes_hash_enumeration_has_more_elements (&e)) {
          LWES_SHORT_STRING tmpAttrName =
            lwes_hash_enumeration_next_element (&e);

          tmp =
            (struct lwes_event_attribute *)lwes_hash_get (event->attributes,
                                                          tmpAttrName);

          lwes_SHORT_STRING_to_stream (tmpAttrName,stream);
          fflush (stream);
          fprintf (stream," = ");
          fflush (stream);
          lwes_event_attribute_to_stream (tmp,stream);
          fflush (stream);
          fprintf (stream,";");
          fflush (stream);
        }
    }
  fprintf (stream,"}\n");
  fflush (stream);
  return 0;
}

static int
count_fields(const char* string, char delim) {
  if (string && *string) {
    int fields = 1;
    while (*string) {
      if (delim == *string) {
        ++fields;
      }
      ++string;
    }
    return fields;
  } else {
    return 0;
  }
}

/* This modifies 'arg' in place but does not allocate any new strings.
 * It only allocates a NULL-terminated array referencing persistent
 * string fragments.  To release, just free() the result (if non-null).
 * The result points to a series of strings, terminated by NULL.
 * 
 * 'arg' should match /([^,]+(,[^,]+)*)?/
 */
static const char**
parse_comma_separated_list (char* arg) {
  const char** result = NULL;
  char *saveptr, *token;

  const int fields = count_fields(arg, ',');
  if (fields > 0) {
    int field = 0;
    result = calloc(fields+1, sizeof(const char*));
    while ((token = strtok_r(arg, ",", &saveptr))) {
      arg = NULL;
      result[field++] = token;
    }
    if (field == fields) {
      result[field] = NULL;
    } else {
      fprintf (stderr, "Internal error while parsing a comma-separated list!\n");
      exit(1);
    }
  }
  
  return result;
}

/* This modifies 'arg' in place but does not allocate any new strings.
 * It only allocates a NULL-terminated array referencing persistent
 * string fragments, which it returns.  To release, just free() the
 * result (if non-null).
 * 
 * The result points to a series of pairs of strings, terminated by NULL,
 * like key1,value1,key2,value2,NULL.
 * 
 * 'arg' should match /([^,]+(,[^,]+)*)?/
 */
static char**
parse_attr_list (char* arg) {
  int   num_fields;
  char *token, *saveptr1, *saveptr2;
  char **result = NULL;

  num_fields = count_fields (arg, ',');
  if (num_fields > 0) {
    int field = 0;
    result = calloc (2*num_fields+1, sizeof(char*));
    while ((token = strtok_r(arg, ",", &saveptr1))) {
      result[2*field] = strtok_r(token, "=", &saveptr2);
      if (! result[2*field]) {
        fprintf (stderr, "Internal error while parsing a key from a "
                 "comma-separated list of pairs!\n");
        exit(1);
      }
      result[2*field+1] = strtok_r(NULL, "=", &saveptr2);
      if (! result[2*field+1]) {
        fprintf (stderr, "Internal error while parsing a value from a "
                 "comma-separated list of pairs!\n");
        exit(1);
      }

      ++field;
      arg = NULL;
    }
  }

  return result;
}


int main (int   argc, char *argv[]) {

  const char  *mcast_ip    = "224.1.1.11";
  const char  *mcast_iface = NULL;
  int          mcast_port  = 12345;
  const char **event_names  = NULL;
  char       **attr_list   = NULL;

  sigset_t fullset;
  struct sigaction act;

  struct lwes_listener * listener;

  opterr = 0;
  while (1) {
    char c = getopt (argc, argv, "m:p:i:e:a:h");

    if (c == -1) {
      break;
    }

    switch (c) {
      case 'm':
        mcast_ip = optarg;
        break;

      case 'p':
        mcast_port = atoi(optarg);
        break;

      case 'h':
        fprintf (stderr, "%s", help);
        return 1;

      case 'i':
        mcast_iface = optarg;
        break;

      case 'e':
        event_names = parse_comma_separated_list (optarg);
        break;

      case 'a':
        attr_list = parse_attr_list (optarg);
        break;

      default:
        fprintf (stderr,
                 "error: unrecognized command line option -%c\n", 
                 optopt);
        return 1;
    }
  }

  sigfillset (&fullset);
  sigprocmask (SIG_SETMASK, &fullset, NULL);

  memset (&act, 0, sizeof (act));
  act.sa_handler = signal_handler;
  sigfillset (&act.sa_mask);

  sigaction (SIGINT, &act, NULL);
  sigaction (SIGTERM, &act, NULL);
  sigaction (SIGPIPE, &act, NULL);

  sigdelset (&fullset, SIGINT);
  sigdelset (&fullset, SIGTERM);
  sigdelset (&fullset, SIGPIPE);

  sigprocmask (SIG_SETMASK, &fullset, NULL);

  listener = lwes_listener_create (
      (LWES_SHORT_STRING) mcast_ip,
      (LWES_SHORT_STRING) mcast_iface,
      (LWES_U_INT_32)     mcast_port);

  while ( ! done ) {
    struct lwes_event *event = lwes_event_create_no_name ( NULL );

    if ( event != NULL ) {
      int ret = lwes_listener_recv ( listener, event);
      if ( ret > 0 ) {
        lwes_event_to_stream (event, stdout, event_names, attr_list);
      }
    }
    lwes_event_destroy (event);
  }

  lwes_listener_destroy (listener);

  return 0;
}

static void signal_handler(int sig) {
  (void)sig; /* appease compiler */
  done = 1;
}
