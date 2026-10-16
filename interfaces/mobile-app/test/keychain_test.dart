import 'package:flutter_test/flutter_test.dart';
import 'package:mysterychain/keychain.dart';
import 'package:mysterychain/protocol.dart';

void main() {
  test('parses the board lines the app keeps', () {
    expect(parseStats('stats 1234 happy'), (steps: 1234, mood: 'happy'));
    expect(parseSettings('settings vib=1 shake=0.45 night=23-7 name=Mio Kedi'),
        (vib: true, shake: .45, from: 23, to: 7, name: 'Mio Kedi'));
    expect(parseGame('game 12 best 40'), (score: 12, best: 40));
    expect(parseStats('stats x happy'), isNull);
    expect(parseSettings('button 3'), isNull);
    expect(parseGame('game over'), isNull);
  });

  test('board lines update the screen state or land in the activity list', () {
    final k = Keychain()
      ..ringing = true
      ..onLine('stats 10 sleepy')
      ..onLine('settings vib=0 shake=0.30 night=0-0 name=Pet')
      ..onLine('fed')
      ..onLine('found');
    expect(k.stats, (steps: 10, mood: 'sleepy'));
    expect(k.settings?.vib, isFalse);
    expect(k.ringing, isFalse);
    expect(k.events.map((e) => e.text), ['found', 'Ate']); // newest first; stats/settings aren't events
  });
}
