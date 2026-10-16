// The app's view of the keychain. The link itself, the one Bluetooth connection to the board, is Android's
// KeychainService (android/.../KeychainService.kt): it lives on with the app closed, sets the clock on every connect and
// carries calls and music. This mirrors its state, keeps what the screen shows (stats, settings, activity) and sends
// through it. Channels: android/.../MainActivity.kt.
import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'protocol.dart';

enum Kind { sent, pet, sys }

typedef PetEvent = ({DateTime at, Kind kind, IconData icon, String text});

class Keychain extends ChangeNotifier {
  static const _link = MethodChannel('mysterychain/link');
  static const _events = EventChannel('mysterychain/events');

  Map<Object?, Object?> _s = {}; // MainActivity "status", updated by link events
  StreamSubscription? _sub;
  Stats? stats; // null: not a pet scene, or nothing heard yet
  Settings? settings;
  bool ringing = false; // Find is running on the board
  final events = <PetEvent>[];

  // Setup steps; true until checked so nothing flashes
  bool get permitted => _s['permitted'] != false;
  bool get bluetooth => _s['bluetooth'] != false;
  bool get paired => _s['paired'] != false;
  bool get access => _s['access'] != false; // Notification access: calls and music
  bool get autostart => _s['autostart'] == true; // Xiaomi: Autostart and Battery saver not opened yet
  bool get connected => _s['connected'] == true;
  DateTime? get synced => switch (_s['synced']) {
        final int ms when ms > 0 => DateTime.fromMillisecondsSinceEpoch(ms),
        _ => null,
      };

  String get status => !permitted
      ? 'Bluetooth permission needed'
      : !bluetooth
          ? 'Bluetooth off'
          : !paired
              ? 'Not paired'
              : connected
                  ? 'Connected'
                  : 'Searching…';

  void start() {
    _sub = _events.receiveBroadcastStream().listen((e) {
      final m = e as Map;
      if (m['line'] case final String text) {
        onLine(text);
      } else {
        _update(m);
      }
    });
    refresh();
  }

  @override
  void dispose() {
    _sub?.cancel();
    super.dispose();
  }

  Future<void> refresh() async => _update(await _link.invokeMethod<Map>('status') ?? {});

  void _update(Map m) {
    final was = _s.isEmpty ? null : connected; // the first status isn't a change
    _s = {..._s, ...m};
    if (was == false && connected) {
      log(Kind.sys, Icons.link, 'Connected');
      log(Kind.sys, Icons.schedule, 'Clock synced');
    } else if (was == true && !connected) {
      ringing = false;
      log(Kind.sys, Icons.link_off, 'Disconnected');
    }
    notifyListeners();
  }

  void log(Kind kind, IconData icon, String text) {
    events.insert(0, (at: DateTime.now(), kind: kind, icon: icon, text: text));
    notifyListeners();
  }

  // One line from the board. stats/settings update the screen, the rest is logged.
  void onLine(String text) {
    if (parseStats(text) case final s?) {
      stats = s;
    } else if (parseSettings(text) case final s?) {
      settings = s;
    } else if (parseGame(text) case final g?) {
      log(Kind.pet, Icons.sports_esports_outlined, 'Game: ${g.score} (best ${g.best})');
    } else if (text == 'fed') {
      log(Kind.pet, Icons.restaurant, 'Ate');
    } else if (text == 'full') {
      log(Kind.pet, Icons.restaurant, 'Not hungry');
    } else {
      if (text == 'found' || text == 'find timeout') ringing = false;
      log(Kind.pet, Icons.pets, text);
    }
    notifyListeners();
  }

  // One message to the board: plain text goes to the bubble, '#...' is a command. false: not connected.
  Future<bool> send(String s) async => await _link.invokeMethod<bool>('send', s) ?? false;

  // A setting: shown at once, the board confirms with its `settings` line.
  Future<bool> set(String cmd, Settings next) {
    settings = next;
    notifyListeners();
    return send('#set $cmd');
  }

  Future<bool> say(String text) async {
    final sent = await send(text);
    if (sent) log(Kind.sent, Icons.north_east, text);
    return sent;
  }

  Future<void> react(String r) async {
    if (await send(r == '♥' ? '#heart' : r)) log(Kind.sent, Icons.north_east, r);
  }

  Future<void> feed() async {
    if (await send('#feed')) log(Kind.sent, Icons.north_east, 'Feed');
  }

  Future<void> find(bool on) async {
    if (!await send(on ? '#find' : '#find 0')) return;
    ringing = on;
    log(Kind.sent, Icons.north_east, on ? 'Find' : 'Stop');
  }

  // The system's pairing dialog; false: cancelled or no keychain found
  Future<bool> pair() async {
    final ok = await _link.invokeMethod<bool>('pair') ?? false;
    await refresh();
    return ok;
  }

  void openAccess() => _link.invokeMethod('openAccess');
  void openAppInfo({bool autostart = false}) => _link.invokeMethod('openAppInfo', autostart);
  void enableBluetooth() => _link.invokeMethod('enableBluetooth');
}
