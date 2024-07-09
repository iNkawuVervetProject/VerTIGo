# Pellet Dispenser Python Driver

This is a python package to use the pellet dispenser with python.


## Installing

Install this package with the following shell command

```bash
pip install inkawuvp-pellet-dispenser
```

`inkawuvp-pellet-dispenser` is based on [hid](https://pypi.org/project/hid/) and
therefore require the [hidapi library](https://github.com/libusb/hidapi) to be
installed separately.

### Installing hidapi

#### Linux

Depending on your distribution you will need to install a  hidapi package.

##### Ubuntu / Debian based

```bash
sudo apt install libhidapi-hidraw0
```


#### macOS

The easiest is to use [Homebrew](https://brew.sh):

```bash
brew install hidapi
```


## Sample usage code

```python

from inkawuvp_pellet_dispenser import Device, DispenserError

dev = Device()

try:
    dispensed = dev.dispense(1)
    print(f'Dispensed {dispensed} pellets')
except DispenserError as e:
    print(e)
    print(f'still dispensed {e.dispensed}')
```
