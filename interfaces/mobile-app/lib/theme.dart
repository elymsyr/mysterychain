// Flat dark theme, one accent taken from the cat's fur. Look: docs/mockups/app_screens.png.
import 'package:flutter/material.dart';

const bg = Color(0xFF171614), surface = Color(0xFF201E1B), line = Color(0xFF312E29), rowLine = Color(0xFF221F1C);
const text1 = Color(0xFFEBE6DD), text2 = Color(0xFFA39D92), text3 = Color(0xFF6C675E);
const accent = Color(0xFFE3A043), accentInk = Color(0xFF1D1406), ok = Color(0xFF8BB174), heart = Color(0xFFF08CA0);
final corner = BorderRadius.circular(6);

final theme = ThemeData(
  brightness: Brightness.dark,
  scaffoldBackgroundColor: bg,
  colorScheme: const ColorScheme.dark(primary: accent, onPrimary: accentInk, surface: bg, onSurface: text1),
  dividerTheme: const DividerThemeData(color: line, thickness: 1, space: 1),
  appBarTheme: const AppBarTheme(
      backgroundColor: bg, foregroundColor: text1, elevation: 0, surfaceTintColor: Colors.transparent),
  outlinedButtonTheme: OutlinedButtonThemeData(
    style: OutlinedButton.styleFrom(
      foregroundColor: text1,
      backgroundColor: surface,
      disabledForegroundColor: text3,
      side: const BorderSide(color: line),
      shape: RoundedRectangleBorder(borderRadius: corner),
      minimumSize: const Size(0, 34),
      padding: const EdgeInsets.symmetric(horizontal: 14),
      textStyle: const TextStyle(fontSize: 14, fontWeight: FontWeight.w400),
    ),
  ),
  inputDecorationTheme: InputDecorationTheme(
    filled: true,
    fillColor: surface,
    hintStyle: const TextStyle(color: text3),
    contentPadding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
    enabledBorder: OutlineInputBorder(borderRadius: corner, borderSide: const BorderSide(color: line)),
    disabledBorder: OutlineInputBorder(borderRadius: corner, borderSide: const BorderSide(color: line)),
    focusedBorder: OutlineInputBorder(borderRadius: corner, borderSide: const BorderSide(color: accent)),
  ),
);
