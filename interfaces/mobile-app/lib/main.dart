// MysteryChain: talks to the keychain over BLE Nordic UART.
// Finds the board by name and connects; if the link drops, Android reconnects on its own once in range (autoConnect).
// Sent text shows up in the board's speech bubble and buzzes; the board's button/shake events land in the list.
import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

const deviceName = 'MysteryChain';
final uartService = Guid('6E400001-B5A3-F393-E0A9-E50E24DCCA9E');
final rxUuid = Guid('6E400002-B5A3-F393-E0A9-E50E24DCCA9E'); // board RX: phone writes
final txUuid = Guid('6E400003-B5A3-F393-E0A9-E50E24DCCA9E'); // board TX: phone listens

void main() => runApp(MaterialApp(
      title: 'MysteryChain',
      theme: ThemeData(colorSchemeSeed: Colors.orange),
      home: const PetPage(),
    ));

typedef PetEvent = ({DateTime at, bool fromPet, String text});

class PetPage extends StatefulWidget {
  const PetPage({super.key});

  @override
  State<PetPage> createState() => _PetPageState();
}

class _PetPageState extends State<PetPage> {
  String status = 'Waiting for Bluetooth';
  BluetoothDevice? pet;
  BluetoothCharacteristic? rx; // non-null = connected and ready
  final events = <PetEvent>[];
  final input = TextEditingController();
  final subs = <StreamSubscription>[];

  @override
  void initState() {
    super.initState();
    subs.add(FlutterBluePlus.adapterState.listen((s) {
      if (s != BluetoothAdapterState.on) return setState(() => status = 'Bluetooth off');
      if (pet == null) scan();
    }));
    FlutterBluePlus.turnOn().catchError((_) {}); // asks to turn it on if off
  }

  @override
  void dispose() {
    for (final s in subs) {
      s.cancel();
    }
    input.dispose();
    super.dispose();
  }

  void log(bool fromPet, String text) =>
      setState(() => events.insert(0, (at: DateTime.now(), fromPet: fromPet, text: text)));

  Future<void> scan() async {
    setState(() => status = 'Scanning');
    final sub = FlutterBluePlus.onScanResults.listen((results) {
      if (results.isEmpty || pet != null) return;
      FlutterBluePlus.stopScan();
      attach(results.last.device);
    });
    FlutterBluePlus.cancelWhenScanComplete(sub);
    await FlutterBluePlus.startScan(withNames: [deviceName]);
  }

  void attach(BluetoothDevice d) {
    pet = d;
    setState(() => status = 'Connecting');
    subs.add(d.connectionState.listen((s) {
      if (s == BluetoothConnectionState.connected) {
        setup(d);
        return;
      }
      if (rx != null) log(true, 'disconnected');
      setState(() {
        rx = null;
        status = 'Waiting';
      });
    }));
    // ponytail: Android autoConnect can be slow (background scan); if it becomes a problem, connect directly + retry loop
    d.connect(license: License.nonprofit, autoConnect: true, mtu: null);
  }

  Future<void> setup(BluetoothDevice d) async {
    try {
      await d.requestMtu(247).catchError((_) => 23); // long messages go in one write (board uses 247 too)
      final uart = (await d.discoverServices()).firstWhere((s) => s.uuid == uartService);
      final tx = uart.characteristics.firstWhere((c) => c.uuid == txUuid);
      d.cancelWhenDisconnected(tx.onValueReceived.listen((v) => log(true, utf8.decode(v, allowMalformed: true).trim())));
      await tx.setNotifyValue(true);
      setState(() {
        rx = uart.characteristics.firstWhere((c) => c.uuid == rxUuid);
        status = 'Connected';
      });
    } catch (e) {
      setState(() => status = 'Setup error'); // retried on the next reconnect
      log(true, 'setup error: $e');
    }
  }

  Future<void> send() async {
    final text = input.text.trim();
    if (text.isEmpty || rx == null) return;
    try {
      await rx!.write(utf8.encode(text));
      log(false, text);
      input.clear();
    } catch (e) {
      if (mounted) ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Send failed: $e')));
    }
  }

  @override
  Widget build(BuildContext context) {
    final connected = rx != null;
    return Scaffold(
      appBar: AppBar(
        title: const Text('MysteryChain'),
        actions: [
          Icon(Icons.circle, size: 12, color: connected ? Colors.green : Colors.grey),
          Padding(padding: const EdgeInsets.fromLTRB(6, 0, 16, 0), child: Text(status)),
        ],
      ),
      body: Column(children: [
        Expanded(
          child: events.isEmpty
              ? const Center(child: Text('No events yet.\nPress the button or shake the keychain.', textAlign: TextAlign.center))
              : ListView.builder(
                  itemCount: events.length,
                  itemBuilder: (_, i) {
                    final e = events[i];
                    return ListTile(
                      dense: true,
                      leading: Icon(e.fromPet ? Icons.pets : Icons.send),
                      title: Text(e.text),
                      trailing: Text(e.at.toIso8601String().substring(11, 19)), // HH:mm:ss
                    );
                  },
                ),
        ),
        SafeArea(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(16, 4, 8, 8),
            child: Row(children: [
              Expanded(
                child: TextField(
                  controller: input,
                  enabled: connected,
                  maxLength: 60, // board screen holds 63 characters
                  textInputAction: TextInputAction.send,
                  onSubmitted: (_) => send(),
                  decoration: const InputDecoration(hintText: 'Message the pet', counterText: ''),
                ),
              ),
              IconButton.filled(onPressed: connected ? send : null, icon: const Icon(Icons.send)),
            ]),
          ),
        ),
      ]),
    );
  }
}
