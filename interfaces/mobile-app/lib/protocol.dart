// The keychain's lines the app reads into records (interfaces/watch/README.md#protocol). Anything else is an event.

typedef Stats = ({int steps, String mood});
// night from == to: no night window; scene: the pet's look (one of [looks]); names: each look's own name
typedef Settings = ({bool vib, double shake, int from, int to, String scene, Map<String, String> names});

const looks = {'cat': 'Cat', 'pavi': 'Pavi', 'nausicaa': 'Nausicaa'}; // board order (firmware pets.h) and labels

String nameOf(Settings s) => s.names[s.scene] ?? looks[s.scene] ?? s.scene;
typedef Game = ({int score, int best});

Stats? parseStats(String line) {
  final m = RegExp(r'^stats (\d+) (\w+)$').firstMatch(line);
  return m == null ? null : (steps: int.parse(m[1]!), mood: m[2]!);
}

Settings? parseSettings(String line) {
  final m = RegExp(r'^settings vib=(\d) shake=([\d.]+) night=(\d+)-(\d+)(?: scene=(\w+))? name=(.*)$').firstMatch(line);
  if (m == null) return null;
  final scene = m[5] ?? 'cat', parts = m[6]!.split('|'); // a board without the look switch is the cat
  return (
    vib: m[1] == '1',
    shake: double.parse(m[2]!),
    from: int.parse(m[3]!),
    to: int.parse(m[4]!),
    scene: scene,
    names: parts.length == 1 ? {scene: parts[0]} : {for (final (i, l) in looks.keys.indexed) if (i < parts.length) l: parts[i]},
  );
}

Game? parseGame(String line) {
  final m = RegExp(r'^game (\d+) best (\d+)$').firstMatch(line);
  return m == null ? null : (score: int.parse(m[1]!), best: int.parse(m[2]!));
}
