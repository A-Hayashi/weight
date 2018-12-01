#include <Ticker.h>
#include <Wire.h>

#define LED 12
#define SW 13
#define WEIGHT 14

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SW, INPUT);
  pinMode(WEIGHT, INPUT);
  Serial.begin(9600);
  weight_init();
  i2c_init();
}

void loop() {
  //  LED_on(SW_on());
}

/************************************************/
#define TIMEOUT_MS 30

typedef struct {
  byte stable;
  float weight;
} weight_t;

static weight_t weight_st;

static Ticker Ticker1;
static Ticker Ticker2;

static uint32_t bh = 0;
static uint32_t bl = 0;
static uint32_t cnt = 0;
static int start_time = 0;

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
    weight_st.stable = 0;
    weight_st.weight = 0;
  } else if (stable == 0x01) {
    weight_st.stable = 1;
    weight_st.weight = 0;
  } else if (stable == 0x03) {
    float weight = (bh & 0xffff) / (float) 10;
    weight_st.stable = 2;
    weight_st.weight = weight;
    Serial.print(weight);
    Serial.println(" kg");
  } else {
  }
}

static void restart()
{
  weight_st.stable = 0xff;
  weight_st.weight = 0;
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
  weight_st.stable = 0xff;
  weight_st.weight = 0;
  attachInterrupt(WEIGHT, rise, RISING);
  LED_on(false);
}

/************************************************/
static bool SW_on()
{
  if (  digitalRead(SW) == HIGH) {
    return false;
  } else {
    return true;
  }
}

static void LED_on(bool on)
{
  if (on == true) {
    digitalWrite(LED, LOW);
  } else {
    digitalWrite(LED, HIGH);
  }
}


static void LED_toggle()
{
  static bool state = true;
  state = !state;
  LED_on(state);
}

/************************************************/

void i2c_init()
{
  Wire.begin(0x30) ;                 // Ｉ２Ｃの初期化、自アドレスを10とする
  Wire.onRequest(requestEvent);     // マスタからのデータ取得要求のコールバック関数登録
  Wire.onReceive(receiveEvent);     // マスタからのデータ送信対応のコールバック関数登録
  Serial.println("i2c slave test");
}

// マスターからを受信
void receiveEvent(int n) {
  Serial.println("receiveEvent");
  for (int i = 0; i < n; i++) {
    if (Wire.available() > 0)  {
      byte d = Wire.read();
      Serial.print(d, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

// マスターからのリクエストに対するデータ送信
void requestEvent() {
  Serial.println("requestEvent");
  byte *p = (byte*)(&weight_st);
  for (int i = 0; i < sizeof(weight_st); i++) {
    Wire.write(p[i]);
  }
}





