# psychopy-zaci

Extension to add support for ZACI's devices such as the pellet dispenser.

## Supported Devices

Installing this package alongside PsychoPy will enable support for the following devices:

* [Pellet Dispenser](https://github.com/atuleu/zaci/tree/main/devices/pellet-dispenser)

## Installing

Install this package with the following shell command:

```bash
pip install psychopy-zaci
```

You may also use PsychoPy's builtin package manager to install this package.

## Usage

The following components will be available after loading this plugin in Builder:

* PelletDispenser

### Pellet Dispenser

The Pellet Dispenser component allows one single dispense action in a
routine. By default it starts the dispense action with the component, and
terminate the routine when the dispense action is done. Due to the impredictable
timing of the dispense action (several attempt will be made by the device in
case of jamming), it is preferable to use a routine to the sole purpose of the
dispensing action.

The `examples` directory contains `.psyexp` files that show how you can
integrate the pellet dispenser.
