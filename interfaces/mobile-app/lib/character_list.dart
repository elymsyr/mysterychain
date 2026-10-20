// The board's looks, one per row: tap picks it (tick on the current one), the pencil renames it. Each look keeps its
// own name and game high score on the board; stats are shared.
import 'package:flutter/material.dart';

import 'keychain.dart';
import 'pet_head.dart';
import 'protocol.dart';
import 'theme.dart';

class CharacterList extends StatelessWidget {
  const CharacterList(this.k, {super.key, this.onPicked});
  final Keychain k;
  final VoidCallback? onPicked;

  Future<void> rename(BuildContext context, Settings s, String look) async {
    final field = TextEditingController(text: s.names[look]);
    final v = await showDialog<String>(
      context: context,
      builder:
          (context) => AlertDialog(
            backgroundColor: surface,
            title: Text('Name of ${looks[look]}'),
            content: TextField(
              controller: field,
              autofocus: true,
              maxLength: 12, // the board keeps 12 ASCII characters
              textInputAction: TextInputAction.done,
              onSubmitted: (v) => Navigator.pop(context, v),
            ),
            actions: [
              TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
              TextButton(onPressed: () => Navigator.pop(context, field.text), child: const Text('Save')),
            ],
          ),
    );
    field.dispose();
    final name = v?.trim().replaceAll('|', '/') ?? '';
    final now = k.settings;
    if (name.isEmpty || now == null || name == now.names[look]) return;
    k.set('name $look $name', (
      vib: now.vib,
      shake: now.shake,
      from: now.from,
      to: now.to,
      scene: now.scene,
      names: {...now.names, look: name},
    ));
  }

  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: k,
    builder: (context, _) {
      final s = k.settings;
      if (s == null) return const SizedBox();
      return Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          for (final look in looks.keys.where(s.names.containsKey))
            ListTile(
              contentPadding: EdgeInsets.zero,
              leading: CustomPaint(size: const Size(36, 34), painter: PetHead(look)),
              title: Text(s.names[look]!),
              subtitle: s.names[look] == looks[look] ? null : Text(looks[look]!, style: const TextStyle(color: text3)),
              trailing: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  IconButton(
                    icon: const Icon(Icons.edit_outlined, size: 20),
                    color: text2,
                    tooltip: 'Rename',
                    onPressed: () => rename(context, s, look),
                  ),
                  SizedBox(width: 24, child: s.scene == look ? Icon(Icons.check, color: accent) : null),
                ],
              ),
              onTap: () {
                if (look != s.scene) {
                  k.set('scene $look', (
                    vib: s.vib,
                    shake: s.shake,
                    from: s.from,
                    to: s.to,
                    scene: look,
                    names: s.names,
                  ));
                }
                onPicked?.call();
              },
            ),
        ],
      );
    },
  );
}
