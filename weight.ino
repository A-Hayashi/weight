#include <Ticker.h>

#define LED 12
#define SW 13
#define WEIGHT 14

Ticker Ticker1;
Ticker Ticker2;

static uint32_t bh = 0;
static uint32_t bl = 0;
static uint32_t cnt = 0;
#define TIMEOUT_MS 30

int start_time = 0;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SW, INPUT);
  pinMode(WEIGHT, INPUT);
  Serial.begin(115200);
  weight_init();
}

void loop() {
  //  LED_on(SW_on());
}

static void itob(char *s, int n, unsigned long long v) {
  if (n > 0) {
    unsigned long long f = 1ULL << (n - 1);
    do
      *s++ = f & v ? '1' : '0';
    while (f >>= 1, --n);
  } else if (n != 0) {
    char *b = s;
    do
      *s++ = '0' + v % 2;
    while ((v >>= 1) && ++n);
    for (char c, *e = s - 1; b < e; b++, e--)
      c = *b, *b = *e, *e = c;
  }
  *s = '\0';
}

static void eee(uint32_t bh, uint32_t bl) {
  uint8_t stable = (bh >> 18) & 0x03;
  if (stable == 0x03) {
    detachInterrupt(WEIGHT);
    Ticker2.once_ms(5000, restart);
    LED_on(true);
  } else {
    LED_toggle();
  }

  char s[64];
  itob(s, 2, (unsigned long long int) stable);
  Serial.write(s);
  Serial.print(" ");

  itob(s, 32, bh);
  Serial.write(s);
  Serial.print(" ");

  itob(s, 32, bl);
  Serial.write(s);
  Serial.println(" ");

  if (stable == 0x00) {
    //    weight_result(0, 0);
  } else if (stable == 0x01) {
    //    weight_result(1, 0);
  } else if (stable == 0x03) {
    float weight = (bh & 0xffff) / (float) 10;
    //    weight_result(2, weight);
    Serial.print(weight);
    Serial.println(" kg");
  } else {
  }
}

static void restart()
{
  attachInterrupt(WEIGHT, rise, RISING);
  LED_on(false);
}

static void timeout() {
  detachInterrupt(WEIGHT);
  uint32_t bh_disp = bh;
  uint32_t bl_disp = bl;
  uint32_t cnt_disp = cnt;
  bh = 0;
  bl = 0;
  cnt = 0;
  attachInterrupt(WEIGHT, rise, RISING);

  if (cnt_disp == 39) {
    eee(bh_disp, bl_disp);
  }
}


static void rise() {
  detachInterrupt(WEIGHT);
  start_time = micros();
  attachInterrupt(WEIGHT, fall, FALLING);
}

static void fall() {
  detachInterrupt(WEIGHT);
  Ticker1.once_ms(TIMEOUT_MS, timeout);
  uint32_t pulse = micros() - start_time;
  uint8_t bit = pulse < 750 ? 1 : 0;
  if (cnt < 32) {
    bh |= bit << (31 - cnt);
  } else if (cnt < 64) {
    bl |= bit << (63 - cnt);
  }
  cnt++;
  attachInterrupt(WEIGHT, rise, RISING);
}

void weight_init() {
  attachInterrupt(WEIGHT, rise, RISING);
  LED_on(false);
}

bool SW_on()
{
  if (  digitalRead(SW) == HIGH) {
    return false;
  } else {
    return true;
  }
}

void LED_on(bool on)
{
  if (on == true) {
    digitalWrite(LED, LOW);
  } else {
    digitalWrite(LED, HIGH);
  }
}


void LED_toggle()
{
  static bool state = true;
  state = !state;
  LED_on(state);
}



