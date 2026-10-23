import 'package:flutter_test/flutter_test.dart';
import 'package:mysterychain/keychain.dart';
import 'package:mysterychain/protocol.dart';

void main() {
  test('parses the board lines the app keeps', () {
    expect(parseStats('stats 1234 happy'), (steps: 1234, mood: 'happy', battery: null));
    expect(parseStats('stats 1234 tired 12'), (steps: 1234, mood: 'tired', battery: 12));
    expect(parseStats('stats 0 happy -1'), (steps: 0, mood: 'happy', battery: -1));
    final old = parseSettings('settings vib=1 shake=0.45 night=23-7 name=Mio Kedi')!;
    expect((old.vib, old.shake, old.from, old.to, old.scene), (true, .45, 23, 7, 'cat'));
    expect(old.names, {'cat': 'Mio Kedi'}); // records compare a map by identity
    expect(parseSettings('settings vib=1 shake=0.45 night=23-7 scene=pavi name=Mio')?.names, {'pavi': 'Mio'});
    final s = parseSettings('settings vib=1 shake=0.45 night=23-7 scene=nausicaa name=Cat|Pavi 2|Nau')!;
    expect(s.names, {'cat': 'Cat', 'pavi': 'Pavi 2', 'nausicaa': 'Nau'});
    expect(nameOf(s), 'Nau');
    expect(parseGame('game 12 best 40'), (score: 12, best: 40));
    expect(parseStats('stats x happy'), isNull);
    expect(parseSettings('button 3'), isNull);
    expect(parseGame('game over'), isNull);
    expect(parseBest('best 3|0|17|0|5'), [3, 0, 17, 0, 5]);
    expect(parseBest('best '), isNull);
  });

  test('board lines update the screen state or land in the activity list', () {
    final k = Keychain()
      ..ringing = true
      ..onLine('stats 10 sleepy')
      ..onLine('settings vib=0 shake=0.30 night=0-0 name=Pet')
      ..onLine('fed')
      ..onLine('found');
    expect(k.stats, (steps: 10, mood: 'sleepy', battery: null));
    expect(k.settings?.vib, isFalse);
    expect(k.ringing, isFalse);
    expect(k.events.map((e) => e.text), ['found', 'Ate']); // newest first; stats/settings aren't events
    k
      ..onLine('settings vib=1 shake=0.45 night=23-7 scene=pavi name=Cat|Pavi|Nau|Gan|Sau')
      ..onLine('best 3|0|17|0|5')
      ..onLine('game 9 best 9');
    expect(k.best, {'cat': 3, 'pavi': 9, 'nausicaa': 17, 'gandalf': 0, 'sauron': 5}); // a game over updates its look
  });
}
