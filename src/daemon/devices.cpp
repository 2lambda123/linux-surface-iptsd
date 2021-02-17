// SPDX-License-Identifier: GPL-2.0-or-later

#include "devices.hpp"

#include "cone.hpp"
#include "config.hpp"
#include "uinput-device.hpp"

#include <common/types.hpp>
#include <ipts/ipts.h>
#include <ipts/protocol.h>

#include <climits>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <stdexcept>
#include <string>
#include <utility>

static i32 res(i32 virt, i32 phys)
{
	f64 res = (f64)(virt * 10) / (f64)phys;
	return (i32)std::round(res);
}

StylusDevice::StylusDevice(IptsdConfig *conf) : UinputDevice()
{
	this->name = "IPTS Stylus";
	this->vendor = conf->info.vendor;
	this->product = conf->info.product;
	this->version = conf->info.version;

	this->set_evbit(EV_KEY);
	this->set_evbit(EV_ABS);

	this->set_propbit(INPUT_PROP_DIRECT);
	this->set_propbit(INPUT_PROP_POINTER);

	this->set_keybit(BTN_TOUCH);
	this->set_keybit(BTN_STYLUS);
	this->set_keybit(BTN_TOOL_PEN);
	this->set_keybit(BTN_TOOL_RUBBER);

	i32 res_x = res(IPTS_MAX_X, conf->width);
	i32 res_y = res(IPTS_MAX_Y, conf->height);

	this->set_absinfo(ABS_X, 0, IPTS_MAX_X, res_x);
	this->set_absinfo(ABS_Y, 0, IPTS_MAX_Y, res_y);
	this->set_absinfo(ABS_PRESSURE, 0, 4096, 0);
	this->set_absinfo(ABS_TILT_X, -9000, 9000, 18000 / M_PI);
	this->set_absinfo(ABS_TILT_Y, -9000, 9000, 18000 / M_PI);
	this->set_absinfo(ABS_MISC, 0, USHRT_MAX, 0);

	this->create();
}

TouchDevice::TouchDevice(IptsdConfig *conf) : UinputDevice(), manager(conf)
{
	this->name = "IPTS Touch";
	this->vendor = conf->info.vendor;
	this->product = conf->info.product;
	this->version = conf->info.version;

	this->set_evbit(EV_ABS);
	this->set_evbit(EV_KEY);

	this->set_propbit(INPUT_PROP_DIRECT);
	this->set_keybit(BTN_TOUCH);

	f32 diag = std::sqrt(conf->width * conf->width + conf->height * conf->height);
	i32 res_x = res(IPTS_MAX_X, conf->width);
	i32 res_y = res(IPTS_MAX_Y, conf->height);
	i32 res_d = res(IPTS_DIAGONAL, diag);

	this->set_absinfo(ABS_MT_SLOT, 0, conf->info.max_contacts, 0);
	this->set_absinfo(ABS_MT_TRACKING_ID, 0, conf->info.max_contacts, 0);
	this->set_absinfo(ABS_MT_POSITION_X, 0, IPTS_MAX_X, res_x);
	this->set_absinfo(ABS_MT_POSITION_Y, 0, IPTS_MAX_Y, res_y);
	this->set_absinfo(ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0);
	this->set_absinfo(ABS_MT_TOOL_X, 0, IPTS_MAX_X, res_x);
	this->set_absinfo(ABS_MT_TOOL_X, 0, IPTS_MAX_X, res_x);
	this->set_absinfo(ABS_MT_ORIENTATION, 0, 180, 0);
	this->set_absinfo(ABS_MT_TOUCH_MAJOR, 0, IPTS_DIAGONAL, res_d);
	this->set_absinfo(ABS_MT_TOUCH_MINOR, 0, IPTS_DIAGONAL, res_d);
	this->set_absinfo(ABS_X, 0, IPTS_MAX_X, res_x);
	this->set_absinfo(ABS_Y, 0, IPTS_MAX_Y, res_y);

	this->create();
}

DeviceManager::DeviceManager(IptsdConfig *conf) : touch(conf)
{
	if (conf->width == 0 || conf->height == 0)
		throw std::runtime_error("Display size is 0");

	this->conf = conf;
	this->switch_stylus(0);
}

DeviceManager::~DeviceManager(void)
{
	for (size_t i = 0; i < std::size(this->styli); i++)
		delete std::exchange(this->styli[i], nullptr);
}

void DeviceManager::switch_stylus(u32 serial)
{
	for (size_t i = 0; i < std::size(this->styli); i++) {
		if (this->styli[i]->serial != serial)
			continue;

		this->active_stylus = this->styli[i];
		return;
	}

	if (std::size(this->styli) > 0 && this->active_stylus->serial == 0) {
		this->active_stylus->serial = serial;
		return;
	}

	StylusDevice *stylus = new StylusDevice(this->conf);
	stylus->serial = serial;

	this->styli.insert(this->styli.end(), stylus);
	this->active_stylus = stylus;

	// TouchProcessor *tp = &this->touch.processor;
	// tp->rejection_cones.insert(tp->rejection_cones.end(), &stylus->cone);
}