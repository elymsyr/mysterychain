// MysteryChain: the keychain's companion app (Android). The Bluetooth link is Android's KeychainService
// (android/.../KeychainService.kt), so it keeps running with the app closed; the app shows it and talks through it
// (keychain.dart). Text and quick reactions show up in the board's speech bubble, Find makes it buzz and blink (#find),
// the board's button/shake/found events land in the activity list. Pet scene (cat): the board reports `stats` and
// `settings`; the app shows them, feeds (#feed) and sets (#set). Calls and the playing song: Bridge.kt.
import 'package:flutter/material.dart';

import 'keychain.dart';
import 'pet_page.dart';
import 'theme.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized(); // the channels are used before runApp
  final keychain = Keychain()..start();
  runApp(MaterialApp(
    title: 'MysteryChain',
    debugShowCheckedModeBanner: false,
    theme: theme,
    home: PetPage(keychain: keychain),
  ));
}
