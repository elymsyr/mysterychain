# Mobile app

Flutter companion app (Android). Connects to the keychain over BLE (Nordic UART service, device name `MysteryChain`).

- Finds and connects to the keychain; reconnects automatically when it comes back in range.
- Sets the keychain's clock on every connect.
- **Find keychain:** the keychain buzzes and blinks until its button is pressed, Stop is tapped, or 60 s pass.
- **Quick reactions:** ♥ (hearts on the keychain), On my way, Where are you?, Good night, Miss you. The list is `reactions` in `lib/main.dart`.
- Sends text: it scrolls in the pet's speech bubble and the motor buzzes twice.
- Activity list: what was sent, events from the keychain (`button N`, `shake N (x.xx g)`, `found`), connection changes.
- **Cat pet** (keychain running the cat scene): the pet's name in the header, today's steps and mood (the 0–100 stats stay on the keychain); **Feed** next to Find; game results in the activity list. The gear opens settings kept on the keychain: name, vibration, shake threshold, night window. Other scenes don't report stats, so these stay hidden.

Dark theme only for now. Design: [docs/mockups/app_screens.png](../../docs/mockups/app_screens.png), pet: [docs/mockups/app_pet_screens.png](../../docs/mockups/app_pet_screens.png).

Tested on a Xiaomi Redmi Note 12. iOS is not set up.

## Run

```bash
flutter pub get
flutter run            # phone connected over USB, developer mode on
flutter build apk      # build/app/outputs/flutter-apk/app-release.apk
```

Bluetooth permission is requested on first start. Location is not needed on Android 12+.

## Protocol

Plain UTF-8 text over Nordic UART, one message per write (MTU 247). The phone writes to RX `6E400002-…` and listens to TX `6E400003-…` (notify). Plain text goes to the bubble; messages starting with `#` are commands (`#time`, `#find`, `#heart`, and for the cat `#feed`, `#stats`, `#set`). A notification may carry several lines; the app splits them. Full list: [watch protocol](../watch/README.md#protocol).
