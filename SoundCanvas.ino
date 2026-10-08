#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>

// ======================================================
//                SOUNDCANVAS VERSION 1
// ======================================================
//
// Android Phone
//      ↓
// HC-05 Bluetooth
//      ↓ UART
// STM32F103C8T6
//    ↓       ↓
// Buzzers    OLED
//
// ======================================================


// ------------------------------------------------------
// HC-05 BLUETOOTH
// ------------------------------------------------------

#define BT Serial1

// STM32 USART1:
// PA9  = TX
// PA10 = RX

String inputCommand = "";


// ------------------------------------------------------
// BUZZERS
// ------------------------------------------------------

#define BUZZER1 PA0
#define BUZZER2 PA6

bool soundPlaying = false;

int currentFrequency = 0;


// ------------------------------------------------------
// OLED
// ------------------------------------------------------

// OLED:
// SCL -> PB6
// SDA -> PB7

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

bool oledAvailable = false;

uint8_t oledAddress = 0x3C;


// ------------------------------------------------------
// DRAWING SETTINGS
// ------------------------------------------------------

// Android app should send Y value between:
//
// 0    = top
// 1000 = bottom

#define DRAW_MIN 0
#define DRAW_MAX 1000

#define MIN_FREQUENCY 180
#define MAX_FREQUENCY 1200

String displayMode = "STOPPED";

unsigned long lastDisplayUpdate = 0;

float animationPhase = 0;


// ======================================================
// CHECK OLED
// ======================================================

bool checkOLEDAddress(uint8_t address)
{
  Wire.beginTransmission(address);

  byte error = Wire.endTransmission();

  return error == 0;
}


// ======================================================
// INITIALIZE OLED
// ======================================================

void initializeOLED()
{
  oledAvailable = false;

  if (checkOLEDAddress(0x3C))
  {
    oledAddress = 0x3C;
    oledAvailable = true;
  }

  else if (checkOLEDAddress(0x3D))
  {
    oledAddress = 0x3D;
    oledAvailable = true;
  }


  // If OLED is disconnected/damaged,
  // SoundCanvas will still work.
  if (!oledAvailable)
  {
    return;
  }


  u8g2.setI2CAddress(oledAddress << 1);

  u8g2.begin();


  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.drawStr(
    20,
    18,
    "SOUNDCANVAS"
  );

  u8g2.drawStr(
    55,
    34,
    "V1"
  );

  u8g2.drawStr(
    18,
    54,
    "Bluetooth Ready"
  );

  u8g2.sendBuffer();

  delay(1500);
}


// ======================================================
// PLAY FREQUENCY
// ======================================================

void playFrequency(int frequency)
{
  frequency = constrain(
    frequency,
    50,
    3000
  );


  currentFrequency = frequency;

  soundPlaying = true;


  // Set PWM frequency

  analogWriteFrequency(frequency);


  // Both buzzers play same frequency

  analogWrite(
    BUZZER1,
    128
  );

  analogWrite(
    BUZZER2,
    128
  );
}


// ======================================================
// STOP SOUND
// ======================================================

void stopSound()
{
  analogWrite(
    BUZZER1,
    0
  );

  analogWrite(
    BUZZER2,
    0
  );


  soundPlaying = false;

  currentFrequency = 0;

  displayMode = "STOPPED";
}


// ======================================================
// DRAW VALUE -> FREQUENCY
// ======================================================

int drawToFrequency(int y)
{
  y = constrain(
    y,
    DRAW_MIN,
    DRAW_MAX
  );


  // Top of screen = high pitch
  // Bottom = low pitch

  int frequency = map(
    y,
    DRAW_MIN,
    DRAW_MAX,
    MAX_FREQUENCY,
    MIN_FREQUENCY
  );


  return frequency;
}


// ======================================================
// NOTE -> FREQUENCY
// ======================================================

int noteToFrequency(String note)
{
  note.trim();

  note.toUpperCase();


  // ---------------- OCTAVE 4 ----------------

  if (note == "C4")  return 262;
  if (note == "C#4") return 277;
  if (note == "D4")  return 294;
  if (note == "D#4") return 311;
  if (note == "E4")  return 330;

  if (note == "F4")  return 349;
  if (note == "F#4") return 370;
  if (note == "G4")  return 392;
  if (note == "G#4") return 415;

  if (note == "A4")  return 440;
  if (note == "A#4") return 466;
  if (note == "B4")  return 494;


  // ---------------- OCTAVE 5 ----------------

  if (note == "C5")  return 523;
  if (note == "C#5") return 554;
  if (note == "D5")  return 587;
  if (note == "D#5") return 622;
  if (note == "E5")  return 659;

  if (note == "F5")  return 698;
  if (note == "F#5") return 740;
  if (note == "G5")  return 784;
  if (note == "G#5") return 831;

  if (note == "A5")  return 880;
  if (note == "A#5") return 932;
  if (note == "B5")  return 988;


  return 0;
}


// ======================================================
// OLED STOPPED SCREEN
// ======================================================

void showStopped()
{
  if (!oledAvailable)
  {
    return;
  }


  u8g2.clearBuffer();


  u8g2.setFont(
    u8g2_font_6x12_tf
  );


  u8g2.drawStr(
    20,
    13,
    "SOUNDCANVAS"
  );


  u8g2.drawHLine(
    0,
    17,
    128
  );


  u8g2.setFont(
    u8g2_font_9x15_tf
  );


  u8g2.drawStr(
    32,
    42,
    "STOPPED"
  );


  u8g2.setFont(
    u8g2_font_5x8_tf
  );


  u8g2.drawStr(
    19,
    60,
    "Waiting for phone"
  );


  u8g2.sendBuffer();
}


// ======================================================
// OLED CLEAR SCREEN
// ======================================================

void showCleared()
{
  if (!oledAvailable)
  {
    return;
  }


  u8g2.clearBuffer();


  u8g2.setFont(
    u8g2_font_6x12_tf
  );


  u8g2.drawStr(
    20,
    15,
    "SOUNDCANVAS"
  );


  u8g2.drawHLine(
    0,
    19,
    128
  );


  u8g2.drawStr(
    18,
    42,
    "CANVAS CLEARED"
  );


  u8g2.sendBuffer();
}


// ======================================================
// OLED SOUND VISUALIZER
// ======================================================

void updateOLED()
{
  if (!oledAvailable)
  {
    return;
  }


  if (!soundPlaying)
  {
    return;
  }


  // Update every 50 ms

  if (
    millis() - lastDisplayUpdate < 50
  )
  {
    return;
  }


  lastDisplayUpdate = millis();


  u8g2.clearBuffer();


  // ---------------- TITLE ----------------

  u8g2.setFont(
    u8g2_font_5x8_tf
  );


  u8g2.drawStr(
    20,
    8,
    "SOUNDCANVAS V1"
  );


  u8g2.drawHLine(
    0,
    11,
    128
  );


  // ---------------- MODE ----------------

  u8g2.setCursor(
    2,
    21
  );

  u8g2.print(
    displayMode
  );


  // ---------------- FREQUENCY ----------------

  u8g2.setCursor(
    73,
    21
  );

  u8g2.print(
    currentFrequency
  );

  u8g2.print(
    "Hz"
  );


  // ---------------- VISUALIZER ----------------

  animationPhase += 0.3;


  for (
    int x = 0;
    x < 128;
    x += 6
  )
  {
    float wave =
      sin(
        animationPhase +
        x * 0.15
      );


    if (wave < 0)
    {
      wave = -wave;
    }


    int height =
      5 +
      (int)(
        wave * 30
      );


    u8g2.drawBox(
      x,
      63 - height,
      4,
      height
    );
  }


  u8g2.sendBuffer();
}


// ======================================================
// PROCESS COMMAND
// ======================================================

void processCommand(String command)
{
  command.trim();


  if (command.length() == 0)
  {
    return;
  }


  // ==================================================
  // STOP
  // ==================================================

  if (
    command.equalsIgnoreCase(
      "STOP"
    )
  )
  {
    stopSound();

    showStopped();

    BT.println(
      "STOPPED"
    );

    return;
  }


  // ==================================================
  // CLEAR
  // ==================================================

  if (
    command.equalsIgnoreCase(
      "CLEAR"
    )
  )
  {
    stopSound();

    displayMode =
      "CLEAR";

    showCleared();

    BT.println(
      "CANVAS CLEARED"
    );

    return;
  }


  // ==================================================
  // DRAW
  //
  // Example:
  //
  // DRAW:500
  //
  // or
  //
  // DRAW:300,500
  //
  // X is ignored.
  // Y controls pitch.
  // ==================================================

  if (
    command.startsWith(
      "DRAW:"
    )
  )
  {
    String data =
      command.substring(5);


    int comma =
      data.indexOf(',');


    int y;


    if (comma >= 0)
    {
      y =
        data
        .substring(
          comma + 1
        )
        .toInt();
    }

    else
    {
      y =
        data.toInt();
    }


    y = constrain(
      y,
      0,
      1000
    );


    int frequency =
      drawToFrequency(y);


    displayMode =
      "DRAW";


    playFrequency(
      frequency
    );


    return;
  }


  // ==================================================
  // FREQUENCY COMMAND
  //
  // FREQ:440
  // ==================================================

  if (
    command.startsWith(
      "FREQ:"
    )
  )
  {
    int frequency =
      command
      .substring(5)
      .toInt();


    if (
      frequency >= 50 &&
      frequency <= 3000
    )
    {
      displayMode =
        "FREQ";


      playFrequency(
        frequency
      );


      BT.print(
        "PLAYING:"
      );

      BT.println(
        frequency
      );
    }


    return;
  }


  // ==================================================
  // NOTE COMMAND
  //
  // NOTE:C4
  // ==================================================

  if (
    command.startsWith(
      "NOTE:"
    )
  )
  {
    String note =
      command.substring(5);


    note.trim();

    note.toUpperCase();


    int frequency =
      noteToFrequency(
        note
      );


    if (
      frequency > 0
    )
    {
      displayMode =
        note;


      playFrequency(
        frequency
      );


      BT.print(
        "NOTE:"
      );

      BT.println(
        note
      );
    }


    return;
  }


  // ==================================================
  // RAW FREQUENCY
  //
  // Example:
  //
  // 440
  // ==================================================

  bool numberOnly =
    true;


  for (
    unsigned int i = 0;
    i < command.length();
    i++
  )
  {
    if (
      !isDigit(
        command[i]
      )
    )
    {
      numberOnly =
        false;

      break;
    }
  }


  if (numberOnly)
  {
    int frequency =
      command.toInt();


    if (
      frequency >= 50 &&
      frequency <= 3000
    )
    {
      displayMode =
        "FREQ";


      playFrequency(
        frequency
      );
    }
  }
}


// ======================================================
// READ HC-05
// ======================================================

void readBluetooth()
{
  while (
    BT.available()
  )
  {
    char c =
      BT.read();


    // Command complete

    if (
      c == '\n' ||
      c == '\r'
    )
    {
      if (
        inputCommand.length() > 0
      )
      {
        processCommand(
          inputCommand
        );


        inputCommand =
          "";
      }
    }

    else
    {
      inputCommand += c;


      // Prevent excessive data

      if (
        inputCommand.length() > 50
      )
      {
        inputCommand =
          "";
      }
    }
  }
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  // ---------------- BUZZERS ----------------

  pinMode(
    BUZZER1,
    OUTPUT
  );

  pinMode(
    BUZZER2,
    OUTPUT
  );


  analogWrite(
    BUZZER1,
    0
  );

  analogWrite(
    BUZZER2,
    0
  );


  // ---------------- HC-05 ----------------

  BT.begin(
    9600
  );


  // ---------------- OLED I2C ----------------

  Wire.setSCL(
    PB6
  );

  Wire.setSDA(
    PB7
  );

  Wire.begin();


  delay(
    300
  );


  initializeOLED();


  if (oledAvailable)
  {
    showStopped();
  }


  // Tell connected device
  // that STM32 is ready

  BT.println(
    "SOUNDCANVAS V1 READY"
  );
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop()
{
  // Read phone commands

  readBluetooth();


  // Update OLED visualizer

  updateOLED();
}