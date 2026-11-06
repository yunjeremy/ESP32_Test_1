// OLED(SSD1306) 연결 테스트 — I2C 스캔(상세 로그+시간) + 화면 출력 + 내장 LED 블링크(로그 없음)
// 라이브러리 필요: Adafruit SSD1306, Adafruit GFX (아두이노 IDE 라이브러리 매니저에서 설치)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#ifndef RGB_BUILTIN          // 보드 설정에 내장 RGB LED 핀이 없으면
#define RGB_BUILTIN 48       // 48번 핀으로 지정 (test2에서 동작 확인된 핀)
#endif

float brightness = 0.15;     // 전체 밝기 비율 (0.0~1.0)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool oledReady = false;

// 모든 로그에 "[몇 ms] " 시간 접두어를 붙여서 출력
void logTime(String msg) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print("ms] ");
  Serial.println(msg);
}

// 색을 켜는 함수: 빨강, 초록, 파랑 값(0~255)을 받아 밝기 비율을 곱해서 LED에 적용
void setColor(int r, int g, int b) {
  rgbLedWrite(RGB_BUILTIN, r * brightness, g * brightness, b * brightness);
}

// 핀 하나를 INPUT_PULLUP으로 설정해서 전기적으로 LOW에 눌려있는지(쇼트/오배선) 확인
void checkPinState(int pin, const char* name) {
  pinMode(pin, INPUT_PULLUP);
  delay(10);
  int state = digitalRead(pin);
  logTime(String(name) + " (GPIO" + String(pin) + ") 상태: " +
          (state == HIGH ? "HIGH (정상 - 풀업대로 떠있음)" : "LOW (!! 뭔가에 눌려있음 - 쇼트/오배선 의심)"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  logTime("========================================");
  logTime("SDA 핀: GPIO" + String(SDA_PIN) + " / SCL 핀: GPIO" + String(SCL_PIN));

  // 0. Wire.begin() 전에 핀 자체 전기 상태 먼저 체크
  logTime("--- 핀 상태 체크 (Wire.begin 전) ---");
  checkPinState(SDA_PIN, "SDA");
  checkPinState(SCL_PIN, "SCL");
  logTime("-----------------------------------");

  bool wireOk = Wire.begin(SDA_PIN, SCL_PIN);
  logTime("Wire.begin() 결과: " + String(wireOk ? "성공" : "실패"));
  Wire.setClock(100000);
  logTime("I2C 클럭: 100kHz로 설정");
  delay(100);

  // 1. I2C 스캔: 연결된 장치 주소 전부 확인
  logTime("I2C 스캔 시작...");
  unsigned long scanStart = millis();
  byte found = 0;
  byte oledAddr = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte err = Wire.endTransmission();
    if (err == 0) {
      logTime("  -> 장치 발견: 0x" + String(addr, HEX));
      oledAddr = addr;
      found++;
    }
  }
  logTime("스캔 소요 시간: " + String(millis() - scanStart) + "ms, 총 " + String(found) + "개 장치 발견");

  bool forcedAttempt = false;
  if (found == 0) {
    logTime("!! 장치를 하나도 못 찾음 -> 배선(VCC/GND/SDA/SCL) 또는 핀 번호부터 재확인하세요.");
    logTime("   (기본 주소 0x3C로 강제 초기화를 한 번 시도해봅니다 - 참고용, 신뢰하지 마세요)");
    oledAddr = 0x3C;
    forcedAttempt = true;
  }

  // 2. OLED 초기화 시도
  logTime("OLED 초기화 시도 (주소 0x" + String(oledAddr, HEX) + ")...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddr)) {
    logTime("!! OLED 초기화 실패 (display.begin()이 false 반환)");
    logTime("   -> 전원(VCC=3V3, GND), 또는 주소가 0x3C가 아닐 가능성(0x3D 모듈도 있음)을 의심해보세요.");
    oledReady = false;
  } else {
    if (forcedAttempt) {
      logTime("OLED 초기화 성공(!) 이라고 뜸 - 근데 스캔에서 장치를 못 찾았으므로 신뢰 불가. 실제로는 연결 안 됐을 가능성 높음.");
    } else {
      logTime("OLED 초기화 성공! (스캔에서도 정상 발견됨 - 신뢰 가능)");
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 8);
    display.println("Welcome to");
    display.println("");
    display.println(" ETboard");
    display.display();
    logTime("OLED에 화면 출력 명령 전송 완료");
    oledReady = true;
  }
  logTime("========================================");
}

void loop() {
  static unsigned long lastBeat = 0;
  if (millis() - lastBeat > 10000) {
    logTime("[살아있음] loop 실행 중... OLED 상태: " + String(oledReady ? "정상" : "실패"));
    lastBeat = millis();
  }

  // 색 5가지를 0.5초 켜짐 / 0.5초 꺼짐으로 블링크 (로그 없음)
  int colors[5][3] = {
    {255, 90, 90},
    {255, 230, 80},
    {90, 230, 120},
    {90, 160, 255},
    {190, 120, 255}
  };

  for (int i = 0; i < 5; i++) {
    setColor(colors[i][0], colors[i][1], colors[i][2]);
    delay(500);
    setColor(0, 0, 0);
    delay(500);
  }
}
