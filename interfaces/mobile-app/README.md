# Mobile app

Flutter companion app (Android). Connects to the keychain over BLE (Nordic UART service, device name `MysteryChain`).

- Finds and connects to the keychain; reconnects automatically when it comes back in range.
- Sends text: it scrolls in the pet's speech bubble and the motor buzzes twice.
- Lists events from the keychain (`button N`, `shake N (x.xx g)`, connection changes).

Tested on a Xiaomi Redmi Note 12. iOS is not set up.

## Run

```bash
flutter pub get
flutter run            # phone connected over USB, developer mode on
flutter build apk      # build/app/outputs/flutter-apk/app-release.apk
```

Bluetooth permission is requested on first start. Location is not needed on Android 12+.

## Protocol

Plain UTF-8 text over Nordic UART, one message per write (MTU 247).

| Direction | Characteristic | Content |
| --- | --- | --- |
| Phone → keychain | RX `6E400002-…` | Any text, max 63 characters on the device. Turkish letters are folded to ASCII, other non-ASCII (emoji) shows as `?`. |
| Keychain → phone | TX `6E400003-…` (notify) | `button N`, `shake N (x.xx g)` |

A typed message protocol (clock, weather, settings, notifications) is planned; see [docs/roadmap.md](../../docs/roadmap.md).
