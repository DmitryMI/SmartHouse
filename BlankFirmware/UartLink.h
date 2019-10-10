#pragma once

#include <stdlib.h>

#include "DeviceConfig.h"

typedef void (*received_callback) (char ch);
typedef void (*request_t) ();

void ULink_init();

void ULink_send_command(char* message);

void ULink_send_info(char* message);

void ULink_set_reset_request_handler(request_t handler);

void ULink_set_prog_request_handler(request_t handler);

char ULink_receive();