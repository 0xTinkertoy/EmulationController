# Emulation Controller

## Introduction

This repository contains the source code of the controller that runs on the host system and interacts with each device in the emulated automatic watering system.
The controller acts like Wireshark, intercepting each message sent by a device, printing the message and redirecting the message to the receiver.

## Usage

```bash
./Controller -m <MonitorPort> -a <ActuatorPort> -g <GatewayPort>
```

The second serial port of each emulated board can be redirected to a TCP port.  
You need to specify at least a port number so that the controller can interact with that device.
For example, to play with the monitor device only, you can run the controller with the following command.

```bash
# Run the monitor kernel
qemu-system-arm -cpu cortex-m3 -M lm3s811evb -kernel build/Kernel -serial stdio -serial tcp::10000,server,wait

# Run the controller and connect to the monitor kernel
./Controller -m 10000
```

Once the controller has connected to the monitor kernel, it acts as a terminal, waiting for your commands.

- `exit`: Quit the emulation controller.
- `soil <LEVEL>`: Change the soil moisture level to <LEVEL>% 
  - For example, `soil 10` will set the value of the emulated sensor to 10 on the monitor board.
- `water <FLAG>`: Change the status of the water bottle.
  - `water 1` will fill the bottle with water.
  - `water 0` will empty the bottle; the emulated sensor will report that the bottle is running out of water.
- `dry`: Send a dry soil alert message to the actuator device on behalf of the monitor device.
- `wet`: Send a wet soil alert message to the actuator device on behalf of the monitor device.
- `coap`: Send a single CoAP message to the gateway device on behalf of the monitor device.
- `gateway <TRIALS> <DELAY>`: Run the experiment on the gateway kernel, measuring the amount of time it takes the gateway to process 1000 messages.

## Dependencies

- fmt 9.1.0 (Available on Homebrew (macOS) and APT (Ubuntu))

## Compiler Requirements

- GCC 10 or later
- Clang 13 or later
- AppleClang 13.1.6 or later (i.e. Xcode 13.3 or later)

## Compilation

Emulation Controller uses CMake as its build system.  
A `CMakeLists.txt` is provided to build the project.

## IDE Support

Emulation Controller supports both CLion and Xcode.   
A Xcode project has been provided to build the controller on macOS easily.

## License

This project is licensed under BSD-3-Clause.
