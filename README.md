# MIL-STD Architecture C++ Application

This project is a C++ application based on a "battle-hardened" architecture used in several NATO land and naval C2 prototypes. It is designed to be highly modular, extensible, and real-time capable, with a strict separation of concerns between device communication, protocol parsing, and business logic. The application is fully compatible with the Qt ecosystem.

## Architecture

The architecture is designed to be robust and scalable, with a clear separation of layers:

*   **Transport Layer**: Responsible for the raw communication with hardware devices (e.g., serial port, Modbus, TCP/IP). It sends and receives byte streams.
*   **Protocol Layer**: Parses the raw byte streams from the Transport Layer into meaningful messages. It understands the specific protocol of a device.
*   **Device Layer**: Contains the business logic for each device. It consumes messages from the Protocol Layer and exposes a high-level API to the rest of the application.
*   **System Controller**: The central orchestrator that manages the lifecycle of all devices, connects the different layers, and exposes system-level functionality.
*   **Data Model**: A centralized, thread-safe data model that holds the state of the entire system.

### Key Features

*   **Real-time Isolation**: All I/O operations are performed in a dedicated, high-priority thread to ensure the GUI remains responsive.
*   **Zero-copy Data Flow**: Data is passed between threads using `std::shared_ptr<const T>`, avoiding unnecessary copies and ensuring thread safety.
*   **Hot-plug and Redundancy**: The architecture supports automatic reconnection to devices in case of link errors.
*   **Modularity and Extensibility**: Adding new devices is a structured process that involves implementing a few key components.
*   **Mockability**: The use of interfaces for the Transport and Protocol layers makes the system highly testable.
*   **Configuration-driven**: The entire system is configured through a single JSON file (`system_config.json`).

## Directory Structure

The repository is organized into the following directories:

*   `communication/`: Contains the Transport Layer implementations (e.g., `SerialPortTransport`, `ModbusTransport`).
*   `data/`: Defines the data structures used throughout the application (`DataTypes.h`).
*   `devices/`: Contains the Device Layer implementations (e.g., `RadarDevice`, `LRFDevice`).
*   `interfaces/`: Defines the core interfaces (`IDevice`, `Transport`, `ProtocolParser`, `Message`).
*   `protocols/`: Contains the Protocol Layer implementations (e.g., `NmeaParser`, `LrfProtocolParser`).
*   `system_config.json`: The main configuration file for the application.

## Getting Started

### Prerequisites

*   Qt 6.x
*   A C++17 compliant compiler

### Building the Application

1.  Open `MIL-STD-architecture.pro` in Qt Creator.
2.  Configure the project for your target environment.
3.  Build and run the application.

## Adding a New Device

To add a new device to the system, you need to follow these steps:

1.  **Define the Data Structure**: In `data/DataTypes.h`, create a new `struct` to hold the data for your device.
2.  **Define the Message Format (if necessary)**: If your device uses a custom protocol, create a new message class in `protocols/` that inherits from `Message`.
3.  **Implement the Protocol Parser**: In `protocols/`, create a new class that inherits from `ProtocolParser` and implements the `parse()` method to convert raw data into your device's messages.
4.  **Implement the Device Logic**: In `devices/`, create a new class that inherits from `IDevice` (or `TemplatedDevice<T>`). This class will contain the business logic for your device.
5.  **Update the System Controller**: In `systemcontroller.cpp`, update the `createDevices()` method to instantiate your new device.
6.  **Configure the Device**: In `system_config.json`, add a new entry for your device, specifying its type, communication parameters, and other settings.

## Configuration

The `system_config.json` file is used to configure all devices and system-level parameters.

### Device Configuration

Each device is configured under the `devices` object. Here is an example for a radar device:

```json
"radar": {
  "enabled": true,
  "type": "RadarDevice",
  "communication": {
    "protocol": "serial",
    "port": "COM1",
    "baudRate": 4800,
    "maxRetries": 3,
    "reconnectBaseDelayMs": 1000
  }
}
```

*   `enabled`: Set to `true` to enable the device.
*   `type`: The class name of the device (e.g., `RadarDevice`).
*   `communication`: An object containing the communication parameters.
    *   `protocol`: The protocol used by the device (e.g., `serial`, `modbus`).
    *   ... other protocol-specific parameters.

### System Configuration

The `system` object contains system-level parameters:

```json
"system": {
  "trackingUpdateIntervalMs": 100,
  "logLevel": "debug"
}
```

*   `trackingUpdateIntervalMs`: The interval for the target tracking loop.
*   `logLevel`: The logging level for the application.
