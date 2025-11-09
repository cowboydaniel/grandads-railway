# Grandad's Railway - Crossover Control System

Arduino-based automatic control system for a model railway with crossover points.

## Overview

This system controls two railway lines with a crossover section, automatically managing track power to prevent collisions when points are set against approaching trains.

### Track Layout

```
Line 1: __________________________________
                           \
                            \ crossover
                             \
Line 2: __________________________________
```

### Traffic Rules

- **Line 1, Left→Right**: Always free to travel (coacting points)
- **Line 1, Right→Left**: Stopped if points are against them
- **Line 2, Left→Right**: Stopped if points are against them
- **Line 2, Right→Left**: Always free to go either way

## Files

- `railway_crossover_control.ino` - Main Arduino code
- `WIRING_DIAGRAM.md` - Complete wiring instructions and schematic
- `README.md` - This file

## Quick Start

1. **Hardware Setup**: Follow instructions in `WIRING_DIAGRAM.md`
2. **Upload Code**: Open `railway_crossover_control.ino` in Arduino IDE and upload to your board
3. **Test**: Follow the testing procedure in the wiring diagram document
4. **Run**: Open Serial Monitor (9600 baud) to see system status

## Features

- Automatic track power control based on points position
- Debounced sensor reading to prevent false triggers
- Visual feedback with status LEDs
- Serial monitor debugging output
- Emergency stop function
- Safe defaults (tracks powered on startup)

## Hardware Required

- Arduino (Uno, Nano, or similar)
- 2x Relay modules (5V, optocoupler isolated)
- 1x Tortoise slow-motion point motor (with 12V bipolar LED)
- 1x 10kΩ resistor + 1x 5kΩ resistor (voltage divider)
- 3x LEDs with 220Ω resistors (optional status indicators)
- 12V power supply (for Tortoise and track)
- 5V power supply (for Arduino)
- Connecting wires

See `WIRING_DIAGRAM.md` for complete component list and detailed wiring instructions.

## Safety

⚠️ **Read the safety notes in WIRING_DIAGRAM.md before connecting track power!**

- **CRITICAL**: Never connect 12V directly to Arduino - always use the voltage divider (10kΩ + 5kΩ)
- Test voltage divider output with multimeter before connecting to Arduino (~4V expected)
- Always use proper relays rated for your track current
- Use optocoupler-isolated relays to protect the Arduino
- Add fuses to track power circuits
- Test thoroughly at low voltage before full power operation

## Serial Monitor Output

The system provides real-time status via serial monitor:

```
Railway Crossover Control System Initializing...
System Ready!
Points Position: Line 1
Status: Line 1 R→L OPEN | Line 2 L→R BLOCKED
Points changed to: Line 2 (L→R)
Status: Line 1 R→L BLOCKED | Line 2 L→R OPEN
```

## Customization

Edit these constants in the code to match your setup:

- `POINTS_SENSOR_PIN` - Pin for points position sensor (default: 2)
- `RELAY_LINE1_RIGHT_TO_LEFT` - Relay control pin (default: 3)
- `RELAY_LINE2_LEFT_TO_RIGHT` - Relay control pin (default: 4)
- `DEBOUNCE_DELAY` - Sensor debounce time in ms (default: 50)

## Troubleshooting

See the troubleshooting table in `WIRING_DIAGRAM.md` for common issues and solutions.

## License

Open source - use freely for your model railway projects!
