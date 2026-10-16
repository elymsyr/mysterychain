// MysteryChain: talks to the keychain over BLE Nordic UART.
// Finds the board by name and connects; if the link drops, Android reconnects on its own once in range (autoConnect).
// On connect it sends the phone's time (#time). Text and quick reactions show up in the board's speech bubble,
// Find makes it buzz and blink (#find); the board's button/shake/found events land in the activity list.
// Pet scene (cat): the board reports `stats` and `settings`; the app shows the stats, feeds (#feed) and sets (#set).
// Look: docs/mockups/app_screens.png. Flat dark theme, one accent taken from the cat's fur.
import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

const deviceName = 'MysteryChain';
final uartService = Guid('6E400001-B5A3-F393-E0A9-E50E24DCCA9E');
final rxUuid = Guid('6E400002-B5A3-F393-E0A9-E50E24DCCA9E'); // board RX: phone writes
final txUuid = Guid('6E400003-B5A3-F393-E0A9-E50E24DCCA9E'); // board TX: phone listens

const reactions = ['♥', 'On my way', 'Where are you?', 'Good night', 'Miss you']; // ♥ goes as #heart, the rest as text

const bg = Color(0xFF171614), surface = Color(0xFF201E1B), line = Color(0xFF312E29), rowLine = Color(0xFF221F1C);
const text1 = Color(0xFFEBE6DD), text2 = Color(0xFFA39D92), text3 = Color(0xFF6C675E);
const accent = Color(0xFFE3A043), accentInk = Color(0xFF1D1406), ok = Color(0xFF8BB174), heart = Color(0xFFF08CA0);
final corner = BorderRadius.circular(6);

void main() => runApp(MaterialApp(
      title: 'MysteryChain',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.dark,
        scaffoldBackgroundColor: bg,
        colorScheme: const ColorScheme.dark(primary: accent, onPrimary: accentInk, surface: bg, onSurface: text1),
        dividerTheme: const DividerThemeData(color: line, thickness: 1, space: 1),
        appBarTheme: const AppBarTheme(
            backgroundColor: bg, foregroundColor: text1, elevation: 0, surfaceTintColor: Colors.transparent),
        outlinedButtonTheme: OutlinedButtonThemeData(
          style: OutlinedButton.styleFrom(
            foregroundColor: text1,
            backgroundColor: surface,
            disabledForegroundColor: text3,
            side: const BorderSide(color: line),
            shape: RoundedRectangleBorder(borderRadius: corner),
            minimumSize: const Size(0, 34),
            padding: const EdgeInsets.symmetric(horizontal: 14),
            textStyle: const TextStyle(fontSize: 14, fontWeight: FontWeight.w400),
          ),
        ),
        inputDecorationTheme: InputDecorationTheme(
          filled: true,
          fillColor: surface,
          hintStyle: const TextStyle(color: text3),
          contentPadding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
          enabledBorder: OutlineInputBorder(borderRadius: corner, borderSide: const BorderSide(color: line)),
          disabledBorder: OutlineInputBorder(borderRadius: corner, borderSide: const BorderSide(color: line)),
          focusedBorder: OutlineInputBorder(borderRadius: corner, borderSide: const BorderSide(color: accent)),
        ),
      ),
      home: const PetPage(),
    ));

enum Kind { sent, pet, sys }

typedef PetEvent = ({DateTime at, Kind kind, IconData icon, String text});
typedef Stats = ({int steps, String mood});
typedef Settings = ({bool vib, double shake, int from, int to, String name}); // night from == to: no night window

String hhmm(DateTime t) => '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}';

class PetPage extends StatefulWidget {
  const PetPage({super.key});

  @override
  State<PetPage> createState() => _PetPageState();
}

class _PetPageState extends State<PetPage> {
  String status = 'Waiting for Bluetooth';
  BluetoothDevice? pet;
  BluetoothCharacteristic? rx; // non-null = connected and ready
  DateTime? synced; // when the board last got the time
  bool ringing = false; // Find is running on the board
  Stats? stats; // null: not a pet scene, or nothing heard yet
  Settings? settings;
  final events = <PetEvent>[];
  final input = TextEditingController();
  final subs = <StreamSubscription>[];

  @override
  void initState() {
    super.initState();
    subs.add(FlutterBluePlus.adapterState.listen((s) {
      if (s != BluetoothAdapterState.on) return setState(() => status = 'Bluetooth off');
      if (pet == null) scan();
    }));
    FlutterBluePlus.turnOn().catchError((_) {}); // asks to turn it on if off
  }

  @override
  void dispose() {
    for (final s in subs) {
      s.cancel();
    }
    input.dispose();
    super.dispose();
  }

  void log(Kind kind, IconData icon, String text) =>
      setState(() => events.insert(0, (at: DateTime.now(), kind: kind, icon: icon, text: text)));

  Future<void> scan() async {
    setState(() => status = 'Scanning');
    final sub = FlutterBluePlus.onScanResults.listen((results) {
      if (results.isEmpty || pet != null) return;
      FlutterBluePlus.stopScan();
      attach(results.last.device);
    });
    FlutterBluePlus.cancelWhenScanComplete(sub);
    await FlutterBluePlus.startScan(withNames: [deviceName]);
  }

  void attach(BluetoothDevice d) {
    pet = d;
    setState(() => status = 'Connecting');
    subs.add(d.connectionState.listen((s) {
      if (s == BluetoothConnectionState.connected) {
        setup(d);
        return;
      }
      if (rx != null) log(Kind.sys, Icons.link_off, 'Disconnected');
      setState(() {
        rx = null;
        ringing = false;
        status = 'Searching…';
      });
    }));
    // ponytail: Android autoConnect can be slow (background scan); if it becomes a problem, connect directly + retry loop
    d.connect(license: License.nonprofit, autoConnect: true, mtu: null);
  }

  Future<void> setup(BluetoothDevice d) async {
    try {
      await d.requestMtu(247).catchError((_) => 23); // long messages go in one write (board uses 247 too)
      final uart = (await d.discoverServices()).firstWhere((s) => s.uuid == uartService);
      final tx = uart.characteristics.firstWhere((c) => c.uuid == txUuid);
      d.cancelWhenDisconnected(tx.onValueReceived.listen((v) {
        for (final text in utf8.decode(v, allowMalformed: true).split('\n')) {
          if (text.trim().isNotEmpty) onLine(text.trim());
        }
      }));
      await tx.setNotifyValue(true);
      setState(() {
        rx = uart.characteristics.firstWhere((c) => c.uuid == rxUuid);
        status = 'Connected';
      });
      log(Kind.sys, Icons.link, 'Connected');
      final now = DateTime.now();
      if (await write('#time ${now.millisecondsSinceEpoch ~/ 1000} ${now.timeZoneOffset.inMinutes}')) {
        setState(() => synced = now);
        log(Kind.sys, Icons.schedule, 'Clock synced');
      }
    } catch (e) {
      setState(() => status = 'Setup error'); // retried on the next reconnect
      log(Kind.sys, Icons.error_outline, 'Setup error: $e');
    }
  }

  // One line from the board (interfaces/watch/README.md#protocol). stats/settings update the screen, the rest is logged.
  void onLine(String text) {
    final st = RegExp(r'^stats (\d+) (\w+)$').firstMatch(text);
    final se = RegExp(r'^settings vib=(\d) shake=([\d.]+) night=(\d+)-(\d+) name=(.*)$').firstMatch(text);
    final game = RegExp(r'^game (\d+) best (\d+)$').firstMatch(text);
    int n(RegExpMatch m, int i) => int.parse(m[i]!);
    if (st != null) {
      setState(() => stats = (steps: n(st, 1), mood: st[2]!));
    } else if (se != null) {
      setState(() => settings =
          (vib: se[1] == '1', shake: double.parse(se[2]!), from: n(se, 3), to: n(se, 4), name: se[5]!));
    } else if (game != null) {
      log(Kind.pet, Icons.sports_esports_outlined, 'Game: ${game[1]} (best ${game[2]})');
    } else if (text == 'fed') {
      log(Kind.pet, Icons.restaurant, 'Ate');
    } else if (text == 'full') {
      log(Kind.pet, Icons.restaurant, 'Not hungry');
    } else {
      if (text == 'found' || text == 'find timeout') setState(() => ringing = false);
      log(Kind.pet, Icons.pets, text);
    }
  }

  // One message to the board: plain text goes to the bubble, '#...' is a command (interfaces/watch/firmware).
  Future<bool> write(String s) async {
    if (rx == null) return false;
    try {
      await rx!.write(utf8.encode(s));
      return true;
    } catch (e) {
      if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Send failed: $e')));
      return false;
    }
  }

  Future<void> send() async {
    final text = input.text.trim();
    if (text.isEmpty) return;
    if (await write(text)) {
      log(Kind.sent, Icons.north_east, text);
      input.clear();
    }
  }

  Future<void> react(String r) async {
    if (await write(r == '♥' ? '#heart' : r)) log(Kind.sent, Icons.north_east, r);
  }

  Future<void> feed() async {
    if (await write('#feed')) log(Kind.sent, Icons.north_east, 'Feed');
  }

  void openSettings() => Navigator.of(context)
      .push(MaterialPageRoute(builder: (_) => SettingsPage(initial: settings!, send: write)));

  Future<void> find(bool on) async {
    if (!await write(on ? '#find' : '#find 0')) return;
    setState(() => ringing = on);
    log(Kind.sent, Icons.north_east, on ? 'Find' : 'Stop');
  }

  @override
  Widget build(BuildContext context) {
    final connected = rx != null;
    return AnnotatedRegion<SystemUiOverlayStyle>(
      value: SystemUiOverlayStyle.light.copyWith(statusBarColor: bg, systemNavigationBarColor: bg),
      child: Scaffold(
        body: SafeArea(
          child: Column(crossAxisAlignment: CrossAxisAlignment.stretch, children: [
            header(connected),
            if (stats != null) statsPanel(stats!),
            const Divider(),
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 14, 16, 0),
              child: ringing
                  ? ringPanel()
                  : Row(children: [
                      Expanded(child: findButton(connected)),
                      if (stats != null) ...[const SizedBox(width: 8), feedButton(connected)],
                    ]),
            ),
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 16, 16, 14),
              child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
                const Text('Quick reactions', style: TextStyle(fontSize: 13, color: text2)),
                const SizedBox(height: 10),
                Wrap(spacing: 8, runSpacing: 8, children: [
                  for (final r in reactions)
                    OutlinedButton(
                      onPressed: connected ? () => react(r) : null,
                      style: r == '♥' ? OutlinedButton.styleFrom(iconColor: heart, disabledIconColor: text3) : null,
                      child: r == '♥' ? const Icon(Icons.favorite, size: 18) : Text(r), // icon: Roboto has no ♥ glyph
                    ),
                ]),
              ]),
            ),
            const Divider(),
            Expanded(child: activity()),
            const Divider(),
            composer(connected),
          ]),
        ),
      ),
    );
  }

  Widget header(bool connected) => Padding(
        padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
        child: Row(children: [
          Opacity(
            opacity: connected ? 1 : .55,
            child: Container(
              width: 44,
              height: 44,
              alignment: Alignment.center,
              decoration: BoxDecoration(color: const Color(0xFF2A2723), borderRadius: corner),
              child: const CustomPaint(size: Size(36, 30), painter: CatHead()),
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text(settings?.name ?? 'MysteryChain', style: const TextStyle(fontSize: 20, fontWeight: FontWeight.w500)),
              const SizedBox(height: 2),
              Row(children: [
                Icon(Icons.circle, size: 8, color: connected ? ok : text3),
                const SizedBox(width: 6),
                Flexible(
                  child: Text(connected && synced != null ? '$status · clock synced ${hhmm(synced!)}' : status,
                      overflow: TextOverflow.ellipsis, style: const TextStyle(fontSize: 13, color: text2)),
                ),
              ]),
            ]),
          ),
          if (settings != null)
            IconButton(
              onPressed: connected ? openSettings : null,
              icon: const Icon(Icons.settings_outlined),
              color: text2,
              tooltip: 'Settings',
            ),
        ]),
      );

  // Under the header: today's steps and mood (the pet's 0-100 stats stay on the board).
  Widget statsPanel(Stats s) => Padding(
        padding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
        child: Text('${s.steps} steps today · ${s.mood}', style: const TextStyle(fontSize: 13, color: text2)),
      );

  Widget feedButton(bool connected) => OutlinedButton.icon(
        onPressed: connected ? feed : null,
        icon: const Icon(Icons.restaurant, size: 18),
        label: const Text('Feed'),
        style: OutlinedButton.styleFrom(minimumSize: const Size(0, 44)),
      );

  Widget findButton(bool connected) => OutlinedButton.icon(
        onPressed: connected ? () => find(true) : null,
        icon: const Icon(Icons.notifications_active_outlined, size: 20),
        label: const Text('Find keychain'),
        style: OutlinedButton.styleFrom(
          foregroundColor: accent,
          backgroundColor: Colors.transparent,
          side: BorderSide(color: connected ? accent : line),
          minimumSize: const Size.fromHeight(44),
          textStyle: const TextStyle(fontSize: 15, fontWeight: FontWeight.w500),
        ),
      );

  Widget ringPanel() => Container(
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: const Color(0xFF2C2418),
          border: Border.all(color: const Color(0xFF5A4423)),
          borderRadius: corner,
        ),
        child: Row(children: [
          const Icon(Icons.notifications_active_outlined, color: accent, size: 26),
          const SizedBox(width: 12),
          const Expanded(
            child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text('Ringing…', style: TextStyle(fontSize: 15, fontWeight: FontWeight.w500, color: accent)),
              SizedBox(height: 3),
              Text('Press the keychain button to stop', style: TextStyle(fontSize: 13, color: text2)),
            ]),
          ),
          FilledButton(
            onPressed: () => find(false),
            style: FilledButton.styleFrom(
              shape: RoundedRectangleBorder(borderRadius: corner),
              minimumSize: const Size(0, 36),
              padding: const EdgeInsets.symmetric(horizontal: 16),
            ),
            child: const Text('Stop', style: TextStyle(fontWeight: FontWeight.w500)),
          ),
        ]),
      );

  Widget activity() => events.isEmpty
      ? const Center(
          child: Text('No events yet.\nPress the button or shake the keychain.',
              textAlign: TextAlign.center, style: TextStyle(color: text3)))
      : ListView.builder(
          itemCount: events.length,
          itemBuilder: (_, i) {
            final e = events[i];
            final isHeart = e.text == '♥';
            return Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
              decoration: const BoxDecoration(border: Border(bottom: BorderSide(color: rowLine))),
              child: Row(children: [
                Icon(e.icon, size: 18, color: e.kind == Kind.sent ? accent : text3),
                const SizedBox(width: 12),
                Expanded(
                  child: isHeart
                      ? const Align(alignment: Alignment.centerLeft, child: Icon(Icons.favorite, size: 16, color: heart))
                      : Text(e.text, style: TextStyle(fontSize: 14, color: e.kind == Kind.sys ? text2 : text1)),
                ),
                Text(hhmm(e.at), style: const TextStyle(fontSize: 12, color: text3)),
              ]),
            );
          },
        );

  Widget composer(bool connected) => Padding(
        padding: const EdgeInsets.fromLTRB(16, 10, 12, 10),
        child: Row(children: [
          Expanded(
            child: TextField(
              controller: input,
              enabled: connected,
              maxLength: 60, // board screen holds 63 characters
              textInputAction: TextInputAction.send,
              onSubmitted: (_) => send(),
              decoration: InputDecoration(
                hintText: connected ? 'Message the pet' : 'Keychain not connected',
                counterText: '',
                isDense: true,
              ),
            ),
          ),
          const SizedBox(width: 8),
          IconButton.filled(
            onPressed: connected ? send : null,
            icon: const Icon(Icons.send),
            style: IconButton.styleFrom(
              shape: RoundedRectangleBorder(borderRadius: corner),
              fixedSize: const Size(44, 44),
              disabledBackgroundColor: surface,
              disabledForegroundColor: text3,
            ),
          ),
        ]),
      );
}

// Pet settings, kept on the board: every change goes out as `#set ...` at once; the board answers with `settings`.
class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key, required this.initial, required this.send});
  final Settings initial;
  final Future<bool> Function(String) send;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late Settings s = widget.initial;
  late final name = TextEditingController(text: widget.initial.name);

  @override
  void dispose() {
    name.dispose();
    super.dispose();
  }

  void set(String cmd, Settings next) {
    setState(() => s = next);
    widget.send('#set $cmd');
  }

  void saveName() {
    final v = name.text.trim();
    if (v.isNotEmpty && v != s.name) set('name $v', (vib: s.vib, shake: s.shake, from: s.from, to: s.to, name: v));
    FocusScope.of(context).unfocus();
  }

  void night(int from, int to) => set('night $from $to', (vib: s.vib, shake: s.shake, from: from, to: to, name: s.name));

  Widget hour(int v, void Function(int) on) => DropdownButton<int>(
        value: v,
        dropdownColor: surface,
        underline: const SizedBox(),
        items: [for (var h = 0; h < 24; h++) DropdownMenuItem(value: h, child: Text('${h.toString().padLeft(2, '0')}:00'))],
        onChanged: (h) => on(h!),
      );

  @override
  Widget build(BuildContext context) {
    const label = TextStyle(fontSize: 13, color: text2), hint = TextStyle(fontSize: 13, color: text3);
    final nightOn = s.from != s.to;
    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: ListView(padding: const EdgeInsets.fromLTRB(16, 8, 16, 24), children: [
        const Text('Name', style: label),
        const SizedBox(height: 8),
        TextField(
          controller: name,
          maxLength: 12, // the board keeps 12 ASCII characters
          textInputAction: TextInputAction.done,
          onSubmitted: (_) => saveName(),
          decoration: InputDecoration(
            counterText: '',
            isDense: true,
            suffixIcon: IconButton(icon: const Icon(Icons.check, size: 20), onPressed: saveName),
          ),
        ),
        const SizedBox(height: 16),
        const Divider(),
        SwitchListTile(
          contentPadding: EdgeInsets.zero,
          title: const Text('Vibration'),
          subtitle: const Text('Find always buzzes', style: hint),
          value: s.vib,
          onChanged: (v) => set('vib ${v ? 1 : 0}', (vib: v, shake: s.shake, from: s.from, to: s.to, name: s.name)),
        ),
        const Divider(),
        const SizedBox(height: 12),
        Text('Shake threshold: ${s.shake.toStringAsFixed(2)} g', style: label),
        const Text('Lower reacts to gentler shakes', style: hint),
        Slider(
          value: s.shake.clamp(.3, 1),
          min: .3,
          max: 1,
          divisions: 14,
          label: s.shake.toStringAsFixed(2),
          onChanged: (v) => setState(() => s = (vib: s.vib, shake: v, from: s.from, to: s.to, name: s.name)),
          onChangeEnd: (v) => set('shake ${v.toStringAsFixed(2)}', s),
        ),
        const Divider(),
        SwitchListTile(
          contentPadding: EdgeInsets.zero,
          title: const Text('Night'),
          subtitle: const Text('The pet sleeps and the motor is quiet', style: hint),
          value: nightOn,
          onChanged: (v) => v ? night(23, 7) : night(0, 0),
        ),
        if (nightOn)
          Row(children: [
            const Text('From', style: label),
            const SizedBox(width: 12),
            hour(s.from, (h) => night(h, s.to)),
            const SizedBox(width: 24),
            const Text('to', style: label),
            const SizedBox(width: 12),
            hour(s.to, (h) => night(s.from, h)),
          ]),
      ]),
    );
  }
}

// The cat's head, eyes open (HEAD[0] in interfaces/watch/firmware/cat.h): K ink, O fur, W cream, P pink.
class CatHead extends CustomPainter {
  const CatHead();

  static const rows = [
    '.K........K.', 'KPK......KPK', 'KPPK....KPPK', 'KOOKKKKKKOOK', 'KOOOOOOOOOOK',
    'KOKWOOOOKWOK', 'KOKKOOOOKKOK', 'KOOOWPPWOOOK', '.KOWWWWWWOK.', '..KKKKKKKK..',
  ];
  static const colors = {
    'K': Color(0xFF1E1216), 'O': Color(0xFFFF9628), 'W': Color(0xFFFFECD2), 'P': Color(0xFFFF8CA0),
  };

  @override
  void paint(Canvas canvas, Size size) {
    final px = size.width / rows[0].length;
    for (var y = 0; y < rows.length; y++) {
      for (var x = 0; x < rows[y].length; x++) {
        final c = colors[rows[y][x]];
        if (c != null) {
          canvas.drawRect(Rect.fromLTWH(x * px, y * px, px, px), Paint()..color = c..isAntiAlias = false);
        }
      }
    }
  }

  @override
  bool shouldRepaint(CatHead oldDelegate) => false;
}
