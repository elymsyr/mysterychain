// The link's on/off switch (the phone), then the pet settings, kept on the board: every change goes out as `#set ...`
// at once; the board answers with `settings`. Pet settings show only once the board has sent them.
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
  late Settings s = widget.k.settings!; // late: read only once the board's settings are in

  void set(String cmd, Settings next) {
    setState(() => s = next);
    widget.k.set(cmd, next);
  }

  void night(int from, int to) => set('night $from $to', (vib: s.vib, shake: s.shake, from: from, to: to, screen: s.screen, wake: s.wake, scene: live.scene, names: live.names));

  void screen(int v) => set('screen $v', (vib: s.vib, shake: s.shake, from: s.from, to: s.to, screen: v, wake: s.wake, scene: live.scene, names: live.names));

  void wake(int bit, bool on) {
    final w = on ? s.wake | bit : s.wake & ~bit;
    set('wake $w', (vib: s.vib, shake: s.shake, from: s.from, to: s.to, screen: s.screen, wake: w, scene: live.scene, names: live.names));
  }

  Widget hour(int v, void Function(int) on) => DropdownButton<int>(
        value: v,
        dropdownColor: surface,
        underline: const SizedBox(),
        items: [for (var h = 0; h < 24; h++) DropdownMenuItem(value: h, child: Text('${h.toString().padLeft(2, '0')}:00'))],
        onChanged: (h) => on(h!),
      );

  static const label = TextStyle(fontSize: 13, color: text2), hint = TextStyle(fontSize: 13, color: text3);

  Settings get live => widget.k.settings ?? s; // the board's latest look and names: a change here never undoes a pick

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: ListenableBuilder(listenable: widget.k, builder: (context, _) => ListView(padding: const EdgeInsets.fromLTRB(16, 8, 16, 24), children: [
        SwitchListTile(
          contentPadding: EdgeInsets.zero,
          title: const Text('Keychain link'),
          subtitle: const Text('Off: no connection, calls, music or weather until it\'s on again', style: hint),
          value: widget.k.service,
          onChanged: widget.k.setService,
        ),
        const Divider(),
        const SizedBox(height: 8),
        if (widget.k.settings != null) ...pet(),
      ])),
    );
  }

  Future<void> reset(String? look) async {
    final what = look == null ? 'all high scores' : 'the high score of ${live.names[look] ?? looks[look]}';
    final ok = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: surface,
        title: Text('Reset $what?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Cancel')),
          TextButton(onPressed: () => Navigator.pop(context, true), child: const Text('Reset')),
        ],
      ),
    );
    if (ok == true) widget.k.resetBest(look);
  }

  List<Widget> scores() {
    final best = widget.k.best;
    return [
      Row(children: [
        const Expanded(child: Text('High scores', style: label)),
        TextButton(onPressed: best.values.any((v) => v > 0) ? () => reset(null) : null, child: const Text('Reset all')),
      ]),
      for (final look in looks.keys.where(best.containsKey))
        ListTile(
          contentPadding: EdgeInsets.zero,
          dense: true,
          title: Text(live.names[look] ?? looks[look]!),
          trailing: Row(mainAxisSize: MainAxisSize.min, children: [
            Text('${best[look]}', style: const TextStyle(fontSize: 15)),
            IconButton(
              icon: const Icon(Icons.restart_alt, size: 20),
              color: text2,
              tooltip: 'Reset',
              onPressed: best[look]! > 0 ? () => reset(look) : null,
            ),
          ]),
        ),
    ];
  }

  List<Widget> pet() {
    final nightOn = s.from != s.to;
    return [
        const Text('Character', style: label),
        const SizedBox(height: 8),
        CharacterList(widget.k),
        const Text('Each has its own name and high score; stats are shared. Triple tap on the keychain = next one', style: hint),
        const SizedBox(height: 16),
        const Divider(),
        ...scores(),
        const Divider(),
        SwitchListTile(
          contentPadding: EdgeInsets.zero,
          title: const Text('Vibration'),
          subtitle: const Text('Find always buzzes', style: hint),
          value: s.vib,
          onChanged: (v) => set('vib ${v ? 1 : 0}', (vib: v, shake: s.shake, from: s.from, to: s.to, screen: s.screen, wake: s.wake, scene: live.scene, names: live.names)),
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
          onChanged: (v) => setState(() => s = (vib: s.vib, shake: v, from: s.from, to: s.to, screen: s.screen, wake: s.wake, scene: live.scene, names: live.names)),
          onChangeEnd: (v) => set('shake ${v.toStringAsFixed(2)}', s),
        ),
        const Divider(),
        const SizedBox(height: 12),
        const Text('Screen off after', style: label),
        RadioGroup<int>(
          groupValue: s.screen, // an older board's 20 s matches none
          onChanged: (v) => screen(v!),
          child: Column(children: [
            for (final (v, name) in [(30, '30 s'), (60, '1 min'), (300, '5 min')])
              RadioListTile<int>(contentPadding: EdgeInsets.zero, value: v, title: Text(name)),
          ]),
        ),
        const SizedBox(height: 12),
        const Text('Wake the screen', style: label),
        const Text('At least one; motion wakes in the pocket too', style: hint),
        for (final (bit, name) in [(wakeLift, 'When lifted'), (wakeShake, 'When shaken'), (wakeButton, 'With the button')])
          CheckboxListTile(
            contentPadding: EdgeInsets.zero,
            controlAffinity: ListTileControlAffinity.leading,
            title: Text(name),
            value: s.wake & bit != 0,
            onChanged: s.wake == bit ? null : (v) => wake(bit, v!), // the last one stays on
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
    ];
  }
}
