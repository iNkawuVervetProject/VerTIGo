
#include "callbacks.h"
#include "../Log.hpp"
#include "ErrorReporter.hpp"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "protocol.hpp"
#include "tusb.h"
#include <cstring>
#include <sstream>

#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/time.h"
static DispenserController *dispenser = nullptr;

void tud_register_dispenser(DispenserController *d) {
	dispenser = d;
}

bool                                 sending = false;
Queue<usb::CommandReport, 32, false> reportQueue;

extern "C" {

void tud_hid_command_report(const usb::CommandReport &report) {
	if (sending == true) {
		if (reportQueue.TryAdd(report) == false) {
			Errorf("USB: cannot send command report: queue is full");
		}
		return;
	}
	sending = true;
	tud_hid_report(
	    0,
	    reinterpret_cast<void const *>(&report),
	    sizeof(usb::CommandReport)
	);
}

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

	Infof("USB: suspended: can wake:%s", remote_wakeup_en ? "true" : "false");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
	Infof("USB: resumed");
}

void reboot_to_bootsel() {
	multicore_reset_core1();
	reset_usb_boot(0, 0);
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

	if (reinterpret_cast<const usb::CommandReport *>(report)->Code ==
	    usb::Command::BOOTSEL) {
		reboot_to_bootsel();
	}

	usb::CommandReport queued;
	if (reportQueue.TryRemove(queued) == false) {
		sending = false;
		return;
	}

	tud_hid_report(
	    0,
	    reinterpret_cast<void const *>(&queued),
	    sizeof(usb::CommandReport)
	);
}

uint16_t pop_and_fill_error_log(uint8_t *buffer, uint16_t reqlen) {
	constexpr static uint16_t MinErrorReportSize =
	    sizeof(ErrorReporter::LoggedError) + 1;

	if (reqlen < MinErrorReportSize) {
		Errorf(
		    "USB: invalid GET_FEATURE ErrorReport: min buffer size is %d, got "
		    "%d",
		    MinErrorReportSize,
		    reqlen
		);
		return 0;
	}

	buffer[0] = ErrorReporter::ErrorLog().Size();

	const size_t maxReport{(reqlen - 1) / sizeof(ErrorReporter::LoggedError)};
	size_t       i{0};

	ErrorReporter::LoggedError err;

	for (; i < maxReport; ++i) {
		if (ErrorReporter::ErrorLog().TryRemove(err) == false) {
			break;
		}
		memcpy(
		    &(buffer[sizeof(ErrorReporter::LoggedError) * i + 1]),
		    &err,
		    sizeof(ErrorReporter::LoggedError)
		);
	}

	return i * sizeof(ErrorReporter::LoggedError) + 1;
}

uint16_t fill_current_time(uint8_t *buffer, uint16_t reqlen) {
	if (reqlen < sizeof(usb::TimeReport)) {
		Errorf(
		    "USB: invalid GET_FEATURE TimeReport: buffer size should be %d, "
		    "got %d",
		    sizeof(usb::TimeReport),
		    reqlen
		);
		return 0;
	}

	auto now = get_absolute_time();
	memcpy(buffer, &now, sizeof(usb::TimeReport));

	return sizeof(usb::TimeReport);
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(
    uint8_t           instance,
    uint8_t           report_id,
    hid_report_type_t report_type,
    uint8_t          *buffer,
    uint16_t          reqlen
) {

	Debugf(
	    "USB: instance: %d, report ID:%d type:%d reqlen: %d",
	    instance,
	    report_id,
	    report_type,
	    reqlen
	);

	if (report_type != HID_REPORT_TYPE_FEATURE) {
		Warnf("USB: received GET_REPORT with invalid type %d", report_type);
		return 0;
	}

	switch (usb::FeatureReportType(report_id)) {
	case usb::FeatureReportType::ERROR_LOG:
		return pop_and_fill_error_log(buffer, reqlen);
	case usb::FeatureReportType::CURRENT_TIME:
		return fill_current_time(buffer, reqlen);
	default:
		Warnf("USB: GET FEATURE REPORT with invalid ID %d", report_id);
		return 0;
	}
}

void dispense(int16_t count) {
	if (dispenser == nullptr) {
		Errorf("USB:dispense: no dispenser reference set");
		return;
	}

	dispenser->Dispense(count, [](uint count, Error err) {
		if (err != Error::NO_ERROR) {
			ErrorReporter::Report(err, 10);
		}

		tud_hid_command_report(usb::CommandReport{
		    .Code  = usb::Command::DISPENSE,
		    .Value = uint16_t(count),
		    .Error = uint8_t(err),
		});
	});
}

void calibrate(int16_t speed) {
	if (dispenser == nullptr) {
		Errorf("USB:calibrate: no dispenser reference set");
		return;
	}

	dispenser->SetSpeedAndCalibrate(speed, [](Error err) {
		if (err != Error::NO_ERROR) {
			ErrorReporter::Report(err, 10);
		}
		tud_hid_command_report(usb::CommandReport{
		    .Code  = usb::Command::CALIBRATE,
		    .Value = 0,
		    .Error = uint8_t(err)});
	});
}

void boot_to_bootsel() {}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(
    uint8_t           instance,
    uint8_t           report_id,
    hid_report_type_t report_type,
    uint8_t const    *buffer,
    uint16_t          bufsize
) {

	if (report_type != 0) {
		Warnf("USB: invalid command %x", report_type);
		return;
	}
	if (bufsize < sizeof(usb::Command)) {
		Errorf("USB: invalid command size %d", bufsize);
		return;
	}
	auto command = reinterpret_cast<const usb::Command *>(buffer);
	switch (command->Code) {
	case usb::Command::DISPENSE:
		dispense(command->Parameter);
		break;
	case usb::Command::CALIBRATE:
		calibrate(command->Parameter);
		break;

	case usb::Command::BOOTSEL:
		tud_hid_command_report(usb::CommandReport{
		    .Code  = usb::Command::BOOTSEL,
		    .Value = 0,
		    .Error = 0,
		});
		break;
	default:
		Errorf("USB: invalid command %x", command->Code);
		break;
	}
}
} // extern "C"
