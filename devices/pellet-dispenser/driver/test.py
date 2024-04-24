import hid

# default is TinyUSB (0xcafe), Adafruit (0x239a), RaspberryPi (0x2e8a), Espressif (0x303a) VID
USB_VID = (0xCAFE, 0x239A, 0x2E8A, 0x303A)

print("VID list: " + ", ".join("%02x" % v for v in USB_VID))

dev = None

for vid in USB_VID:
    for dict in hid.enumerate(vid):
        print(dict)
        dev = hid.Device(dict["vendor_id"], dict["product_id"])
if dev:
    # Get input from console and encode to UTF8 for array of chars.
    # hid generic in/out is single report therefore by HIDAPI requirement
    # it must be preceded, with 0x00 as dummy reportID
    str_out = b"\x00"
    str_out += "hello".encode("utf-8")
    dev.write(str_out)
    str_in = dev.read(64)
    print("Received from HID Device:", str_in, "\n")
