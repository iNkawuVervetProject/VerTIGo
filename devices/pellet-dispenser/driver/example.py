from unil_pellet_dispenser import Device, Error

dev = Device()
report = dev.dispense(1)
if report.has_error():
    print("Could not dispense: %s" % report.error_description)
else:
    print("Dispensed %d" % report.value)
