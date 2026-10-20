import 'package:flutter/material.dart';

// The pet's head, eyes open, as on the keychain: the cat (HEAD[0] in interfaces/watch/firmware/cat.h), Pavi or
// Nausicaa (HEAD[0] in pavi.h / nausicaa.h, facing right). Pixels are square and as big as fit the size.
class PetHead extends CustomPainter {
  const PetHead(this.scene);
  final String scene; // the board's `scene=` setting: cat, pavi or nausicaa

  static const cat = [
    '.K........K.', 'KPK......KPK', 'KPPK....KPPK', 'KOOKKKKKKOOK', 'KOOOOOOOOOOK',
    'KOKWOOOOKWOK', 'KOKKOOOOKKOK', 'KOOOWPPWOOOK', '.KOWWWWWWOK.', '..KKKKKKKK..',
  ];
  static const catColors = {
    'K': Color(0xFF1E1216), 'O': Color(0xFFFF9628), 'W': Color(0xFFFFECD2), 'P': Color(0xFFFF8CA0),
  };
  static const pavi = [
    '.........KK.....', '........KHK.....', '.....KKKHKKKK...', '...KKHHHHHHHHKK.', '..KHHHLHHHHHHHHK',
    '.KHHYYYYYYYYYYHK', '.KHHHHHHHHHHHHHK', 'KHHHHLHHHHHHHHHK', 'KHHHHHSHHSSSHSHK', 'KhHHHSSKKSSSKKSK',
    'KhHHHSSWESSSWESK', 'KhHHHSSEESSSEESK', 'KhHHHSSSSSSSSSSK', 'KhHHHHSSSSMMSSK.', 'KhHHHHHSSSSSSK..',
    'KhHHHHHKKKKKK...', 'KHhHHHK.........', '.KHhHK..........', '..KKK...........',
  ];
  static const paviColors = {
    'K': Color(0xFF180E1E), 'H': Color(0xFF543E96), 'h': Color(0xFF322664), 'L': Color(0xFF826EC8),
    'Y': Color(0xFFEEC65C), 'S': Color(0xFF865036), 'W': Color(0xFFF5EEDE), 'E': Color(0xFF50BE6E),
    'M': Color(0xFFA03C46),
  };
  static const nausicaa = [
    '.....KKKKKK.....', '...KKHHHHHHKK...', '..KHHHLLHHHHHK..', '.KHHHHHHLLHHHHK.', '.KHHHHHHHHHHHHHK',
    'KHHHHHHHHHHHHHHK', 'KHHHHHHHHSHHHSHK', 'KhHHHHHSSSSHSSSK', 'KhHHHHSKKSSSKKSK', 'KhHHHHSWESSSWESK',
    'KhHHHhSEESSSEESK', 'KhHHHhSSSSSSSSSK', 'KhHHHhSSSSSSSSK.', 'KhHHHhSSSSSSMSK.', 'KhHhhRSSSSSSSK..',
    '.KhKKKKSSSSSK...', '..K....KKKKK....',
  ];
  static const nausicaaColors = {
    'K': Color(0xFF1E161E), 'H': Color(0xFFBA4E3A), 'h': Color(0xFF7C2C26), 'L': Color(0xFFE27652),
    'S': Color(0xFFF6C8A6), 'E': Color(0xFF40486E), 'W': Color(0xFFFAF0DC), 'M': Color(0xFFBE5A5A),
    'R': Color(0xFFDC2832),
  };
  static const gandalf = [
    '......KK......', '.....KHHK.....', '....KHHHHK....', '...KHHHHHHK...', 'KKKHHHHHHHHKKK',
    'KHHHHHHHHHHHHK', '.KKKKKKKKKKKK.', '..KWSSSSSSWK..', '..KWEKSSKEWK..', '..KWSSSSSSWK..',
    '..KWWWWWWWWK..', '..KWWWWWWWWK..', '...KWWWWWWK...', '....KWWWWK....', '.....KKKK.....',
  ];
  static const gandalfColors = {
    'K': Color(0xFF1E1A1E), 'H': Color(0xFF8C8C96), 'W': Color(0xFFDCDCE0), 'S': Color(0xFFF0C8A0),
    'E': Color(0xFF3C4A6E),
  };
  static const sauron = [
    '.K....KK....K.', '.KK..KAAK..KK.', '.KAKKAAAAKKAK.', '..KAAAAAAAAK..', '..KAAAAAAAAK..',
    '..KAKKKKKKAK..', '..KAKRRRRKAK..', '..KAKKKKKKAK..', '..KAAAKKAAAK..', '..KAAAKKAAAK..',
    '...KAAKKAAK...', '....KAAAAK....', '.....KKKK.....',
  ];
  static const sauronColors = {'K': Color(0xFF0E0C10), 'A': Color(0xFF4A4650), 'R': Color(0xFFFF7A1E)};
  static const art = {'cat': (cat, catColors), 'pavi': (pavi, paviColors), 'nausicaa': (nausicaa, nausicaaColors),
    'gandalf': (gandalf, gandalfColors), 'sauron': (sauron, sauronColors)};

  @override
  void paint(Canvas canvas, Size size) {
    final (rows, colors) = art[scene] ?? art['cat']!;
    final px = (size.width / rows[0].length).clamp(0, size.height / rows.length).toDouble();
    final dx = (size.width - px * rows[0].length) / 2, dy = (size.height - px * rows.length) / 2;
    for (var y = 0; y < rows.length; y++) {
      for (var x = 0; x < rows[y].length; x++) {
        final c = colors[rows[y][x]];
        if (c != null) {
          canvas.drawRect(Rect.fromLTWH(dx + x * px, dy + y * px, px, px), Paint()..color = c..isAntiAlias = false);
        }
      }
    }
  }

  @override
  bool shouldRepaint(PetHead oldDelegate) => oldDelegate.scene != scene;
}
