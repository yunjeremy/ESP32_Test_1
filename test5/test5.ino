// 마이크(INMP441/MS3625) 피치 검출 테스트 — 시리얼 모니터 출력 전용 (상세 로그+시간)
// (OLED 화면 출력은 test6에서 따로 진행 예정)

#include <driver/i2s.h>
#include <math.h>

#define I2S_SCK  4   // 마이크 SCK(BCLK)
#define I2S_WS   5   // 마이크 WS(LRCL)
#define I2S_SD   6   // 마이크 SD(DOUT)
#define I2S_PORT I2S_NUM_0

#define SAMPLE_RATE 16000
#define BUFFER_LEN  1024
#define MIN_FREQ    70.0
#define MAX_FREQ    1000.0
#define SILENCE_THRESHOLD 2000.0  // 이보다 진폭이 작으면 "소리 없음" 처리 (필요하면 조정)

int32_t rawBuffer[BUFFER_LEN];
float samples[BUFFER_LEN];

const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// 모든 로그에 "[몇 ms] " 시간 접두어를 붙여서 출력
void logTime(String msg) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print("ms] ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  logTime("========================================");
  logTime("SCK 핀: GPIO" + String(I2S_SCK) + " / WS 핀: GPIO" + String(I2S_WS) + " / SD 핀: GPIO" + String(I2S_SD));

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  esp_err_t installErr = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  logTime("i2s_driver_install() 결과: " + String(installErr == ESP_OK ? "성공(ESP_OK)" : "실패! 에러코드=" + String(installErr)));

  esp_err_t pinErr = i2s_set_pin(I2S_PORT, &pin_config);
  logTime("i2s_set_pin() 결과: " + String(pinErr == ESP_OK ? "성공(ESP_OK)" : "실패! 에러코드=" + String(pinErr)));

  logTime("!! 참고: I2S는 I2C 스캔 같은 '장치 감지' 기능이 없습니다.");
  logTime("   위 결과가 '성공'이어도 마이크가 실제 연결됐는지는 보장 안 됨 - 아래 레벨 값으로 직접 판단하세요.");
  logTime("마이크 피치 검출 시작. 악기 소리를 들려주세요.");
  logTime("========================================");
}

// 주파수를 가장 가까운 음이름 + 센트 오차 문자열로 변환
void freqToNote(float freq, char* outBuf, int bufSize) {
  float midi = 69.0 + 12.0 * log2(freq / 440.0);
  int midiRound = (int)round(midi);
  float cents = (midi - midiRound) * 100.0;
  int noteIndex = ((midiRound % 12) + 12) % 12;
  int octave = (midiRound / 12) - 1;
  snprintf(outBuf, bufSize, "%s%d (%+.0fcents)", noteNames[noteIndex], octave, cents);
}

// 자기상관(autocorrelation)으로 기본 주파수 찾기
float detectPitch(float* buf, int len, int sampleRate) {
  int minLag = sampleRate / MAX_FREQ;
  int maxLag = sampleRate / MIN_FREQ;
  if (maxLag >= len) maxLag = len - 1;

  float bestCorr = 0;
  int bestLag = -1;

  for (int lag = minLag; lag <= maxLag; lag++) {
    float corr = 0;
    int n = len - lag;
    for (int i = 0; i < n; i++) {
      corr += buf[i] * buf[i + lag];
    }
    corr /= n;
    if (corr > bestCorr) {
      bestCorr = corr;
      bestLag = lag;
    }
  }

  if (bestLag <= 0) return -1;
  return (float)sampleRate / bestLag;
}

void loop() {
  size_t bytesRead = 0;
  esp_err_t readErr = i2s_read(I2S_PORT, rawBuffer, sizeof(rawBuffer), &bytesRead, portMAX_DELAY);
  int samplesRead = bytesRead / sizeof(int32_t);

  if (readErr != ESP_OK || bytesRead == 0) {
    delay(200);
    return;
  }

  // 32비트 I2S 프레임에서 24비트 데이터 추출 + DC 오프셋 제거
  float mean = 0;
  for (int i = 0; i < samplesRead; i++) {
    samples[i] = (float)(rawBuffer[i] >> 8);
    mean += samples[i];
  }
  mean /= samplesRead;

  float maxAmp = 0;
  for (int i = 0; i < samplesRead; i++) {
    samples[i] -= mean;
    if (fabs(samples[i]) > maxAmp) maxAmp = fabs(samples[i]);
  }

  if (maxAmp < SILENCE_THRESHOLD) {
    delay(200);
    return;
  }

  // 지금 들리는 주파수를 실시간으로 표시
  float freq = detectPitch(samples, samplesRead, SAMPLE_RATE);

  if (freq > 0) {
    char noteBuf[32];
    freqToNote(freq, noteBuf, sizeof(noteBuf));
    logTime("주파수: " + String(freq, 1) + "Hz  ->  " + String(noteBuf));
  }

  delay(150);
}
