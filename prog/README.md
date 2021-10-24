# Pin mapping

## back connector
|:pin:|:esp32 pin:|:GPIO:|:usage              :|
|-----|-----------|------|---------------------|
|   1 |           |      | GND                 |
|   2 |         3 |      | EN                  |
|   3 |        25 |    0 | IO0                 |
|   4 |           |      | NC                  |
|   5 |        34 |    3 | RXD0                |
|   6 |        35 |    1 | TXD0                |

## front connector
|:pin:|:esp32 pin:|:GPIO:|:usage              :|
|   1 |           |      | 3V3                 |
|   2 |        37 |   23 | SPI MOSI            |
|   3 |        30 |   18 | SPI SCLK            |
|   4 |        29 |    5 | OLED CS             |
|   5 |        31 |   19 | OLED reset          |
|   6 |        36 |   22 | OLED data/command   |
|   7 |        33 |   21 |                     |
|   8 |           |      | GND                 |

## left connector
|:pin:|:esp32 pin:|:GPIO:|:usage              :|
|   1 |           |      | GND                 |
|   2 |           |      | 3V3                 |
|   3 |         6 |   34 | SW1                 |
|   4 |         7 |   35 | SW2                 |
|   5 |        10 |   25 | SPK                 |
|   6 |        11 |   26 |                     |
|   7 |        12 |   27 |                     |
|   8 |        13 |   14 |                     |
|   9 |        14 |   12 |                     |
|  10 |        16 |   13 | SW3                 |

## right connector
|:pin:|:esp32 pin:|:GPIO:|:usage              :|
|   1 |           |      | GND                 |
|   2 |           |      | Vcc                 |
|   3 |        37 |   23 | SPI MOSI            |
|   4 |        31 |   19 |                     |
|   5 |        30 |   18 | SPI SCLK            |
|   6 |        28 |   17 | charger stat        |
|   7 |        27 |   16 | 3V3 output          |
|   8 |        26 |    4 |                     |
|   9 |        24 |    2 |                     |
|  10 |        23 |   15 | 3V3 measure(ADC2_3) |

## unexposed GPIOs
|:pin:|:esp32 pin:|:GPIO:|:usage              :|
|     |         8 |   32 | XTALP               |
|     |         9 |   33 | XTALN               |
|     |         4 |   36 |                     |
|     |         5 |   39 |                     |

# ESP-IDF

This project use ESP-IDF v4.0.

# Playing alarm

Clock can play alarm using wav data in spiffs storage.

1. create `spiffs` directory
2. place alarm files in the `spiffs` directory.
    - File name must be `alarm0.wav`, `alarm1.wav` or `alarm2.wav`.
    - Format of .wav files must be RIFF WAVE and audio encoding is PCM.
3. create spiffs and flash
    ```sh
    make spiffs && make spiffs-flash
    ```
4. configure alarm in settings and select sound id matching the number in the file.

# Say time and SPIFFS

To make clock say time, precreated voice(.wav) files and `time_vo.bin` must be
placed in the `spiffs` directory.

1. create file named `time_vo.txt` in the `spiffs` directory\
    see [time_vo.txt](#time_votxt) for detail.
2. compile `time_vo.txt` to `time_vo.bin`
    ```sh
    make -f gen.make gen-time_vo
    ```
3. create spiffs and flash
    ```sh
    make spiffs && make spiffs-flash
    ```

## time_vo.txt
`time_vo.txt` contains which files to play when hour is HH and minutes is MM:
```
hourHH: file1.wav file2.wav ...
minMM: file1.wav file2.wav ...
```
where HH is from 00 to 23, MM is from 00 to 59.

Each entry can conatin up to 8 files.
Each file name must be less than 16 bytes.

See `time_vo.txt.example` in `tools` for exmaple.

Suppose `time_vo.txt` contains this:
```
hour12: hour_is.wav num12.wav
min21: min_is.wav num20.wav num01.wav
```
And when time is 12:21, clock plays files in order:
```
hour_is.wav
num12.wav
min_is.wav
num20.wav
num01.wav
```
so you would hear
```
hour is twelve minute is twenty one
```

Note clock only plays voice in the fixed order (`hourHH` then `minMM`)
and does not support something like "half past 6" or "six thirty a.m.".
