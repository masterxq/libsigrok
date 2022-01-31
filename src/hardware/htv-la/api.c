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

#include <config.h>
#include "protocol.h"
#include "stdio.h"

static struct sr_dev_driver htv_la_driver_info;


static const uint32_t scanopts[] = {
	SR_CONF_CONN,
};

static const uint32_t drvopts[] = {
	SR_CONF_LOGIC_ANALYZER,
};

static const uint32_t devopts[] = {
	SR_CONF_CONTINUOUS | SR_CONF_SET | SR_CONF_GET,
// 	SR_CONF_LIMIT_SAMPLES | SR_CONF_GET | SR_CONF_SET,
// 	SR_CONF_VOLTAGE_THRESHOLD | SR_CONF_LIST,
// 	SR_CONF_CONN | SR_CONF_GET,
	SR_CONF_SAMPLERATE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
// 	SR_CONF_TRIGGER_MATCH | SR_CONF_LIST,
	SR_CONF_CAPTURE_RATIO | SR_CONF_GET | SR_CONF_SET,
// 	SR_CONF_EXTERNAL_CLOCK | SR_CONF_GET | SR_CONF_SET,
// 	SR_CONF_CLOCK_EDGE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
};

static const int32_t trigger_matches[] = {
	SR_TRIGGER_ZERO,
	SR_TRIGGER_ONE,
	SR_TRIGGER_RISING,
	SR_TRIGGER_FALLING,
	SR_TRIGGER_EDGE,
};

static const char *signal_edges[] = {
	[DS_EDGE_RISING] = "rising",
	[DS_EDGE_FALLING] = "falling",
};


static const uint64_t samplerates[] =
{
	SR_KHZ(10),
	SR_KHZ(20),
	SR_KHZ(50),
	SR_KHZ(100),
	SR_KHZ(200),
	SR_KHZ(500),
	SR_MHZ(1),
	SR_MHZ(2),
	SR_MHZ(5),
	SR_MHZ(10),
	SR_MHZ(20),
	SR_MHZ(25),
	SR_MHZ(50),
	SR_MHZ(75),
	SR_MHZ(100),
};

void *alloc_channel_list(uint8_t num, struct sr_dev_inst *sdi)
{
	struct sr_channel_group *cg;
	struct sr_channel *ch;
	char channel_name[16];
	cg = g_malloc0(sizeof(struct sr_channel_group));
	cg->name = g_strdup("Logic");
	cg->channels = NULL;
	for (int j = 0; j < num; j++)
	{
		sprintf(channel_name, "%d", j);
		ch = sr_channel_new(sdi, j, SR_CHANNEL_LOGIC, TRUE, channel_name);
		cg->channels = g_slist_append(cg->channels, ch);
	}
	return cg;
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	struct drv_context *drvc;
// 	struct dev_context *devc;
	struct sr_dev_inst *sdi;
// 	struct sr_usb_dev_inst *usb;
	
	struct sr_config *src;
// 	const struct dslogic_profile *prof;
	GSList *l, *devices/*, *conn_devices*/;
	struct libusb_device_descriptor dev_desc;
	libusb_device **devlist;
	struct libusb_device_handle *dev_handle;
	const char *conn;
	char manufacturer[64], product[64], serial_num[64], connection_id[64];
	

	drvc = di->context;

	
	conn = NULL;
	for (l = options; l; l = l->next) {
		src = l->data;
		switch (src->key) {
		case SR_CONF_CONN:
			conn = g_variant_get_string(src->data, NULL);
			break;
		}
	}
	if (conn)
		sr_err("Conn: %s\n", conn);
	
	devices = NULL;
	ssize_t cnt = libusb_get_device_list(drvc->sr_ctx->libusb_ctx, &devlist);
	for(uint8_t i = 0; i < cnt; i++)
	{
		libusb_device *dev = devlist[i];
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if(r < 0)
		{
			continue;
		}
// 		printf("vendor: 0x%X product: 0x%X\n", desc.idVendor, desc.idProduct);
		if(desc.idProduct == 0x1338 && desc.idVendor == 0x9999)
		{
			//test if device can be opened
			int ret = libusb_open(dev, &dev_handle);
			sr_err("Ret: %d", ret);
			if(ret < 0)
			{
				libusb_free_device_list(devlist, 1);
				return NULL;
			}
			libusb_get_device_descriptor(dev, &dev_desc);
			libusb_get_string_descriptor_ascii(dev_handle, dev_desc.iSerialNumber, (unsigned char *)serial_num, 64);
			libusb_get_string_descriptor_ascii(dev_handle, dev_desc.iManufacturer, (unsigned char *)manufacturer, 64);
			libusb_get_string_descriptor_ascii(dev_handle, dev_desc.iProduct, (unsigned char *)product, 64);
			
			//Alloc sdi
			sdi = g_malloc0(sizeof(struct sr_dev_inst));
			
			//Remember connection id
			usb_get_port_path(dev, connection_id, sizeof(connection_id));
			sdi->connection_id = g_strdup(connection_id);
			
			
			sdi->status = SR_ST_INACTIVE;
			
			//Remember serial name and vendor
			sdi->serial_num = g_strdup(serial_num);
			sdi->vendor = g_strdup(manufacturer);
			sdi->model = g_strdup(product);
			
			
			printf("serial: %s, manufacturer: %s, product: %s, connection_id: %s\n", serial_num, manufacturer, product, connection_id);
			
			//Collect all channel TODO: make it correct

			sdi->channel_groups = g_slist_append(NULL, alloc_channel_list(16, sdi));
			
			//Have user data
			sdi->priv = NULL;
			
			//Set type USB
			sdi->inst_type = SR_INST_USB;
			
			//Remember bus and address
			sdi->conn = sr_usb_dev_inst_new(libusb_get_bus_number(dev), libusb_get_device_address(dev), NULL);
// 			sdi->priv = custom_data_ptr;
			devices = g_slist_append(devices, sdi);
			libusb_close(dev_handle);
		}
	}

	
	libusb_free_device_list(devlist, 1);


	return std_scan_complete(di, devices);
}

static int dev_open(struct sr_dev_inst *sdi)
{
	sr_err("open htv\n");
	struct sr_dev_driver *di = sdi->driver;
	struct sr_usb_dev_inst *usb = sdi->conn;
	struct drv_context *drvc = di->context;
	struct libusb_device **devlist;
	char connection_id[64];
	int ret;
	
	ssize_t cnt = libusb_get_device_list(drvc->sr_ctx->libusb_ctx, &devlist);
	for(ssize_t i = 0; i < cnt; i++)
	{
		libusb_device *dev = devlist[i];
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if(r < 0)
		{
			sr_warn("Failed to get device descriptor");
			continue;
		}
		if(desc.idProduct != 0x1338 || desc.idVendor != 0x9999)
		{
			continue;
		}
		sr_info("Found compatible device, checking if selected");
		
		if (usb_get_port_path(dev, connection_id, sizeof(connection_id)) < 0)
		{
			sr_err("Could not get connection id");
			continue;
		}
		if(strcmp(connection_id, sdi->connection_id) != 0)
		{
			sr_err("Wrong connection id: %s", connection_id);
			continue;
		}
		
		sr_info("Found htv-la");
		ret = libusb_open(dev, &usb->devhdl);
		if(ret < 0)
		{
			sr_err("Failed to open htv-la: %s.", libusb_error_name(ret));
			libusb_free_device_list(devlist, 1);
			return SR_ERR;
		}
		usb->address = libusb_get_device_address(devlist[i]);
		
		//There should not be a kernel driver now, but if SUMP with CDC is done we need this
		if(libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER))
		{
			if(libusb_kernel_driver_active(usb->devhdl, 0) == 1)
			{
				if((ret = libusb_detach_kernel_driver(usb->devhdl, 0)) < 0)
				{
					sr_err("Failed to detach kernel driver: %s.", libusb_error_name(ret));
					libusb_free_device_list(devlist, 1);
					return SR_ERR;
					break;
				}
			}
		}
		break;
	}
	libusb_free_device_list(devlist, 1);


	ret = libusb_claim_interface(usb->devhdl, 0);
	if (ret != 0)
	{
		switch (ret)
		{
			case LIBUSB_ERROR_BUSY:
				sr_err("Unable to claim USB interface. Another "
							"program or driver has already claimed it.");
				break;
			case LIBUSB_ERROR_NO_DEVICE:
				sr_err("Device has been disconnected.");
				break;
			default:
				sr_err("Unable to claim interface: %s.",
							libusb_error_name(ret));
				break;
		}
	}

	//Lets check what we need to do here
	sdi->priv = g_malloc0(sizeof(htv_la_settings_t));
	sr_err("Device open");
	return SR_OK;
}

static int dev_close(struct sr_dev_inst *sdi)
{
	(void)sdi;

	/* TODO: get handle from sdi->conn and close it. */

	return SR_OK;
}



static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_err("get htv\n");
	int ret;

	(void)data;
	(void)cg;
	struct sr_usb_dev_inst *usb = sdi->conn;
	
	if (!sdi)
		return SR_ERR_ARG;

	switch (key) {
	case SR_CONF_CONN:
		if (!sdi->conn)
			return SR_ERR_ARG;
		*data = g_variant_new_printf("%d.%d", usb->bus, usb->address);
		break;
	case SR_CONF_VOLTAGE_THRESHOLD:
			*data = std_gvar_tuple_double(1.5, 1.9);
		break;
	case SR_CONF_LIMIT_SAMPLES:
		*data = g_variant_new_uint64(64);
		break;
	case SR_CONF_SAMPLERATE:
		*data = g_variant_new_uint64(SR_MHZ(100));
		break;
// 	case SR_CONF_CAPTURE_RATIO:
// 		*data = g_variant_new_uint64(devc->capture_ratio);
		break;
	case SR_CONF_EXTERNAL_CLOCK:
		*data = g_variant_new_boolean(TRUE);
		break;
	case SR_CONF_CONTINUOUS:
		*data = g_variant_new_boolean(TRUE);
		break;
	case SR_CONF_CLOCK_EDGE:
		*data = g_variant_new_string(signal_edges[0]);
		break;
	default:
		return SR_ERR_NA;
	}

	return SR_OK;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_err("set htv\n");
	int ret;
	
	htv_la_settings_t *settings = sdi->priv;;
	int index;
	struct sr_usb_dev_inst *usb = sdi->conn;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
		case SR_CONF_SAMPLERATE:
			index = std_u64_idx(data, samplerates, ARRAY_SIZE(samplerates));
			if(index < 0)
				return SR_ERR_ARG;
			settings->sample_rate = samplerates[index];
			if(settings->sample_rate == SR_KHZ(20))
			{
					int r = libusb_control_transfer(usb->devhdl, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT, HTV_LA_SET_SAMPLE_RATE, 0, 0, (unsigned char *)&settings->sample_rate, 8, 200);
					if(r != 8)
					{
						sr_err("Failed to set samplerate");
						return SR_ERR;
					}
			}

			break;
		case SR_CONF_CAPTURE_RATIO:
			settings->capture_ratio = g_variant_get_uint64(data);
			break;

		case SR_CONF_CONTINUOUS:
// 			devc->continuous_mode = g_variant_get_boolean(data);
			break;
// 		case SR_CONF_CLOCK_EDGE:
// 			if ((idx = std_str_idx(data, ARRAY_AND_SIZE(signal_edges))) < 0)
// 				return SR_ERR_ARG;
// 			devc->clock_edge = idx;
// 			break;
		default:
			ret = SR_ERR_NA;
	}

	return ret;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_err("get list");
	struct dev_context *devc;

	devc = (sdi) ? sdi->priv : NULL;

	switch (key) {
	case SR_CONF_SCAN_OPTIONS:
	case SR_CONF_DEVICE_OPTIONS:
		return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
	case SR_CONF_VOLTAGE_THRESHOLD:
		*data = std_gvar_tuple_double(1.5, 1.9);
		break;
	case SR_CONF_SAMPLERATE:
		if (!devc)
			return SR_ERR_ARG;
		*data = std_gvar_samplerates(samplerates, sizeof(samplerates)/sizeof(uint64_t));
		break;
	case SR_CONF_TRIGGER_MATCH:
		*data = std_gvar_array_i32(ARRAY_AND_SIZE(trigger_matches));
		break;
	case SR_CONF_CLOCK_EDGE:
		*data = g_variant_new_strv(ARRAY_AND_SIZE(signal_edges));
		break;
	default:
		return SR_ERR_NA;
	}

	return SR_OK;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	/* TODO: configure hardware, reset acquisition state, set up
	 * callbacks and send header packet. */

	(void)sdi;

	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	/* TODO: stop acquisition. */

	(void)sdi;

	return SR_OK;
}

static struct sr_dev_driver htv_la_driver_info = {
	.name = "htv-la",
	.longname = "HTV-LA",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = dev_open,
	.dev_close = dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,
	.context = NULL,
};
SR_REGISTER_DEV_DRIVER(htv_la_driver_info);
