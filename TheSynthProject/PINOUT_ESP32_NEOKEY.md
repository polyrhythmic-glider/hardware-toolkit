# Pinout NeoKey nel firmware integrato

Il pinout attivo non è più quello della vecchia build solo TFT/NeoKey.

- SDA della catena NeoKey: PCA9548A `SD4`
- SCL della catena NeoKey: PCA9548A `SC4`
- primo gruppo fisico da sinistra, step 1-4: `0x30`
- secondo gruppo fisico, step 5-8: `0x31`, jumper A0 chiuso
- terzo gruppo fisico, step 9-12: `0x32`, jumper A1 chiuso
- quarto gruppo fisico, step 13-16: `0x33`, jumper A0 e A1 chiusi
- VIN: 3V3
- GND: massa comune

Gli step avanzano da sinistra verso destra, sia tra i quattro moduli sia tra i
quattro tasti di ogni NeoKey.

I quattro NeoKey sono concatenati tra loro e condividono il canale 4 del PCA9548A; i
quattro OLED usano i canali 0-3. Il canale 5 resta libero. Con il backend Daisy
il PCM5102 non è collegato; D13, D14 e D27 restano liberi.

Per i pin ESP32 usare esclusivamente i nomi della serigrafia elencati in
[`ESP32_PIN_NAMES.md`](ESP32_PIN_NAMES.md).

Per la tabella completa e la procedura usare
[`WIRING_INTEGRATED.md`](WIRING_INTEGRATED.md).
