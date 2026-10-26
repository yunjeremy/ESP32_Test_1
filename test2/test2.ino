#ifndef RGB_BUILTIN          // 보드 설정에 내장 RGB LED 핀이 없으면
#define RGB_BUILTIN 48       // 48번 핀으로 지정 (지금 동작 확인된 핀)
#endif

float brightness = 0.15;     // 전체 밝기 비율 (0.0~1.0). 15%로 낮게 설정, 눈부시면 더 낮추기

// 색을 켜는 함수: 빨강, 초록, 파랑 값(0~255)을 받아 밝기 비율을 곱해서 LED에 적용
void setColor(int r, int g, int b) {
  rgbLedWrite(RGB_BUILTIN, r * brightness, g * brightness, b * brightness);
}

void setup() {
  Serial.begin(115200);      // 시리얼 모니터 통신 속도 설정
}

void loop() {
  setColor(255, 255, 0);   Serial.println("노랑");   delay(1000);  // 노랑 켜고 1초 유지
  setColor(255, 0, 255);   Serial.println("보라");   delay(1000);  // 보라
  setColor(0, 255, 255);   Serial.println("하늘색"); delay(1000);  // 하늘색
  setColor(255, 80, 0);    Serial.println("주황");   delay(1000);  // 주황
  setColor(255, 20, 60);   Serial.println("분홍");   delay(1000);  // 분홍
}