# Corridor Lighting Control for Proteus 8

## Wiring (ATMEGA328P Proteus component)

| Signal | ATmega328P port | Proteus U1 pin | Arduino alias |
|---|---|---:|---|
| LDR divider output | PC0/ADC0 | 23 | A0 |
| Motion detector (`LOGICSTATE`) | PD2 | 32 | D2 |
| Button to GND | PD4 | 2 | D4 |
| PWM to MOSFET gate via 220 ohms | PB1/OC1A | 13 | D9 |

Supply U1 at 5 V and use common GND with the 12 V LED load.

## Build HEX without Arduino IDE (GitHub Actions)

1. Create an empty GitHub repository. You can choose public visibility for easiest access to Actions artifacts on a free account.
2. Upload the **contents** of this package, keeping both `Corridor_Lighting_Control/Corridor_Lighting_Control.ino` and `.github/workflows/build-hex.yml` in the correct folders.
3. Open `Actions` -> `Compile Proteus HEX` -> `Run workflow` (or allow the upload-triggered build to complete).
4. Open the successful workflow run -> `Artifacts` -> download `Corridor_Lighting_Control-HEX`.
5. Extract the downloaded ZIP. In Proteus, open ATMEGA328P properties and set `Program File` to `Corridor_Lighting_Control.ino.hex`.
6. Set the MCU `Clock Frequency` to `16MHz`; use an external 16 MHz crystal in your schematic.
7. Start simulation; change the LDR light level and toggle `LOGICSTATE`.

Note: a GitHub-hosted runner needs to reach Arduino's core-package download servers. This approach normally avoids a **local** HTTP 403 restriction but cannot guarantee that every runner will have network access.

## Operation

- Daytime: brightness 0% in AUTO.
- Dark corridor, no motion: 20% standby brightness.
- Dark corridor with motion: 100% brightness.
- 30 seconds after last motion: transition back to 20%.
- Momentary button cycles AUTO -> ALWAYS_ON -> ALWAYS_OFF -> AUTO.
- Brightness ramps smoothly, with software hysteresis around the light threshold.

This is a demonstration model for educational use, not a real-world safety-rated emergency lighting controller.
