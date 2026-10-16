import 'package:flutter/material.dart';

// The cat's head, eyes open (HEAD[0] in interfaces/watch/firmware/cat.h): K ink, O fur, W cream, P pink.
class CatHead extends CustomPainter {
  const CatHead();

  static const rows = [
    '.K........K.', 'KPK......KPK', 'KPPK....KPPK', 'KOOKKKKKKOOK', 'KOOOOOOOOOOK',
    'KOKWOOOOKWOK', 'KOKKOOOOKKOK', 'KOOOWPPWOOOK', '.KOWWWWWWOK.', '..KKKKKKKK..',
  ];
  static const colors = {
    'K': Color(0xFF1E1216), 'O': Color(0xFFFF9628), 'W': Color(0xFFFFECD2), 'P': Color(0xFFFF8CA0),
  };

  @override
  void paint(Canvas canvas, Size size) {
    final px = size.width / rows[0].length;
    for (var y = 0; y < rows.length; y++) {
      for (var x = 0; x < rows[y].length; x++) {
        final c = colors[rows[y][x]];
        if (c != null) {
          canvas.drawRect(Rect.fromLTWH(x * px, y * px, px, px), Paint()..color = c..isAntiAlias = false);
        }
      }
    }
  }

  @override
  bool shouldRepaint(CatHead oldDelegate) => false;
}
