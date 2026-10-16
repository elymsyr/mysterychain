// Pet settings, kept on the board: every change goes out as `#set ...` at once; the board answers with `settings`.
import 'package:flutter/material.dart';

import 'character_list.dart';
import 'keychain.dart';
import 'protocol.dart';
import 'theme.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage(this.k, {super.key});
  final Keychain k;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late Settings s = widget.k.settings!;

  void set(String cmd, Settings next) {
    setState(() => s = next);
    widget.k.set(cmd, next);
  }

  void night(int from, int to) => set('night $from $to', (vib: s.vib, shake: s.shake, from: from, to: to, scene: live.scene, names: live.names));

  Widget hour(int v, void Function(int) on) => DropdownButton<int>(
        value: v,
        dropdownColor: surface,
        underline: const SizedBox(),
        items: [for (var h = 0; h < 24; h++) DropdownMenuItem(value: h, child: Text('${h.toString().padLeft(2, '0')}:00'))],
        onChanged: (h) => on(h!),
      );

  Settings get live => widget.k.settings ?? s; // the board's latest look and names: a change here never undoes a pick

  @override
  Widget build(BuildContext context) {
    const label = TextStyle(fontSize: 13, color: text2), hint = TextStyle(fontSize: 13, color: text3);
    final nightOn = s.from != s.to;
    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: ListView(padding: const EdgeInsets.fromLTRB(16, 8, 16, 24), children: [
        const Text('Character', style: label),
        const SizedBox(height: 8),
        CharacterList(widget.k),
        const Text('Each has its own name; stats and high score are shared', style: hint),
        const SizedBox(height: 16),
        const Divider(),
        SwitchListTile(
          contentPadding: EdgeInsets.zero,
          title: const Text('Vibration'),
          subtitle: const Text('Find always buzzes', style: hint),
          value: s.vib,
          onChanged: (v) => set('vib ${v ? 1 : 0}', (vib: v, shake: s.shake, from: s.from, to: s.to, scene: live.scene, names: live.names)),
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
          onChanged: (v) => setState(() => s = (vib: s.vib, shake: v, from: s.from, to: s.to, scene: live.scene, names: live.names)),
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
