/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "mqtt.h"
#include "rpl.h"
#include "rpl-neighbor.h"
#include "contiki-net.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOG_LEVEL LOG_LEVEL_INFO

// RPL DEFINES
#define MAXIMUM_NODE_ID_SIZE 120
#define BUFFER_SIZE MAXIMUM_NODE_ID_SIZE

// MQTT DEFINES
// MQTT CONNECTION
#define MAX_TCP_SEGMENT_SIZE 32
#define BROKER_IP "fe80::201:1:1:1"
#define BROKER_PORT 1883
#define PUBLISH_INTERVAL    (30 * CLOCK_SECOND)
#define CONNECTION_WAIT (CLOCK_SECOND >> 1)
#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
#define NET_CONNECT_PERIODIC (CLOCK_SECOND >> 2)
#define RECONNECT_INTERVAL (CLOCK_SECOND * 2)
#define RETRY_FOREVER              0xFF
#define RECONNECT_ATTEMPTS RETRY_FOREVER
#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)

// MQTT STATES
#define STATE_INIT 0
#define STATE_REGISTERED 1
#define STATE_CONNECTING 2
#define STATE_CONNECTED 3
#define STATE_PUBLISHING 4
#define STATE_DISCONNECTED 5
# define STATE_ERROR 6

// MQTT TOPICS
#define NOTIFICATIONS "/notifications"
#define EVENTS "/events"
#define CONTACTS "/contacts"
#define RANDOM_EVENT_PROBABILITY 0.5
#define MAX_TOPIC_SUFFIX_LEN 14

// RPL STATICS
static int last_table_row= -1;

// MQTT STATICS
// MQTT CONNECTION
static struct mqtt_connection conn;
static char client_id[BUFFER_SIZE];
static uint8_t connect_attempt;
static struct etimer publish_periodic_timer;
static struct timer connection_life;
static struct mqtt_message *msg_ptr = 0;

// MQTT STATE MACHINE
static uint8_t state;

// MQTT TOPICS
static char notifications[BUFFER_SIZE + MAX_TOPIC_SUFFIX_LEN];
static char events[BUFFER_SIZE + MAX_TOPIC_SUFFIX_LEN];
static char contacts[BUFFER_SIZE + MAX_TOPIC_SUFFIX_LEN];


/*---------------------------------------------------------------------------*/
PROCESS(contact_tracing_process, "Hello world process");

static char*
get_nbrs(){
    if(curr_instance.used) {
        rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);
        char* nbr_ips = (char*) malloc(sizeof(char)*2);
        snprintf(nbr_ips, sizeof("["), "%s", "[");

        int curr_rows = -1;
        while(nbr != NULL) {
            if(curr_rows < last_table_row){
                nbr = nbr_table_next(rpl_neighbors, nbr);
                curr_rows++;
                continue;
            }
            char nbr_ipaddr[MAXIMUM_NODE_ID_SIZE];
            int ip_len;
            ip_len = uiplib_ipaddr_snprint(nbr_ipaddr, sizeof(nbr_ipaddr), rpl_neighbor_get_ipaddr(nbr));
            if (ip_len <= 0 || ip_len > MAXIMUM_NODE_ID_SIZE){
                printf("IP_LEN either < 0 or too large, Failed at line 68 in contact_tracing_node.c: ip_len = %d", ip_len);
                return NULL;
            }
            printf("%s\n", nbr_ipaddr);
            nbr_ips = (char*) realloc((void*) nbr_ips, sizeof(nbr_ips) + sizeof(char)*(MAXIMUM_NODE_ID_SIZE + 1));
            nbr_ips = strcat(nbr_ips, nbr_ipaddr);
            nbr_ips = strcat(nbr_ips, "-");
            nbr = nbr_table_next(rpl_neighbors, nbr);
            curr_rows++;
            last_table_row++;
        }
        nbr_ips = strcat(nbr_ips, "]");
        return nbr_ips;
    }
    return NULL;
}


//MQTT CALLBACK

static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
    switch(event){
        case MQTT_EVENT_CONNECTED: {
            printf("MQTT Connected\n");
            timer_set(&connection_life, CONNECTION_STABLE_TIME);
            state = STATE_CONNECTED;
            break;
        }
        case MQTT_EVENT_PUBLISH: {
            msg_ptr = data;
            printf("Application received a publish on topic '%s'; payload '%s' size is %i bytes\n",
                   msg_ptr->topic, msg_ptr->payload_chunk, msg_ptr->payload_length);
            //TODO add to the printf also leds blinking
            //TODO take them from mqtt-demo state_machine()
            if(msg_ptr->first_chunk)
                msg_ptr->first_chunk = 0;

            break;
        }
        case MQTT_EVENT_SUBACK: {
            printf("Application subscribed to topic");
            break;
        }

        case MQTT_EVENT_UNSUBACK: {
            printf("Application is unsubscribed to topic successfully\n");
            break;
        }
        case MQTT_EVENT_PUBACK: {
            printf("Publishing complete\n");
            break;
        }
        default:
            printf("Application got a unhandled MQTT event: %i\n", event);
            break;
    }
}

void
build_topics(){
    char client_topic[BUFFER_SIZE + 1];
    snprintf(client_topic, sizeof(client_topic), "%s", "/");
    strcat(client_topic, client_id);

    snprintf(notifications, sizeof(notifications), "%s", client_topic);
    strcat(notifications, NOTIFICATIONS);

    snprintf(events, sizeof(events), "%s", client_topic);
    strcat(events, EVENTS);

    snprintf(contacts, sizeof(contacts), "%s", client_topic);
    strcat(contacts, CONTACTS);
}

static void
mqtt_state_machine(void)
{
    switch(state){
        case STATE_INIT:
            printf("MQTT init \n");
            uiplib_ipaddr_snprint(client_id, sizeof(client_id), rpl_get_global_address());
            build_topics();

            mqtt_register(&conn, &contact_tracing_process, client_id, mqtt_event, MAX_TCP_SEGMENT_SIZE);

            conn.auto_reconnect = 0;
            connect_attempt = 1;

            state = STATE_REGISTERED;
            // Continue
        case STATE_REGISTERED:
            if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
                mqtt_connect(&conn, BROKER_IP, BROKER_PORT, PUBLISH_INTERVAL);
                state = STATE_CONNECTING;
            } else{
                printf("Cannot get global ipv6 address\n");
            }
            etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
            return;
        case STATE_CONNECTING:
            printf("Connecting\n");
            break;
        case STATE_CONNECTED:
            // Continue
        case STATE_PUBLISHING:
            if(timer_expired(&connection_life)) {
                connect_attempt = 0;
            }
            if(mqtt_ready(&conn) && conn.out_buffer_sent) {
                /* Connected; publish */
                if (state == STATE_CONNECTED) {
                    mqtt_status_t status;
                    status = mqtt_subscribe(&conn, NULL, notifications, MQTT_QOS_LEVEL_0);
                    printf("Subscribing");
                    if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
                        printf("Tried to subscribe but command queue was full!\n");
                    }
                    state = STATE_PUBLISHING;
                } else {
                    printf("Publishing\n");
                    char* nbrs = get_nbrs();
                    mqtt_publish(&conn, NULL, contacts, (uint8_t*) nbrs , sizeof(nbrs), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
                    if((double)rand() / (double)RAND_MAX > RANDOM_EVENT_PROBABILITY){
                        // send an important event randomly with 0.5 probability
                        char* to_publish = "Important Event";
                        mqtt_publish(&conn, NULL, events, (uint8_t*) to_publish , sizeof(char) * strlen(to_publish), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
                    }
                }
                etimer_set(&publish_periodic_timer, PUBLISH_INTERVAL);
                return;
            }else{
                // Lost connectivity or some message not fully ACKd
                printf("Publishing... (MQTT state=%d, q=%u)\n", conn.state,
                         conn.out_queue_full);
            }
            break;
        case STATE_DISCONNECTED:
            printf("Disconnected\n");
            if (connect_attempt < RECONNECT_ATTEMPTS || RECONNECT_ATTEMPTS == RETRY_FOREVER){

                clock_time_t interval;
                mqtt_disconnect(&conn);
                connect_attempt++;

                // increasing reconnection interval
                interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt :
                           RECONNECT_INTERVAL << 3;

                printf("Disconnected: attempt %u in %lu ticks\n", connect_attempt, interval);

                etimer_set(&publish_periodic_timer, interval);
                state = STATE_REGISTERED;
                return;
            } else{
                state = STATE_ERROR;
                printf("Aborting connection after %u attempts\n", connect_attempt - 1);
            }
            break;
        case STATE_ERROR:
            // Continue
        default:
            printf("Error, sinked in Default State\n");
            return;
    }

    etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);
}
AUTOSTART_PROCESSES(&contact_tracing_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contact_tracing_process, ev, data)
{

  PROCESS_BEGIN();

  state = STATE_INIT;


  while(1) {

      PROCESS_YIELD();
      if (ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) {
          mqtt_state_machine();
      }
  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
