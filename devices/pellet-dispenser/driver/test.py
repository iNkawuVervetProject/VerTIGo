import hid
import ctypes


# default is TinyUSB (0xcafe), Adafruit (0x239a), RaspberryPi (0x2e8a), Espressif (0x303a) VID
USB_VID = (0xCAFE, 0x239A, 0x2E8A, 0x303A)

print("VID list: " + ", ".join("%02x" % v for v in USB_VID))

dev = None

for vid in USB_VID:
    for dict in hid.enumerate(vid):
        dev = hid.Device(dict["vendor_id"], dict["product_id"])
if dev is None:
    raise RuntimeError("No device found")


print(dev.get_feature_report(0, 16))
print(dev.get_feature_report(1, 16))
print(dev.get_feature_report(2, 16))
print(dev.get_input_report(2, 16))

dev.write(b"\x00\x01\x03")
