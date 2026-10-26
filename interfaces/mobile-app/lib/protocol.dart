// The keychain's lines the app reads into records (interfaces/watch/README.md#protocol). Anything else is an event.

typedef Stats = ({int steps, String mood, int? battery}); // battery %: -1 on USB, null from an older board
// night from == to: no night window; screen: s lit after the button (30/60/300); wake: wakeButton|wakeShake|wakeLift bits; scene: the pet's look (one of [looks]); names: each look's own name
typedef Settings = ({bool vib, double shake, int from, int to, int screen, int wake, String scene, Map<String, String> names});
const wakeButton = 1, wakeShake = 2, wakeLift = 4; // firmware WAKE_*

const looks = {'cat': 'Cat', 'pavi': 'Pavi', 'nausicaa': 'Nausicaa', 'gandalf': 'Gandalf', 'sauron': 'Sauron'}; // board order (firmware pets.h) and labels

String nameOf(Settings s) => s.names[s.scene] ?? looks[s.scene] ?? s.scene;
typedef Game = ({int score, int best});

Stats? parseStats(String line) {
  final m = RegExp(r'^stats (\d+) (\w+)(?: (-?\d+))?$').firstMatch(line);
  return m == null ? null : (steps: int.parse(m[1]!), mood: m[2]!, battery: m[3] == null ? null : int.parse(m[3]!));
}

Settings? parseSettings(String line) {
  final m = RegExp(r'^settings vib=(\d) shake=([\d.]+) night=(\d+)-(\d+)(?: screen=(\d+) wake=(\d+))?(?: scene=(\w+))? name=(.*)$').firstMatch(line);
  if (m == null) return null;
  final scene = m[7] ?? 'cat', parts = m[8]!.split('|'); // a board without the look switch is the cat
  return (
    vib: m[1] == '1',
    shake: double.parse(m[2]!),
    from: int.parse(m[3]!),
    to: int.parse(m[4]!),
    screen: int.parse(m[5] ?? '20'), // an older board: fixed 20 s, button only
    wake: int.parse(m[6] ?? '$wakeButton'),
    scene: scene,
    names: parts.length == 1 ? {scene: parts[0]} : {for (final (i, l) in looks.keys.indexed) if (i < parts.length) l: parts[i]},
  );
}

// Each look's game high score, board order; a single-scene board sends one
List<int>? parseBest(String line) {
  final m = RegExp(r'^best (\d+(?:\|\d+)*)$').firstMatch(line);
  return m?[1]!.split('|').map(int.parse).toList();
}

Game? parseGame(String line) {
  final m = RegExp(r'^game (\d+) best (\d+)$').firstMatch(line);
  return m == null ? null : (score: int.parse(m[1]!), best: int.parse(m[2]!));
}
