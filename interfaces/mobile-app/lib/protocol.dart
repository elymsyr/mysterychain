// The keychain's lines the app reads into records (interfaces/watch/README.md#protocol). Anything else is an event.

typedef Stats = ({int steps, String mood});
typedef Settings = ({bool vib, double shake, int from, int to, String name}); // night from == to: no night window
typedef Game = ({int score, int best});

Stats? parseStats(String line) {
  final m = RegExp(r'^stats (\d+) (\w+)$').firstMatch(line);
  return m == null ? null : (steps: int.parse(m[1]!), mood: m[2]!);
}

Settings? parseSettings(String line) {
  final m = RegExp(r'^settings vib=(\d) shake=([\d.]+) night=(\d+)-(\d+) name=(.*)$').firstMatch(line);
  if (m == null) return null;
  return (vib: m[1] == '1', shake: double.parse(m[2]!), from: int.parse(m[3]!), to: int.parse(m[4]!), name: m[5]!);
}

Game? parseGame(String line) {
  final m = RegExp(r'^game (\d+) best (\d+)$').firstMatch(line);
  return m == null ? null : (score: int.parse(m[1]!), best: int.parse(m[2]!));
}
