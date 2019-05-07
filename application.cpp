#include <stdint.h>
#include <string.h>

#include <Wire.h>

#include "raat.hpp"
#include "raat-buffer.hpp"

#include "raat-oneshot-timer.hpp"
#include "raat-oneshot-task.hpp"
#include "raat-task.hpp"

#include "http-get-server.hpp"

static HTTPGetServer s_server(true);
static raat_devices_struct * s_pDevices = NULL;

static void send_standard_erm_response()
{
    s_server.set_response_code_P(PSTR("200 OK"));
    s_server.set_header_P(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    s_server.finish_headers();
}

static void get_part1(char const * const url) {
    (void)url;
    s_server.add_body_P(s_pDevices->pPart1Input->state() ? PSTR("COMPLETE") : PSTR("NOT COMPLETE"));
    send_standard_erm_response();
}

static void get_part2(char const * const url) {
    (void)url;
    s_server.add_body_P(s_pDevices->pPart2Input->state() ? PSTR("COMPLETE") : PSTR("NOT COMPLETE"));
    send_standard_erm_response();
}


static void trigger_part1(char const * const url) {
    (void)url;
    s_pDevices->pPart1Output->set(true);
    send_standard_erm_response();
}

static void trigger_part2(char const * const url) {
    (void)url;
    s_pDevices->pPart2Output->set(true);
    send_standard_erm_response();
}


static void reset_part1(char const * const url) {
    (void)url;
    s_pDevices->pPart1Output->set(false);
    send_standard_erm_response();
}

static void reset_part2(char const * const url) {
    (void)url;
    s_pDevices->pPart2Output->set(false);
    send_standard_erm_response();
}

static const char PART1_GET_URL[] PROGMEM = "/part1/status";
static const char PART2_GET_URL[] PROGMEM = "/part2/status";
static const char PART1_TRIGGER_URL[] PROGMEM = "/part1/trigger";
static const char PART2_TRIGGER_URL[] PROGMEM = "/part2/trigger";
static const char PART1_RESET_URL[] PROGMEM = "/part1/reset";
static const char PART2_RESET_URL[] PROGMEM = "/part2/reset";

static http_get_handler s_handlers[] = 
{
    {PART1_GET_URL, get_part1},
    {PART2_GET_URL, get_part2},
    {PART1_TRIGGER_URL, trigger_part1},
    {PART2_TRIGGER_URL, trigger_part2},
    {PART1_RESET_URL, reset_part1},
    {PART2_RESET_URL, reset_part2},
    {"", NULL}
};

void ethernet_packet_handler(char * req)
{
    s_server.handle_req(s_handlers, req);
}

char * ethernet_response_provider()
{
    return s_server.get_response();
}

void raat_custom_setup(const raat_devices_struct& devices, const raat_params_struct& params)
{
    (void)params;

    s_pDevices = &devices;
}

void raat_custom_loop(const raat_devices_struct& devices, const raat_params_struct& params)
{
    (void)devices; (void)params;
}