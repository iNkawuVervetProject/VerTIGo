
#include "../Log.hpp"
#include "class/hid/hid_device.h"
#include "tusb.h"
#include <cstring>
#include <sstream>
// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request

extern "C" {

// Invoked when device is mounted
void tud_mount_cb(void) {
	Infof("USB: Mounted");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
	Infof("USB: Unmounted");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
	(void)remote_wakeup_en;
	Infof("USB: suspended");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
	Infof("USB: resumed");
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(
    uint8_t instance, uint8_t const *report, uint16_t len
) {
	Debugf(
	    "USB: report complete {instance: %d, report:%x, len:%d",
	    instance,
	    report[0],
	    len
	);
}

uint16_t tud_hid_get_report_cb(
    uint8_t           instance,
    uint8_t           report_id,
    hid_report_type_t report_type,
    uint8_t          *buffer,
    uint16_t          reqlen
) {

	Debugf(
	    "get report {instance: %d, id: %d, type: %x, reqlen",
	    instance,
	    report_id,
	    report_type
	);
	return reqlen;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(
    uint8_t           instance,
    uint8_t           report_id,
    hid_report_type_t report_type,
    uint8_t const    *buffer,
    uint16_t          bufsize
) {
	(void)instance;

	std::ostringstream oss;
	for (size_t i = 0; i < bufsize; ++i) {
		if (i > 0) {
			if (i % 4 == 0) {
				oss << "  ";
			} else {
				oss << " ";
			}
		}
		oss << std::hex << int(buffer[i]) << ":{" << buffer[i] << "}";
	}

	Debugf(
	    "set report {instance: %d, id: %d, type: %x,data:%s ",
	    instance,
	    report_id,
	    report_type,
	    oss.str().c_str()
	);

	tud_hid_report(0xaa, "World", 5);
}
}
