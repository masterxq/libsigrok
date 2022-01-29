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
	SR_CONF_LIMIT_SAMPLES | SR_CONF_GET | SR_CONF_SET,
	SR_CONF_VOLTAGE_THRESHOLD | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_CONN | SR_CONF_GET,
	SR_CONF_SAMPLERATE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_TRIGGER_MATCH | SR_CONF_LIST,
	SR_CONF_CAPTURE_RATIO | SR_CONF_GET | SR_CONF_SET,
	SR_CONF_EXTERNAL_CLOCK | SR_CONF_GET | SR_CONF_SET,
	SR_CONF_CLOCK_EDGE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
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


static const uint64_t samplerates[] = {
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
	SR_MHZ(100),
	SR_MHZ(200),
	SR_MHZ(400),
};

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	struct drv_context *drvc;
// 	struct dev_context *devc;
	struct sr_dev_inst *sdi;
// 	struct sr_usb_dev_inst *usb;
	struct sr_channel *ch;
	struct sr_channel_group *cg;
	struct sr_config *src;
// 	const struct dslogic_profile *prof;
	GSList *l, *devices/*, *conn_devices*/;
	struct libusb_device_descriptor dev_desc;
	libusb_device **devlist;
	struct libusb_device_handle *dev_handle;
	const char *conn;
	char manufacturer[64], product[64], serial_num[64], connection_id[64];
	char channel_name[16];

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
			printf("serial: %s, manufacturer: %s, product: %s\n", serial_num, manufacturer, product);
			usb_get_port_path(dev, connection_id, sizeof(connection_id));
			sdi = g_malloc0(sizeof(struct sr_dev_inst));
			sdi->status = SR_ST_INITIALIZING;
			sdi->serial_num = g_strdup(serial_num);
			sdi->vendor = g_strdup(manufacturer);
			sdi->model = g_strdup(product);
			sdi->connection_id = g_strdup(connection_id);
			
			cg = g_malloc0(sizeof(struct sr_channel_group));
			cg->name = g_strdup("Logic");
			cg->channels = NULL;
			for (int j = 0; j < 16; j++)
			{
				sprintf(channel_name, "%d", j);
				ch = sr_channel_new(sdi, j, SR_CHANNEL_LOGIC, TRUE, channel_name);
				cg->channels = g_slist_append(cg->channels, ch);
			}
			sdi->channel_groups = g_slist_append(NULL, cg);
			sdi->priv = NULL;
			sdi->inst_type = SR_INST_USB;
			sdi->conn = sr_usb_dev_inst_new(libusb_get_bus_number(dev), libusb_get_device_address(dev), NULL);
// 			sdi->priv = custom_data_ptr;
			devices = g_slist_append(devices, sdi);
			libusb_close(dev_handle);
		}
	}

	
	libusb_free_device_list(devlist, 1);



// 		sdi = g_malloc0(sizeof(struct sr_dev_inst));
// 		sdi->status = SR_ST_INITIALIZING;
// 		sdi->vendor = g_strdup(prof->vendor);
// 		sdi->model = g_strdup(prof->model);
// 		sdi->version = g_strdup(prof->model_version);
// 		sdi->serial_num = g_strdup(serial_num);
// 		sdi->connection_id = g_strdup(connection_id);

// 		/* Logic channels, all in one channel group. */
// 		cg = g_malloc0(sizeof(struct sr_channel_group));
// 		cg->name = g_strdup("Logic");
// 		for (j = 0; j < NUM_CHANNELS; j++) {
// 			sprintf(channel_name, "%d", j);
// 			ch = sr_channel_new(sdi, j, SR_CHANNEL_LOGIC,
// 						TRUE, channel_name);
// 			cg->channels = g_slist_append(cg->channels, ch);
// 		}
// 		sdi->channel_groups = g_slist_append(NULL, cg);

// 		devc = dslogic_dev_new();
// 		devc->profile = prof;
// 		sdi->priv = devc;
// 		devices = g_slist_append(devices, sdi);
// 
// 		devc->samplerates = samplerates;
// 		devc->num_samplerates = ARRAY_SIZE(samplerates);
// 		has_firmware = usb_match_manuf_prod(devlist[i], "DreamSourceLab", "USB-based Instrument");
// 
// 		if (has_firmware) {
// 			/* Already has the firmware, so fix the new address. */
// 			sr_dbg("Found a DSLogic device.");
// 			sdi->status = SR_ST_INACTIVE;
// 			sdi->inst_type = SR_INST_USB;
// 			sdi->conn = sr_usb_dev_inst_new(libusb_get_bus_number(devlist[i]),
// 					libusb_get_device_address(devlist[i]), NULL);
// 		} else {
// 			if (ezusb_upload_firmware(drvc->sr_ctx, devlist[i],
// 					USB_CONFIGURATION, prof->firmware) == SR_OK) {
// 				/* Store when this device's FW was updated. */
// 				devc->fw_updated = g_get_monotonic_time();
// 			} else {
// 				sr_err("Firmware upload failed for "
// 				       "device %d.%d (logical), name %s.",
// 				       libusb_get_bus_number(devlist[i]),
// 				       libusb_get_device_address(devlist[i]),
// 				       prof->firmware);
// 			}
// 			sdi->inst_type = SR_INST_USB;
// 			sdi->conn = sr_usb_dev_inst_new(libusb_get_bus_number(devlist[i]),
// 					0xff, NULL);
// 		}
// 	}
// 	libusb_free_device_list(devlist, 1);
// 	g_slist_free_full(conn_devices, (GDestroyNotify)sr_usb_dev_inst_free);

	return std_scan_complete(di, devices);
}

static int dev_open(struct sr_dev_inst *sdi)
{
	sr_err("open htv\n");
	struct sr_dev_driver *di = sdi->driver;
	struct sr_usb_dev_inst *usb;
	int ret;
	int64_t timediff_us, timediff_ms;


	usb = sdi->conn;

	/*
	 * If the firmware was recently uploaded, wait up to MAX_RENUM_DELAY_MS
	 * milliseconds for the FX2 to renumerate.
	 */
	ret = SR_ERR;

// 	sr_info("Firmware upload was not needed.");
// 	ret = htv_la_dev_open(sdi, di);
// 
// 
// 	if (ret != SR_OK) {
// 		sr_err("Unable to open device.");
// 		return SR_ERR;
// 	}
// 
// 	ret = libusb_claim_interface(usb->devhdl, USB_INTERFACE);
// 	if (ret != 0) {
// 		switch (ret) {
// 		case LIBUSB_ERROR_BUSY:
// 			sr_err("Unable to claim USB interface. Another "
// 			       "program or driver has already claimed it.");
// 			break;
// 		case LIBUSB_ERROR_NO_DEVICE:
// 			sr_err("Device has been disconnected.");
// 			break;
// 		default:
// 			sr_err("Unable to claim interface: %s.",
// 			       libusb_error_name(ret));
// 			break;
// 		}
// 
// 		return SR_ERR;
// 	}
// 
// 
// 	if ((ret = dslogic_fpga_firmware_upload(sdi)) != SR_OK)
// 		return ret;
// 
// 	if (devc->cur_samplerate == 0) {
// 		/* Samplerate hasn't been set; default to the slowest one. */
// 		devc->cur_samplerate = devc->samplerates[0];
// 	}
// 
// 	if (devc->cur_threshold == 0.0) {
// 		devc->cur_threshold = thresholds[1][0];
// 		return dslogic_set_voltage_threshold(sdi, devc->cur_threshold);
// 	}


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

	(void)sdi;
	(void)data;
	(void)cg;

	
	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_err("set htv\n");
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		ret = SR_ERR_NA;
	}

	return ret;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;

	devc = (sdi) ? sdi->priv : NULL;

	switch (key) {
	case SR_CONF_SCAN_OPTIONS:
	case SR_CONF_DEVICE_OPTIONS:
		return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
	case SR_CONF_VOLTAGE_THRESHOLD:
		*data = std_gvar_min_max_step_thresholds(0.0, 5.0, 0.1);
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
