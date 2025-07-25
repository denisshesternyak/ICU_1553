# MIL-STD-1553 Communication Framework

This project implements a three-component communication system using **MIL-STD-1553** and **UDP** between modules:

- `platform` — command sender
- `icu` — bridge between 1553 and the network
- `irst` — remote command processor and responder

## Driver

The project communicates with MIL-STD-1553 hardware using a custom Linux kernel driver named `excalbr`. The driver must be compiled, loaded, and properly interfaced before running the application.

### Build the driver

To build the kernel module:

```shell
cd driver
make
```

This will produce the file `excalbr.ko`, which is the kernel module.
Ensure you have Linux kernel headers installed for your current kernel:

```shell
sudo apt install linux-headers-$(uname -r)
```

### Load the driver

To load the driver:

```shell
sudo ./excload
```

Example output:

```
loading module excalbr.ko
The major number of the excalbr device is 235
```

This step inserts the `excalbr` module into the kernel.

### Unload the driver

To unload the driver from the kernel:

```shell
sudo ./excunload
```

Make sure you're using `sudo`.

### Integration

You must load the driver before running any of the components (`platform`, `icu`, `irst`).

## Build components

```shell
mkdir build
cd build
cmake ..
make
```

After building, the binaries will appear in the `build/src/` directory:

```shell
build/src/
├── platform/platform
├── icu/icu
└── irst/irst
```

## Configuration

All system settings are defined in the config.ini file. This file is parsed at startup to configure devices, network interfaces, and message behavior.

### [device]
```ini
[device]
Device_Number=0
Module_Number=0
RT_Addr=5
```

| Key             | Description                                               |
| --------------- | --------------------------------------------------------- |
| `Device_Number` | Index of the MIL-STD-1553 device (e.g., 0 for `/dev/bc0`) |
| `Module_Number` | Internal module index (for multi-module boards)           |
| `RT_Addr`       | Remote Terminal address (valid range: 0–31)               |

### [source]
```ini
[source]
IP=192.168.10.74
Port=12345
```

| Key    | Description                                             |
| ------ | ------------------------------------------------------- |
| `IP`   | Local IP address used for sending/receiving UDP packets |
| `Port` | UDP port number on the source machine                   |

### [destination]
```ini
[destination]
IP=192.168.10.26
Port=12345
```

| Key    | Description                                    |
| ------ | ---------------------------------------------- |
| `IP`   | Target IP address (e.g., IRST or ICU endpoint) |
| `Port` | UDP port number on the destination device      |

### [send_command_N]
Each send_command_N block configures a command that should be transmitted over the 1553 bus.

```ini
[send_command_1]
OpCode=0x01
Rate=10
SubAddress=1
Message=IRST_01_commands
```

| Key          | Description                                       |
| ------------ | ------------------------------------------------- |
| `OpCode`     | Operation code of the command (hex or decimal)    |
| `Rate`       | Transmission interval in Hz (messages per second) |
| `SubAddress` | RT subaddress to send this message to             |
| `Message`    | Payload content (plain ASCII text)                |

### [get_command_N]
Each get_command_N block configures a command that should be received from the 1553 bus.

```ini
[get_command_1]
OpCode=0x0B
Rate=10
SubAddress=11
```

| Key          | Description                                    |
| ------------ | ---------------------------------------------- |
| `OpCode`     | Operation code expected from incoming messages |
| `Rate`       | Polling or expected frequency in Hz            |
| `SubAddress` | Subaddress from which this message will arrive |


## Communication Flow

1. `platform` sends commands via the MIL-STD-1553 bus.
2. `icu` receives and translates them into UDP packets and forwards to `irst`.
3. `irst` processes the commands and sends responses via UDP.
4. `icu` receives and stores the responses in memory.
5. `platform` requests and retrieves those responses via 1553.

## Running Examples

### platform

```shell
./platform
```

Sample output:

```
Init MODULE_1553 success!
...
Time           Dir  OpCode   Len    Message
10:46:47.890   T    0x09     23     IRST_09_navigation_data
10:46:47.965   R    0x0B     21     IRST_11_status_report
```

### icu

```shell
./icu
```

Sample output:

```
Init MODULE_1553 success!
Init SOCKET success!
...
Time           From       To         OpCode   Len    Message
10:46:47.895   PLATFORM   IRST       0x09     23     IRST_09_navigation_data
10:46:47.960   IRST       PLATFORM   0x0B     21     IRST_11_status_report
```

### irst

```shell
./irst
```

Sample output:

```
Init SOCKET success!
Server listening on port 12345...
...
Time           Dir  OpCode   Len    Message
10:46:47.824   R    0x10     10     ICU STATUS
10:46:47.824   T    0x11     14     ICU STATUS ACK
```

## Code Structure

* `platform/` — 1553 message sender
* `icu/` — converter between MIL-STD-1553 and UDP
* `irst/` — UDP-based responder
* `config.h`, `config.ini` — device and command configuration
* `mil_std_1553.h` — low-level driver interface
* `client.h`, `server.h` — socket communication

## Dependencies

* Linux / POSIX-compliant system
* `cmake >= 3.10`
* MIL-STD-1553 support via `Galahad_Px` or compatible driver

## Signal Handling

The application gracefully shuts down on Ctrl+C (`SIGINT`) and terminates all threads properly.


