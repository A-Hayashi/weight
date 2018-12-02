#include <Wire.h>
#include <MsTimer2.h>

#define PIN 3
#define LED 2
#define  SW 4

#define TIMEOUT_US 30000

typedef struct {
  uint32_t stable;
  float weight;
} weight_t;

static weight_t weights;

void weights_set(uint32_t stable, float weight)
{
  weights.stable = stable;
  weights.weight = weight;
}

void weight_init()
{
  MsTimer2::stop();
  weights_set(0xff, 0);
  LED_on(false);
}

int read_bits(uint32_t *bh, uint32_t *bl) {
  int count = 0;
  uint64_t ret = 0;

  while (true) {
    unsigned long pulse = pulseIn(PIN, HIGH, TIMEOUT_US);
    if (pulse == 0) break;

    uint32_t bit = pulse < 750 ? 1 : 0;
    if (count < 32) {
      bitWrite(*bh, 31 - count, bit);
    } else if (count < 64) {
      bitWrite(*bl, 63 - count, bit);
    }
    ++count;
  }
  return count;
}

void setup() {
  pinMode(PIN, INPUT);
  pinMode(SW, INPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  weight_init();
  i2c_init();
}

void loop() {
  uint32_t bh = 0;
  uint32_t bl = 0;
  int count = read_bits(&bh, &bl);
  if (count == 39) {
    eee(bh, bl);
  }
}

static void eee(uint32_t bh, uint32_t bl) {
  uint32_t stable = (bh >> 18) & 0x03;
  LED_toggle();

  char s[64];
  itob(s, 2, stable);
  Serial.write(s);
  Serial.print(" ");

  itob(s, 32, bh);
  Serial.write(s);
  Serial.print(" ");

  itob(s, 32, bl);
  Serial.write(s);
  Serial.println(" ");

  if (stable == 0x00) {
    weights_set(stable, 0);
  } else if (stable == 0x01) {
    weights_set(stable, 0);
  } else if (stable == 0x03) {
    float weight = (bh & 0xffff) / (float) 10;
    Serial.print(weight);
    Serial.println(" kg");
    LED_on(true);
    weights_set(stable, weight);
    MsTimer2::set(5000, weight_init);
    MsTimer2::start();
  } else {
  }
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

/************************************************************/
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
/************************************************************/


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
  byte *p = (byte*)(&weights);
  for (int i = 0; i < sizeof(weights); i++) {
    Wire.write(p[i]);
  }
}
