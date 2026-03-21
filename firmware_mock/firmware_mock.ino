#include <Arduino.h>

/* =================== Protocolo / Mapeamento =================== */

// Ordem dos 11 bits (bit0→bit10):
// 0:A, 1:B, 2:Select, 3:Start, 4:Up, 5:Down, 6:Left, 7:Right, 8:C, 9:D, 10:Push
#define NES_BITS 11

// Pinos do protocolo (host -> ESP32: LATCH/CLOCK, ESP32 -> host: DATA)
const int PIN_LATCH = 12; // entrada
const int PIN_CLOCK = 14; // entrada
const int PIN_DATA  = 27;  // saída

// Botões digitais (GND = pressionado)
const int PIN_A      = 32;
const int PIN_B      = 33;
const int PIN_SELECT = 25;
const int PIN_START  = 26;
const int PIN_C      = 18;
const int PIN_D      = 19;
const int PIN_PUSH   = 23; // <- push-button do joystick (NOVO)

// Joystick analógico (ADC1)
const int PIN_VRX = 34; // ADC1_CH6
const int PIN_VRY = 35; // ADC1_CH7

/* =================== Ajustes de eixo (ADC) =================== */

// ADC 12 bits (0..4095)
static const int ADC_MIN = 0;
static const int ADC_MAX = 4095;

// Centro aproximado (autocalibra levemente durante uso)
int centerX = 2048;
int centerY = 2048;

const int DEADZONE = 120;
const int TRIG     = 550;
const int HYST     = 80;
const int SAMPLES  = 4;

volatile bool dpadUp = false, dpadDown = false, dpadLeft = false, dpadRight = false;
int axBuf[SAMPLES] = {0}, ayBuf[SAMPLES] = {0};
int axIdx = 0, ayIdx = 0;

// Counters for interrupts
volatile unsigned long latchCount = 0;
volatile unsigned long clockCount = 0;
volatile unsigned long lastClockCount = 0;

/* =================== Snapshot =================== */

volatile uint16_t shift_reg = 0; // 1 = pressed (interno); DATA sai invertido (ativo em 0)
volatile uint8_t  shift_idx = 0;

// Estados “recentes” (atualizados no loop)
volatile uint8_t aA=0, aB=0, aSEL=0, aSTA=0, aC=0, aD=0, aPUSH=0;

/* =================== Utilidades =================== */

// inline uint8_t readBtn(int pin) { return (digitalRead(pin) == LOW) ? 1 : 0; }
inline uint8_t readBtn(int pin) { return random(0, 2); }


int movingAvg(int *buf, int &idx, int sample) {
  buf[idx] = sample;
  idx = (idx + 1) % SAMPLES;
  long sum = 0;
  for (int i=0;i<SAMPLES;i++) sum += buf[i];
  return (int)(sum / SAMPLES);
}

// void updateDpadFromAxes(int rawX, int rawY) {
//   int dx = rawX - centerX;
//   int dy = rawY - centerY;

//   if (!dpadLeft)  { dpadLeft  = (dx < -(DEADZONE + TRIG)); }
//   else            { dpadLeft  = (dx < -(DEADZONE + TRIG - HYST)); }

//   if (!dpadRight) { dpadRight = (dx >  (DEADZONE + TRIG)); }
//   else            { dpadRight = (dx >  (DEADZONE + TRIG - HYST)); }

//   if (!dpadUp)    { dpadUp    = (dy < -(DEADZONE + TRIG)); }
//   else            { dpadUp    = (dy < -(DEADZONE + TRIG - HYST)); }

//   if (!dpadDown)  { dpadDown  = (dy >  (DEADZONE + TRIG)); }
//   else            { dpadDown  = (dy >  (DEADZONE + TRIG - HYST)); }

//   // Evitar opostos simultâneos
//   if (dpadLeft && dpadRight)  { dpadLeft  = (dx < 0); dpadRight = !dpadLeft; }
//   if (dpadUp   && dpadDown)   { dpadUp    = (dy < 0); dpadDown  = !dpadUp;   }
// }

void updateDpadFromAxes() {
  long randInt = random(0, 2);
  if (randInt == 1) {
    dpadLeft = true;
    dpadUp = true;
    dpadDown = false;
    dpadRight = false;
  } else {
    dpadLeft = false;
    dpadUp = false;
    dpadDown = true;
    dpadRight = true;
  }
}

uint16_t buildSnapshot() {
  uint16_t s = 0;
  aA = 1;
  aB = 0;
  aSEL = 0;
  aSTA = 0;
  uint8_t UP    = 1;
  uint8_t DOWN  = 0;
  uint8_t LEFT  = 1;
  uint8_t RIGHT = 0;
  aC = 1;
  aD = 1;
  aPUSH = 0;
  
  s |= (aA    << 0);
  s |= (aB    << 1);
  s |= (aSEL  << 2);
  s |= (aSTA  << 3);
  s |= (UP    << 4);
  s |= (DOWN  << 5);
  s |= (LEFT  << 6);
  s |= (RIGHT << 7);
  s |= (aC    << 8);
  s |= (aD    << 9);
  s |= (aPUSH <<10); // NOVO bit 10

  return s;
}

inline void writeDataBit(uint8_t bit) {
  // ativo-em-0: 1 (pressed) => LOW na linha DATA
  digitalWrite(PIN_DATA, bit ? LOW : HIGH);
}

/* =================== ISRs =================== */

volatile int lastLatch = HIGH;
volatile int lastClock = LOW;
volatile int v;

void IRAM_ATTR isrLatch()
{
  int v = digitalRead(PIN_LATCH);
  if (lastLatch == LOW && v == HIGH) {
    shift_reg = buildSnapshot();
    shift_idx = 0;
    writeDataBit((shift_reg >> 0) & 0x1);

    latchCount++;
    lastClockCount = clockCount;
    clockCount = 0;
  }
  lastLatch = v;
}

void IRAM_ATTR isrClock()
{ int v = digitalRead(PIN_CLOCK);
  if (lastClock == LOW && v == HIGH) {
    shift_idx++;
    if (shift_idx >= NES_BITS) {
      digitalWrite(PIN_DATA, HIGH); // repouso
    } else {
      writeDataBit((shift_reg >> shift_idx) & 0x1);
    }

    clockCount++;
  }
  lastClock = v;
}

/* =================== Setup / Loop =================== */

void setup()
{
  Serial.begin(115200);
  // Protocolo
  pinMode(PIN_LATCH, INPUT_PULLUP);
  pinMode(PIN_CLOCK, INPUT_PULLUP);
  pinMode(PIN_DATA,  OUTPUT);
  // digitalWrite(PIN_DATA, HIGH);

  // Botões
  pinMode(PIN_A,      INPUT_PULLUP);
  pinMode(PIN_B,      INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);
  pinMode(PIN_START,  INPUT_PULLUP);
  pinMode(PIN_C,      INPUT_PULLUP);
  pinMode(PIN_D,      INPUT_PULLUP);
  pinMode(PIN_PUSH,   INPUT_PULLUP);

  // ADC
  // analogReadResolution(12);
  // analogSetAttenuation(ADC_11db);
  //
  // int initX = analogRead(PIN_VRX);
  // int initY = analogRead(PIN_VRY);
  // for (int i=0;i<SAMPLES;i++){ axBuf[i]=initX; ayBuf[i]=initY; }
  // centerX = movingAvg(axBuf, axIdx, initX);
  // centerY = movingAvg(ayBuf, ayIdx, initY);
  //
  attachInterrupt(digitalPinToInterrupt(PIN_LATCH), isrLatch, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CLOCK), isrClock, CHANGE);
}

unsigned long lastCenterMs = 0;

void loop()
{
  // Botões digitais
  // Print latch and clock counts
  Serial.print("Latch count: ");
  Serial.print(latchCount);
  Serial.print(" | Clocks per last latch: ");
  Serial.println(lastClockCount);

  delay(100);
}
