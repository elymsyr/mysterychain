// The one screen: header with the link, setup steps until done, Find/Feed, quick reactions, activity, message box.
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'character_list.dart';
import 'pet_head.dart';
import 'keychain.dart';
import 'protocol.dart';
import 'settings_page.dart';
import 'theme.dart';

const reactions = ['♥', 'On my way', 'Where are you?', 'Good night', 'Miss you']; // ♥ goes as #heart, the rest as text

String hhmm(DateTime t) => '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}';

class PetPage extends StatefulWidget {
  const PetPage({super.key, required this.keychain});
  final Keychain keychain;

  @override
  State<PetPage> createState() => _PetPageState();
}

class _PetPageState extends State<PetPage> {
  Keychain get k => widget.keychain;
  final input = TextEditingController();
  late final AppLifecycleListener lifecycle;

  @override
  void initState() {
    super.initState();
    lifecycle = AppLifecycleListener(onResume: k.refresh); // back from a settings screen or the pairing dialog
  }

  @override
  void dispose() {
    lifecycle.dispose();
    input.dispose();
    super.dispose();
  }

  Future<void> send() async {
    final text = input.text.trim();
    if (text.isNotEmpty && await k.say(text)) input.clear();
  }

  Future<void> pair() async {
    if (await k.pair() || !mounted) return;
    ScaffoldMessenger.of(context)
        .showSnackBar(const SnackBar(content: Text('Not paired. Is the keychain on and close by?')));
  }

  void openSettings() => Navigator.of(context)
      .push(MaterialPageRoute(builder: (_) => SettingsPage(k)));

  void pickCharacter() => showModalBottomSheet<void>(
        context: context,
        backgroundColor: surface,
        builder: (context) => SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(16, 16, 8, 8),
            child: Column(mainAxisSize: MainAxisSize.min, crossAxisAlignment: CrossAxisAlignment.start, children: [
              const Text('Character', style: TextStyle(fontSize: 13, color: text2)),
              CharacterList(k, onPicked: () => Navigator.pop(context)),
            ]),
          ),
        ),
      );

  @override
  Widget build(BuildContext context) => ListenableBuilder(listenable: k, builder: (context, _) => page(k.connected));

  Widget page(bool connected) => AnnotatedRegion<SystemUiOverlayStyle>(
        value: SystemUiOverlayStyle.light.copyWith(statusBarColor: bg, systemNavigationBarColor: bg),
        child: Scaffold(
          body: SafeArea(
            child: Column(crossAxisAlignment: CrossAxisAlignment.stretch, children: [
              header(connected),
              if (k.stats != null) statsPanel(k.stats!),
              ...setup(),
              const Divider(),
              Padding(
                padding: const EdgeInsets.fromLTRB(16, 14, 16, 0),
                child: k.ringing
                    ? ringPanel()
                    : Row(children: [
                        Expanded(child: findButton(connected)),
                        if (k.stats != null) ...[const SizedBox(width: 8), feedButton(connected)],
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
                        onPressed: connected ? () => k.react(r) : null,
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

  Widget header(bool connected) => Padding(
        padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
        child: Row(children: [
          GestureDetector(
            onTap: connected && k.settings != null ? pickCharacter : null,
            child: Opacity(
              opacity: connected ? 1 : .55,
              child: Container(
                width: 44,
                height: 44,
                alignment: Alignment.center,
                decoration: BoxDecoration(color: const Color(0xFF2A2723), borderRadius: corner),
                child: CustomPaint(size: const Size(36, 34), painter: PetHead(k.settings?.scene ?? 'cat')),
              ),
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text(k.settings == null ? 'MysteryChain' : nameOf(k.settings!),
                  style: const TextStyle(fontSize: 20, fontWeight: FontWeight.w500)),
              const SizedBox(height: 2),
              Row(children: [
                Icon(Icons.circle, size: 8, color: connected ? ok : text3),
                const SizedBox(width: 6),
                Flexible(
                  child: Text(
                      connected && k.synced != null ? '${k.status} · clock synced ${hhmm(k.synced!)}' : k.status,
                      overflow: TextOverflow.ellipsis,
                      style: const TextStyle(fontSize: 13, color: text2)),
                ),
              ]),
            ]),
          ),
          if (k.settings != null)
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

  // What's still missing, in order: the link needs the first three, calls and music need Notification access,
  // and Xiaomi stops background apps unless allowed.
  List<Widget> setup() => [
        if (!k.permitted)
          setupRow(Icons.bluetooth_disabled, 'Allow Nearby devices to reach the keychain', 'Open', k.openAppInfo)
        else if (!k.bluetooth)
          setupRow(Icons.bluetooth_disabled, 'Bluetooth is off', 'Turn on', k.enableBluetooth)
        else if (!k.paired)
          setupRow(Icons.link, 'Pair with your keychain', 'Pair', pair),
        if (!k.access) setupRow(Icons.music_note_outlined, 'Show calls and music on the keychain', 'Allow', k.openAccess),
        if (k.autostart)
          setupRow(Icons.battery_saver_outlined, 'Xiaomi: turn on Autostart, set Battery saver to No restrictions', 'Open',
              () => k.openAppInfo(autostart: true)),
      ];

  Widget setupRow(IconData icon, String text, String action, VoidCallback onTap) => Padding(
        padding: const EdgeInsets.fromLTRB(16, 0, 8, 6),
        child: Row(children: [
          Icon(icon, size: 18, color: text3),
          const SizedBox(width: 10),
          Expanded(child: Text(text, style: const TextStyle(fontSize: 13, color: text2))),
          TextButton(onPressed: onTap, child: Text(action)),
        ]),
      );

  Widget feedButton(bool connected) => OutlinedButton.icon(
        onPressed: connected ? k.feed : null,
        icon: const Icon(Icons.restaurant, size: 18),
        label: const Text('Feed'),
        style: OutlinedButton.styleFrom(minimumSize: const Size(0, 44)),
      );

  Widget findButton(bool connected) => OutlinedButton.icon(
        onPressed: connected ? () => k.find(true) : null,
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
          Icon(Icons.notifications_active_outlined, color: accent, size: 26),
          const SizedBox(width: 12),
          Expanded(
            child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
              Text('Ringing…', style: TextStyle(fontSize: 15, fontWeight: FontWeight.w500, color: accent)),
              const SizedBox(height: 3),
              const Text('Press the keychain button to stop', style: TextStyle(fontSize: 13, color: text2)),
            ]),
          ),
          FilledButton(
            onPressed: () => k.find(false),
            style: FilledButton.styleFrom(
              shape: RoundedRectangleBorder(borderRadius: corner),
              minimumSize: const Size(0, 36),
              padding: const EdgeInsets.symmetric(horizontal: 16),
            ),
            child: const Text('Stop', style: TextStyle(fontWeight: FontWeight.w500)),
          ),
        ]),
      );

  Widget activity() => k.events.isEmpty
      ? const Center(
          child: Text('No events yet.\nPress the button or shake the keychain.',
              textAlign: TextAlign.center, style: TextStyle(color: text3)))
      : ListView.builder(
          itemCount: k.events.length,
          itemBuilder: (_, i) {
            final e = k.events[i];
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
