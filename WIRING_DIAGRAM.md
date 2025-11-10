# Railway Crossover Control - Wiring Diagram

## Component List

### Required Components:
- 1x Arduino (Uno, Nano, or similar)
- 2x Relay modules (5V, optocoupler isolated recommended)
- 1x Tortoise point motor (with built-in 12V bipolar LED and current-limiting resistor)
- 3x LEDs (optional, for visual feedback on control panel)
- 3x 220Ω resistors (for optional status LEDs)
- 12V power supply for Tortoise motor
- 5V power supply for Arduino (USB or 7-12V DC via barrel jack)
- Connecting wires

### Optional Components:
- 1x DPDT switch (for manual points control)
- 1x Emergency stop button

## Pin Connections

### Input (Points Position Sensor - Tortoise LED Direct Connection)
```
Tortoise Bipolar LED → Arduino Pin 2

Connection:
  Sense after the Tortoise's internal current-limiting resistor
  Connect directly to Arduino Pin 2 (no external voltage divider needed)

  12V+ → Tortoise Resistor → LED → Sense Point → 12V-
                                        ↓
                                  Arduino Pin 2

How it works:
- Red LED (one polarity): Forward voltage ~1.9V → Pin 2 reads this directly (LOW/borderline)
- Green LED (reversed polarity): Forward voltage ~2.1V → Pin 2 reads this directly (HIGH)
- Both voltages are inherently safe for Arduino (< 5V)
- The LED forward voltage drop provides natural voltage limiting
- No external resistor divider required

Typical Arduino thresholds:
- Input LOW: < 1.5V (typical)
- Input HIGH: > 3.0V (typical)
- The 1.9V and 2.1V readings are close, so test to confirm reliable detection
- If readings are unreliable, you may need a small external circuit (see troubleshooting)
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

## Tortoise LED Voltage Sensing

The Tortoise motor's built-in bipolar LED provides different forward voltages for position detection:

```
LED voltage indicates points direction:

~1.9V (Red LED) = Points favor Line 1 (R→L) - Pin 2 may read LOW (borderline threshold)
~2.1V (Green LED) = Points favor Line 2 (L→R) - Pin 2 may read HIGH (borderline threshold)

Wiring Notes:
- Connect sense wire to Tortoise LED circuit after the internal current-limiting resistor
- Tap the connection point between LED and ground return
- Connect directly to Arduino Pin 2 (no external components needed)
- Polarity/color depends on how you wire the Tortoise motor power
- If readings are inverted, swap the Tortoise motor power wires (pins 1 & 8)

Important Considerations:
- Both voltages (1.9V and 2.1V) are in the Arduino's "gray zone" for digital inputs
- Standard Arduino HIGH threshold: ~3.0V (typ), LOW threshold: ~1.5V (typ)
- The 200mV difference may not be reliably detected as HIGH vs LOW
- Test thoroughly with Serial Monitor to confirm your Arduino can distinguish these levels
- If unreliable, consider adding a comparator or voltage level shifter

Alternative Solutions if Detection is Unreliable:
1. Use an analog input (A0-A5) and read actual voltage values
2. Add a comparator circuit with 2.0V reference
3. Add a simple op-amp buffer/level shifter
4. Use the Tortoise auxiliary contacts (SPDT pins 1-4-5) instead
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

1. **Verify** you're sensing AFTER the LED (at LED forward voltage ~2V, NOT before LED at 12V)
2. **Test** sense point voltage with multimeter first (~1.9-2.1V expected, never >5V)
3. **Never** connect the 12V rail or pre-resistor point directly to Arduino pins
4. **Never** connect track power directly to Arduino pins
5. **Always** use relays to switch track power
6. **Use** optocoupler-isolated relays to protect the Arduino
7. **Ensure** relay current rating exceeds your maximum track current
8. **Add** fuses to track power circuits (1-3A recommended)
9. **Test** voltage levels thoroughly before connecting to Arduino
10. **Double-check** all wiring before powering on

## Testing Procedure

1. **LED Voltage Test (CRITICAL - Do this first!)**
   - Power up the Tortoise motor circuit (do NOT connect to Arduino yet)
   - Use multimeter to measure voltage at your sense point (after LED, before ground)
   - Toggle Tortoise between positions with DPDT switch
   - Verify readings: ~1.9V (one position) and ~2.1V (other position)
   - **CRITICAL**: If you see voltages >5V, you're at the wrong point! Find the LED side.
   - **DO NOT** connect to Arduino until voltage is verified safe (1.5-2.5V range)

2. **Initial Test (No Track Power)**
   - Upload code to Arduino
   - Connect sense wire to Arduino Pin 2
   - Open Serial Monitor (9600 baud)
   - Manually toggle Tortoise with DPDT switch
   - Verify serial output shows points position changes
   - **NOTE**: If position detection is unreliable/intermittent, see "Alternative Solutions" section
   - If inverted (but detection works), swap Tortoise motor wires (pins 1 & 8)

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
| Arduino damage/erratic | 12V connected to pin (wrong sense point) | Verify sense point with multimeter, should be ~2V not 12V |
| No position changes detected | Both LEDs read same (1.9V/2.1V too close) | Use analog input A0 instead, or add comparator circuit |
| Points position inverted | LED color mapping reversed | Swap Tortoise motor wires (pins 1 & 8) |
| Always reads HIGH or LOW | Sense point disconnected | Check connection with multimeter, verify continuity |
| Relays always on/off | Wrong relay type (active HIGH vs LOW) | Check relay module type, may need to invert logic |
| Intermittent/unreliable readings | Voltage too close to threshold | Switch to analog input or add level shifter |
| Tortoise not moving | No power to motor | Check 12V supply and DPDT switch wiring |
| Arduino resets | Power supply insufficient | Use separate power supply for Arduino |
| Relays chattering | Debounce time too short | Increase DEBOUNCE_DELAY in code |
| Reads >5V at sense point | Wrong tap point (before LED) | Move sense point to after LED (lower voltage side) |

## Schematic Diagram (ASCII)

```
   Tortoise Motor Circuit      Arduino                Track Power Control

   12V+ ──┬── DPDT ──┬── Pin 1 (Black)
          │  Switch  │   Tortoise
   GND ───┴─────────┴── Pin 8 (Red)

   Internal LED Circuit:

   12V ───► [Resistor] ───► [Bipolar LED] ───┬───► GND
                                              │
                                    Sense Point (1.9-2.1V)
                                              │
                         Arduino              │
                       ┌─────────┐            │
                       │         │            │
                       │    Pin 2├────────────┘
                       │         │
                       │    Pin 3├───► Relay 1 ───► Line 1 (R→L) Track Section
                       │    Pin 4├───► Relay 2 ───► Line 2 (L→R) Track Section
                       │         │
                       │    Pin 5├───► LED 1 (Line 1 blocked indicator)
                       │    Pin 6├───► LED 2 (Line 2 blocked indicator)
                       │    Pin 7├───► LED 3 (Points position indicator)
                       │         │
                       │  5V  GND│
                       └──┬───┬──┘
                          │   │
                       Power  Ground

Key Points:
- Sense point taps between LED and ground (after current-limiting resistor)
- No external voltage divider needed
- LED forward voltage (1.9-2.1V) is naturally Arduino-safe
- DPDT switch reverses polarity to Tortoise, changing points direction
```

## Additional Notes

- **Tortoise Compatibility**: This design works with Tortoise slow-motion point motors and their built-in 12V bipolar LEDs
- **Simplified Sensing**: By sensing at the LED (after current-limiting resistor), no external voltage divider is needed
- **Voltage Levels**: LED forward voltages (1.9V red, 2.1V green) are naturally Arduino-safe
- **Detection Reliability**: The 200mV difference between red/green may be marginal for reliable digital input detection
- **Analog Alternative**: If digital detection is unreliable, change to analog input (analogRead on A0-A5)
- **Relay Logic**: Active LOW (Arduino pin LOW = relay ON = track power OFF)
- **Alternative Relays**: If using different relay modules, you may need to invert the logic in code
- **DPDT Switch**: Allows manual control of Tortoise and serves as backup if Arduino fails
- **Tortoise LED Sets**: Tortoise has two identical LED terminal sets (pins 2-3 and 6-7) - use either one
- **Emergency Stop**: Consider adding a button wired to call the `emergencyStop()` function
- **Expansion**: Tortoise also has SPDT auxiliary contacts (pins 1-4-5) that can control other accessories
- **Alternative Sensor**: If LED sensing proves unreliable, consider using the Tortoise SPDT contacts instead
