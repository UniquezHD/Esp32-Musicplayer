#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include "Arduino.h"
#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

// https://forum.arduino.cc/t/how-to-play-random-files-and-folders-on-a-yx-5300/574797
// https://github.com/rdiot/rdiot-b208/blob/master/Serial%20MP3%20Player%20v1.0%20Manual.pdf
// https://www.youtube.com/watch?v=wXDFW6NsDK4

//https://arduinoplusplus.wordpress.com/2018/07/23/yx5300-serial-mp3-player-catalex-module/

// Adafruit LC709203F LiPoly / LiIon Fuel Gauge and Battery Monitor - STEMMA JST PH & QT / Qwiic

#define CMD_NEXT_SONG         0X01    //-> Play next song.
#define CMD_PREV_SONG         0X02    //-> Play previous song.
#define CMD_PLAY_W_INDEX      0X03    //-> Play with index.
// Play with index : 7E FF 06 03 00 00 01 EF -> Play the first song.
// Play with index : 7E FF 06 03 00 00 02 EF -> Play the second song.

#define CMD_VOLUME_UP         0X04    //-> Volume increased one.
#define CMD_VOLUME_DOWN       0X05    //-> Volume decrease one.
#define CMD_SET_VOLUME        0X06    //-> Set volume.
// Set volume : 7E FF 06 06 00 00 1E EF -> Set the volume to 30 (0x1E is 30) (Set volume is 06).

#define CMD_SNG_CYCL_PLAY     0X08    //-> Single Cycle Play.
// Single Cycle Play : 7E FF 06 08 00 00 01 EF -> Single cycle play the first song.

#define CMD_SEL_DEV           0X09    //-> Select device.
#define DEV_TF                0X02    //-> 02 for TF card.
// Select device : 7E FF 06 09 00 00 02 EF -> Select storage device to TF card.

#define CMD_SLEEP_MODE        0X0A    //-> Chip enters sleep mode
#define CMD_WAKE_UP           0X0B    //-> Chip wakes up
#define CMD_RESET             0X0C    //-> Chip reset
#define CMD_PLAY              0X0D    //-> Resume playback
#define CMD_PAUSE             0X0E    //-> Playback is paused
#define CMD_PLAY_FOLDER_FILE  0X0F    //-> Play with folder and file name.
// Play with folder and file name : 7E FF 06 0F 00 01 01 EF -> Play the song with the directory: /01/001xxx.mp3
// Play with folder and file name : 7E FF 06 0F 00 01 02 EF -> Play the song with the directory: /01/002xxx.mp3

#define CMD_STOP_PLAY         0X16    //-> Stop playing continuously. 
#define CMD_FOLDER_CYCLE      0X17    //-> Cycle play with folder name.
// Cycle play with folder name : 7E FF 06 17 00 01 02 EF -> 01 folder cycle play

#define CMD_SHUFFLE_PLAY      0x18    //-> Shuffle play.
#define CMD_SET_SNGL_CYCL     0X19    //-> Set single cycle play.
// Set single cycle play : 7E FF 06 19 00 00 00 EF -> Start up single cycle play.
// Set single cycle play : 7E FF 06 19 00 00 01 EF -> Close single cycle play.

#define CMD_SET_DAC           0X1A    //-> Set DAC.
#define DAC_ON                0X00    //-> DAC On
#define DAC_OFF               0X01    //-> DAC Off
// Set DAC : 7E FF 06 1A 00 00 00 EF -> Start up DAC output.
// Set DAC : 7E FF 06 1A 00 00 01 EF -> DAC no output.

#define CMD_PLAY_W_VOL        0X22    //-> Play with volume.
// Play with volume : 7E FF 06 22 00 1E 01 EF -> Set the volume to 30 (0x1E is 30) and play the first song
// Play with volume : 7E FF 06 22 00 0F 02 EF -> Set the volume to 15(0x0f is 15) and play the second song

#define CMD_PLAYING_N         0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_SET_EQUALIZER     0x07
#define CMD_QUERY_EQUALIZER   0x44
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f
// -------------------------------------------------------

#define BACK 0
#define FORWARD 1
#define CURRENT 2

int BUTTON_PIN_NEXT = 14;
int BUTTON_PIN_BACK = 32;
int BUTTON_PIN_ENTER = 33;
int BUTTON_PIN_DIR_BACK = 26;
int BUTTON_PIN_MODE = 25;

int DISPLAY_HEIGHT = 64;
int DISPLAY_WIDTH = 128;

const int MODE_ITEM_COUNT = 4;

void SendCommand(byte command, byte dat1, byte dat2);
String decodeMP3Answer();
void SendCommand(byte command);
String sbyte2hex(uint8_t b);
void SendMP3Command(char c);
String sanswer(void);
void PlayNext(int currentSong, int direction);
String FormatTime(const char *durationMs);
String FormatTimeMs(unsigned long ms);
void HandleTime(char* duration);
void HandleKey(char c);
void Search(String query);