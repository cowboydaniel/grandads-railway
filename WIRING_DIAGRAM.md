# Railway Crossover Control - Wiring Diagram

## Component List

### Required Components:
- 1x Arduino (Uno, Nano, or similar)
- 2x Relay modules (5V, optocoupler isolated recommended)
- 1x Tortoise point motor (with built-in 12V bipolar LED)
- 1x 10kΩ resistor (for voltage divider)
- 1x 5kΩ resistor (for voltage divider) - or use 4.7kΩ
- 3x LEDs (optional, for visual feedback)
- 3x 220Ω resistors (for status LEDs)
- 12V power supply for Tortoise motor
- 5V power supply for Arduino (USB or 7-12V DC via barrel jack)
- Connecting wires

### Optional Components:
- 1x DPDT switch (for manual points control)
- 1x Emergency stop button

## Pin Connections

### Input (Points Position Sensor - Tortoise LED via Resistor Divider)
```
Tortoise Bipolar LED → Resistor Divider → Arduino Pin 2

Circuit:
  Tortoise LED Terminal 1 (12V when points set one way)
    ↓
  10kΩ resistor (R1)
    ↓
    ├─────→ Arduino Pin 2 (reads ~4V when LED forward biased, ~0V when reverse)
    ↓
  5kΩ resistor (R2) - can substitute 4.7kΩ
    ↓
  GND

Tortoise LED Terminal 2 connects to the opposite polarity from Tortoise motor driver

How it works:
- When Tortoise moves points to Line 1: LED reverse biased → Pin 2 reads LOW
- When Tortoise moves points to Line 2: LED forward biased → ~12V divided to ~4V → Pin 2 reads HIGH
- Voltage divider formula: Vout = Vin × (R2 / (R1 + R2)) = 12V × (5k / 15k) = 4V (safe for Arduino)
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

### Tortoise Point Motor
```
Tortoise Slow-Motion Point Motor:
  - Wire 1 (Black) → 12V positive (via DPDT switch for manual control)
  - Wire 8 (Red) → 12V negative/ground (via DPDT switch)
  - Reversing polarity changes points direction

  Internal LED connections (pins 2-3 and 6-7):
  - One LED terminal → 10kΩ resistor → Arduino Pin 2 → 5kΩ resistor → GND
  - Other LED terminal → Tortoise motor circuit (12V alternating polarity)

Note: Tortoise motors draw ~15-16mA and are designed for continuous power.
The built-in SPDT auxiliary contacts can be used for other purposes if needed.
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

## Tortoise LED Polarity Detection

The Tortoise motor's built-in bipolar LED changes polarity based on points position:

```
LED Polarity indicates points direction:

LOW  = Points favor Line 1 (R→L) - LED reverse biased, minimal voltage to Pin 2
HIGH = Points favor Line 2 (L→R) - LED forward biased, ~4V to Pin 2 via divider

Wiring Notes:
- Connect to Tortoise LED terminals (pins 2-3 or 6-7, both sets are identical)
- Polarity depends on how you wire the Tortoise motor power
- If readings are inverted, swap the Tortoise motor power wires (pins 1 & 8)
- The resistor divider ensures Arduino safety (12V → 4V max)

Testing:
- Manually toggle Tortoise with DPDT switch
- Watch Serial Monitor to confirm correct polarity readings
- If inverted, swap either motor wires OR LED divider connection
```

## Power Supply Recommendations

- **Arduino**: 5V via USB or 7-12V via barrel jack
- **Tortoise Motor**: 12V DC (draws ~15-16mA, can be continuously powered)
- **Track Power**: 12V DC (adjust based on your train voltage - can share Tortoise supply)
- **Relays**: Ensure relays can handle your track current (typically 2-5A minimum)
- **Isolation**: Use optocoupler-isolated relays for safety
- **Note**: You can use a single 12V supply for both Tortoise and track power if properly rated

## Safety Notes

⚠️ **IMPORTANT SAFETY WARNINGS:**

1. **Never** connect 12V directly to Arduino pins - always use the resistor divider
2. **Never** connect track power directly to Arduino pins
3. **Always** use relays to switch track power
4. **Use** optocoupler-isolated relays to protect the Arduino
5. **Ensure** relay current rating exceeds your maximum track current
6. **Verify** resistor divider values before connecting (10kΩ + 5kΩ brings 12V → 4V)
7. **Add** fuses to track power circuits (1-3A recommended)
8. **Test** voltage divider output with multimeter first (~4V expected)
9. **Test** with low voltage first before connecting full track power
10. **Double-check** all wiring before powering on

## Testing Procedure

1. **Voltage Divider Test (CRITICAL - Do this first!)**
   - Build the resistor divider circuit (10kΩ + 5kΩ)
   - Connect 12V to the top of the divider
   - Measure voltage at the Arduino pin connection point
   - Should read approximately 4V (3.8V - 4.2V is acceptable)
   - **DO NOT** connect to Arduino until voltage is verified safe (<5V)

2. **Initial Test (No Track Power)**
   - Upload code to Arduino
   - Connect voltage divider to Arduino Pin 2
   - Open Serial Monitor (9600 baud)
   - Manually toggle Tortoise with DPDT switch
   - Verify serial output shows correct points position changes
   - If inverted, swap Tortoise motor wires or LED connections

3. **Status LED Test**
   - Check that status LEDs illuminate correctly when Tortoise moves
   - Line 1 LED should light when points favor Line 2 (blocking Line 1 R→L)
   - Line 2 LED should light when points favor Line 1 (blocking Line 2 L→R)
   - Points position LED reflects current Tortoise position

4. **Relay Test (No Track Power)**
   - Listen for relay clicking when Tortoise changes position
   - Use multimeter to verify relay contacts switch correctly
   - Verify relays energize/de-energize in sync with Tortoise movement

5. **Low Voltage Test**
   - Connect 3V battery to track through relays
   - Toggle Tortoise between positions
   - Verify power is cut/restored correctly with points changes
   - Test both blocked sections

6. **Full Power Test**
   - Connect actual track power supply (12V)
   - Test with a single locomotive at low speed
   - Approach from Line 1 right side, verify train stops when points against it
   - Approach from Line 2 left side, verify train stops when points against it
   - Verify free-running directions work correctly
   - Gradually increase to normal operating conditions

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| Arduino damage/erratic | 12V connected directly to pin | Check voltage divider, ensure <5V at Arduino pin |
| Points position inverted | LED polarity reversed | Swap Tortoise motor wires (pins 1 & 8) |
| No position detection | Voltage divider wrong | Verify 10kΩ and 5kΩ values, check connections |
| Relays always on/off | Wrong relay type (active HIGH vs LOW) | Check relay module type, may need to invert logic |
| Intermittent readings | Poor resistor connections | Solder resistor divider joints |
| Tortoise not moving | No power to motor | Check 12V supply and DPDT switch wiring |
| Arduino resets | Power supply insufficient | Use separate power supply for Arduino |
| Relays chattering | Debounce time too short | Increase DEBOUNCE_DELAY in code |
| Pin reads always HIGH/LOW | Voltage divider disconnected | Check all divider connections with multimeter |

## Schematic Diagram (ASCII)

```
   Tortoise Motor              Arduino                    Track Power
   ┌──────────┐              ┌─────────┐
   │  12V LED │              │         │
   │Terminal  ├──────┐       │         │
   └──────────┘      │       │         │
                  10kΩ       │         │
                     │       │         │
                     ├───────┤Pin 2    │
                     │       │         │
                   5kΩ       │    Pin 3├───► Relay 1 ───► Line 1 (R→L)
                     │       │    Pin 4├───► Relay 2 ───► Line 2 (L→R)
                    GND      │         │
                             │    Pin 5├───► LED 1 (Line 1 blocked)
                             │    Pin 6├───► LED 2 (Line 2 blocked)
                             │    Pin 7├───► LED 3 (Points position)
                             │         │
                             │  5V  GND│
                             └──┬───┬──┘
                                │   │
                             Power  Ground

Tortoise Motor Control:
   12V+ ──┬── DPDT ──┬── Pin 1 (Black)
          │  Switch  │
   GND ───┴─────────┴── Pin 8 (Red)

   (Flip DPDT to reverse polarity and change points)
```

## Additional Notes

- **Tortoise Compatibility**: This design works with Tortoise slow-motion point motors and their built-in 12V bipolar LEDs
- **Voltage Divider**: Essential to bring 12V down to 4V for Arduino safety - do not skip this!
- **Resistor Values**: 10kΩ + 5kΩ is ideal, but 10kΩ + 4.7kΩ also works (gives ~4.2V)
- **Relay Logic**: Active LOW (Arduino pin LOW = relay ON = track power OFF)
- **Alternative Relays**: If using different relay modules, you may need to invert the logic in code
- **DPDT Switch**: Allows manual control of Tortoise and serves as backup if Arduino fails
- **Tortoise LED Sets**: Tortoise has two identical LED terminal sets (pins 2-3 and 6-7) - use either one
- **Emergency Stop**: Consider adding a button wired to call the `emergencyStop()` function
- **Expansion**: Tortoise also has SPDT auxiliary contacts (pins 1-4-5) that can control other accessories
