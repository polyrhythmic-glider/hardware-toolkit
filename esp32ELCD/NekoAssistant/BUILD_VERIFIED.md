# Build Arduino verificata

Verifica eseguita il **15 luglio 2026** con:

- Arduino CLI 1.5.1 (quello incluso in Arduino IDE);
- `esp32:esp32` 3.3.10;
- board `ESP32S3 Dev Module`;
- U8g2 2.36.19 installata nell'Arduino IDE;
- Flash 16 MB, QIO 80 MHz, OPI PSRAM, doppia partizione OTA da 3 MB.

FQBN usato:

```text
esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadMode=default
```

Esito:

```text
Sketch uses 1287753 bytes (40%) of program storage space. Maximum is 3145728 bytes.
Global variables use 52112 bytes (15%) of dynamic memory, leaving 275568 bytes for local variables. Maximum is 327680 bytes.
```

Artefatti:

| File | SHA-256 |
|---|---|
| `build-key-loop-task-v2/NekoAssistant.ino.bin` | `5F098B45557A9E387DF8B8A20EAAD71302A2A0F687DCE386E952980F76217D4E` |
| `build-key-loop-task-v2/NekoAssistant.ino.merged.bin` | `AE4CB53B43600E13428E4ACD544AE798F91F2FFFD1477D779CE7D995E123DE3D` |

La compilazione non sostituisce il collaudo sulla scheda fisica: display, I2C,
ADC, SD, RF, sleep/wake e OTA devono essere provati seguendo il test plan.
