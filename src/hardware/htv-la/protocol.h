/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2022 MasterQ <masterq@mutemail.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBSIGROK_HARDWARE_HTV_LA_PROTOCOL_H
#define LIBSIGROK_HARDWARE_HTV_LA_PROTOCOL_H

#include <stdint.h>
#include <glib.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"

#define LOG_PREFIX "htv-la"

#define HTV_LA_SET_SAMPLE_RATE 0xC1
#define HTV_LA_ACQUISITION_START 0xC5
#define HTV_LA_ACQUISITION_STOP 0xC6

struct dev_context {
};

enum dslogic_edge_modes {
	DS_EDGE_RISING,
	DS_EDGE_FALLING,
};

SR_PRIV int htv_la_receive_data(int fd, int revents, void *cb_data);

void htv_la_open_dev(void);

typedef struct htv_la_settings_t
{
	uint64_t sample_rate;
	uint64_t capture_ratio;
} htv_la_settings_t;

#endif
