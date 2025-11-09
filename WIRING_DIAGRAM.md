# Railway Crossover Control - Wiring Diagram

## Component List

### Required Components:
- 1x Arduino (Uno, Nano, or similar)
- 2x Relay modules (5V, optocoupler isolated recommended)
- 1x Points position sensor (microswitch, reed switch, or hall sensor)
- 3x LEDs (optional, for visual feedback)
- 3x 220Ω resistors (for LEDs)
- Power supply for Arduino (USB or 7-12V DC)
- Connecting wires

### Optional Components:
- 1x Servo motor (if automating points control)
- 1x Emergency stop button

## Pin Connections

### Input (Points Position Sensor)
```
Points Sensor → Arduino Pin 2
  - Common → Pin 2
  - NO/NC → GND (configure based on your switch type)
  - Uses internal pullup resistor
```

### Outputs (Relays)
```
Line 1 (Right→Left) Relay → Arduino Pin 3
  - VCC → Arduino 5V
  - GND → Arduino GND
  - IN  → Arduino Pin 3
  - COM → Track Power Supply +
  - NO  → Line 1 Right Approach Track +

Line 2 (Left→Right) Relay → Arduino Pin 4
  - VCC → Arduino 5V
  - GND → Arduino GND
  - IN  → Arduino Pin 4
  - COM → Track Power Supply +
  - NO  → Line 2 Left Approach Track +
```

### Status LEDs (Optional)
```
LED 1 (Line 1 Blocked) → Pin 5
  - Pin 5 → 220Ω resistor → LED anode → LED cathode → GND

LED 2 (Line 2 Blocked) → Pin 6
  - Pin 6 → 220Ω resistor → LED anode → LED cathode → GND

LED 3 (Points Position) → Pin 7
  - Pin 7 → 220Ω resistor → LED anode → LED cathode → GND
```

### Optional Servo (Points Motor) → Pin 9
```
Servo:
  - Signal (Orange/Yellow) → Pin 9
  - Power (Red) → 5V (or external power supply)
  - Ground (Brown/Black) → GND
```

## Track Wiring

### Line 1 (Top Line)
```
Left Section (L→R): Direct power (always on)
  Track + → Power Supply +
  Track - → Power Supply -

Right Section (R→L): Through relay
  Track + → Relay 1 NO
  Track - → Power Supply -
```

### Line 2 (Bottom Line)
```
Left Section (L→R): Through relay
  Track + → Relay 2 NO
  Track - → Power Supply -

Right Section (R→L): Direct power (always on)
  Track + → Power Supply +
  Track - → Power Supply -
```

## Points Position Sensor Mounting

The sensor should detect which direction the points are set:

```
Sensor Position: At the points mechanism

LOW (switch open)  = Points favor Line 1 (R→L)
HIGH (switch closed) = Points favor Line 2 (L→R)

Example with Microswitch:
- Mount switch so it's pressed when points move to Line 2 position
- When pressed: Pin 2 reads HIGH (Line 2 mode)
- When released: Pin 2 reads LOW (Line 1 mode)
```

## Power Supply Recommendations

- **Arduino**: 5V via USB or 7-12V via barrel jack
- **Track Power**: 12V DC (adjust based on your train voltage)
- **Relays**: Ensure relays can handle your track current (typically 2-5A minimum)
- **Isolation**: Use optocoupler-isolated relays for safety

## Safety Notes

⚠️ **IMPORTANT SAFETY WARNINGS:**

1. **Never** connect track power directly to Arduino pins
2. **Always** use relays to switch track power
3. **Use** optocoupler-isolated relays to protect the Arduino
4. **Ensure** relay current rating exceeds your maximum track current
5. **Add** fuses to track power circuits (1-3A recommended)
6. **Test** with low voltage first before connecting full track power
7. **Double-check** all wiring before powering on

## Testing Procedure

1. **Initial Test (No Track Power)**
   - Upload code to Arduino
   - Open Serial Monitor (9600 baud)
   - Toggle points sensor manually
   - Verify serial output shows correct points position

2. **LED Test**
   - Check that LEDs illuminate correctly when points change
   - Line 1 LED should light when points favor Line 2
   - Line 2 LED should light when points favor Line 1

3. **Relay Test (No Track Power)**
   - Listen for relay clicking when points change
   - Use multimeter to verify relay contacts switch correctly

4. **Low Voltage Test**
   - Connect 3V battery to track through relays
   - Verify power is cut/restored correctly with points changes

5. **Full Power Test**
   - Connect actual track power supply
   - Test with a single locomotive at low speed
   - Verify train stops when points are against it
   - Gradually increase to normal operating conditions

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| Relays always on/off | Wrong relay type (active HIGH vs LOW) | Check relay module type, may need to invert logic |
| Points position wrong | Sensor wiring reversed | Swap sensor wiring or invert in code |
| Intermittent operation | Poor connections | Check all wire connections, solder if needed |
| Arduino resets | Power supply insufficient | Use separate power supply for Arduino |
| Relays chattering | Debounce time too short | Increase DEBOUNCE_DELAY in code |

## Schematic Diagram (ASCII)

```
                    Arduino
                  ┌─────────┐
Points Sensor ────┤Pin 2    │
                  │         │
                  │    Pin 3├───► Relay 1 ───► Line 1 (R→L) Track
                  │    Pin 4├───► Relay 2 ───► Line 2 (L→R) Track
                  │         │
LED 1 ◄───────────┤Pin 5    │
LED 2 ◄───────────┤Pin 6    │
LED 3 ◄───────────┤Pin 7    │
                  │         │
Servo (opt) ◄─────┤Pin 9    │
                  │         │
                  │  5V  GND│
                  └──┬───┬──┘
                     │   │
                  Power  Ground
```

## Additional Notes

- The code uses internal pullup resistors, so no external resistors needed for the points sensor
- Relay logic is active LOW (pin LOW = relay ON = track power OFF)
- Adjust relay logic in code if using different relay modules
- Consider adding a manual override switch for maintenance
- Add an emergency stop button that calls `emergencyStop()` function
