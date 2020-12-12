#include <stdint.h>
#include <string.h>

#include <Wire.h>

#include "raat.hpp"
#include "raat-buffer.hpp"

#include "raat-oneshot-timer.hpp"
#include "raat-oneshot-task.hpp"
#include "raat-task.hpp"

#include "http-get-server.hpp"

static const raat_devices_struct * s_pDevices = NULL;
static bool all_inputs_on = false;

static void send_standard_erm_response(HTTPGetServer& server)
{
    server.set_response_code_P(PSTR("200 OK"));
    server.set_header_P(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    server.finish_headers();
}

static void get_part(HTTPGetServer& server, uint8_t part)
{
    send_standard_erm_response(server);
    server.add_body_P(s_pDevices->pInputs[part-1]->state() ? PSTR("COMPLETE\r\n\r\n") : PSTR("NOT COMPLETE\r\n\r\n"));
}

static void get_part(HTTPGetServer& server, char const * const url, char const * const additional)
{
    (void)url;
    uint8_t input_number = 255;

    if (raat_parse_single_numeric(additional, input_number, NULL) && inrange(input_number, (uint8_t)1, (uint8_t)4))
    {
        get_part(server, input_number);
    }
}

static void trigger_part(HTTPGetServer& server, uint8_t part)
{
    s_pDevices->pOutputs[part-1]->set(true);
    send_standard_erm_response(server);
    server.add_body_P(PSTR("OK\r\n\r\n"));
}

static void trigger_part(HTTPGetServer& server, char const * const url, char const * const additional)
{
    (void)url;
    uint8_t output_number = 255;

    if (raat_parse_single_numeric(additional, output_number, NULL) && inrange(output_number, (uint8_t)1, (uint8_t)5))
    {
        trigger_part(server, output_number);
    }
}

static void reset_part(HTTPGetServer& server, uint8_t part)
{
    s_pDevices->pOutputs[part-1]->tristate(false);
    send_standard_erm_response(server);
    server.add_body_P(PSTR("OK\r\n\r\n"));
}

static void reset_part(HTTPGetServer& server, char const * const url, char const * const additional)
{
    (void)url;
    uint8_t output_number = 255;

    if (raat_parse_single_numeric(additional, output_number, NULL) && inrange(output_number, (uint8_t)1, (uint8_t)5))
    {
        reset_part(server, output_number);
    }
}

static const char PART_GET_URL[] PROGMEM = "/part/status";
static const char PART_TRIGGER_URL[] PROGMEM = "/part/trigger";
static const char PART_RESET_URL[] PROGMEM = "/part/reset";

static http_get_handler s_handlers[] = 
{
    {PART_GET_URL, get_part},
    {PART_TRIGGER_URL, trigger_part},
    {PART_RESET_URL, reset_part},
    {"", NULL}
};
static HTTPGetServer s_server(NULL)

;

void ethernet_packet_handler(char * req)
{
    s_server.handle_req(s_handlers, req);
}

char * ethernet_response_provider()
{
    return s_server.get_response();
}

static void status_task_fn(RAATTask& task, void * pTaskData)
{
    (void)task; (void)pTaskData;
    for (uint8_t i = 0; i<s_pDevices->InputsCount; i++)
    {
        raat_logln_P(LOG_APP, PSTR("Part %u: %S%S"),
            i,
            s_pDevices->pInputs[i]->state() ? PSTR("1") : PSTR("0"),
            s_pDevices->pOutputs[i]->is_tristated() ? PSTR("") : PSTR(",OVR")
        );
    }
}
static RAATTask s_status_task(1000, status_task_fn);

static void input_scan_task_fn(RAATTask& task, void * pTaskData)
{
    (void)task; (void)pTaskData;
    
    all_inputs_on = true;

    for (uint8_t i = 0; i<s_pDevices->InputsCount; i++)
    {
        all_inputs_on &= s_pDevices->pInputs[i]->state();
    }

    if (all_inputs_on)
    {
        s_pDevices->pOutputs[4]->set(true);
    }
}
static RAATTask s_input_scan_task(50, input_scan_task_fn);

void raat_custom_setup(const raat_devices_struct& devices, const raat_params_struct& params)
{
    (void)params;
    s_pDevices = &devices;

    for (uint8_t i = 0; i<s_pDevices->OutputsCount; i++)
    {
        s_pDevices->pOutputs[i]->tristate(false);
    }  
}

void raat_custom_loop(const raat_devices_struct& devices, const raat_params_struct& params)
{
    (void)devices; (void)params;
    s_status_task.run();
    s_input_scan_task.run();
}
