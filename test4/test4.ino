// WiFi 연결 테스트 — 주변 스캔 + 연결 시도 로그 + 실패 원인까지 전부 출력

#include <WiFi.h>

#define WIFI_SSID "Iptime1"
#define WIFI_PASSWORD "dbswofla"

#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif

float brightness = 0.15;

void setColor(int r, int g, int b) {
  rgbLedWrite(RGB_BUILTIN, r * brightness, g * brightness, b * brightness);
}

String statusToText(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:     return "IDLE (대기중)";
    case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL (해당 SSID를 찾을 수 없음)";
    case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED";
    case WL_CONNECTED:       return "CONNECTED (연결 성공)";
    case WL_CONNECT_FAILED:  return "CONNECT_FAILED (비밀번호 오류 등 인증 실패)";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST (연결 끊김)";
    case WL_DISCONNECTED:    return "DISCONNECTED (연결 안 됨)";
    default:                 return "알 수 없음 (" + String(status) + ")";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("========================================");
  Serial.print("ESP32 MAC 주소: ");
  Serial.println(WiFi.macAddress());

  // 1. 주변 WiFi 스캔 (ESP32는 2.4GHz만 보임)
  Serial.println("\n주변 WiFi 스캔 중...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("주변에 아무 WiFi도 안 잡힘 (안테나/거리 문제일 수 있음)");
  } else {
    Serial.print(n);
    Serial.println("개 네트워크 발견:");
    bool targetFound = false;
    for (int i = 0; i < n; i++) {
      Serial.print("  ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.print("dBm, 채널: ");
      Serial.print(WiFi.channel(i));
      Serial.print(", 암호화: ");
      Serial.print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "없음" : "있음");
      Serial.println(")");
      if (WiFi.SSID(i) == WIFI_SSID) targetFound = true;
    }
    Serial.print("\n타겟 SSID('");
    Serial.print(WIFI_SSID);
    Serial.print("') 스캔 목록에 ");
    Serial.println(targetFound ? "있음" : "없음 (5GHz 전용/SSID 오타/범위 밖일 수 있음)");
  }

  // 2. 연결 시도
  Serial.println("\n----------------------------------------");
  Serial.print("WiFi 연결 시도: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print("[");
    Serial.print((millis() - startTime) / 1000.0, 1);
    Serial.print("s] 상태: ");
    Serial.println(statusToText(WiFi.status()));
    tries++;
  }

  Serial.println("----------------------------------------");
  if (WiFi.status() == WL_CONNECTED) {
    setColor(0, 0, 255);
    Serial.println("최종 결과: 연결 성공!");
    Serial.print("IP 주소: ");
    Serial.println(WiFi.localIP());
    Serial.print("신호 강도(RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println("dBm");
  } else {
    setColor(255, 0, 0);
    Serial.println("최종 결과: 연결 실패");
    Serial.print("최종 상태 코드: ");
    Serial.println(statusToText(WiFi.status()));
  }
  Serial.println("========================================");
}

void loop() {
}
