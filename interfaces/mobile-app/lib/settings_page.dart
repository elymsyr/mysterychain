// Pet settings, kept on the board: every change goes out as `#set ...` at once; the board answers with `settings`.
import 'package:flutter/material.dart';

import 'protocol.dart';
import 'theme.dart';

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
