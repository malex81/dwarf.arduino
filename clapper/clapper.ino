const int freqPot = A0;
const int dutyPot = A1;
const int pwmPin = 9;
const int buttonPin = 2;

// 1. Описываем структуру "Режим"
struct Mode {
  const int* intervals;  // Указатель на массив с временами
  const int length;      // Длина этого массива
};

// 2. Сами массивы времени (меняйте их длину как хотите)
const int mode0[] = { 1000 };
const int mode1[] = { 500, 300 };
const int mode2[] = { 500, 100, 200, 100, 200, 500 };
const int mode3[] = { 100, 100, 300, 400 };

// 3. Автоматически упаковываем массивы в структуру с подсчетом длины через sizeof
// Макрос С_ARRAY_LEN автоматически считает количество элементов в массиве
#define C_ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

Mode modes[] = {
  { mode0, C_ARRAY_LEN(mode0) },
  { mode1, C_ARRAY_LEN(mode1) },
  { mode2, C_ARRAY_LEN(mode2) },
  { mode3, C_ARRAY_LEN(mode3) }
};

// Автоматически считаем общее количество режимов в списке
const int totalModes = C_ARRAY_LEN(modes);

int currentMode = 0;
int currentStep = 0;
unsigned long prevTime = 0;

bool isPwmActive = true;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void setup() {
  pinMode(pwmPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  TCCR1A |= (1 << COM1A1);
  TCCR1B |= (1 << WGM13);
  TCCR1B |= (1 << CS11) | (1 << CS10);  // Предделитель 64
}

void loop() {
  // --- 1. ОБРАБОТКА КНОПКИ ---
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    if (millis() - lastDebounceTime > 50) {
      if (reading == LOW) {
        currentMode++;
        if (currentMode >= totalModes) currentMode = 0;

        currentStep = 0;
        prevTime = millis();
        isPwmActive = true;
      }
      lastDebounceTime = millis();
    }
  }
  lastButtonState = reading;

  // --- 2. ЛОГИКА ТАЙМЕРА РЕЖИМОВ (Автоматическая) ---
  unsigned long currentMillis = millis();

  // Берем длительность текущего шага из структуры
  unsigned long stepDuration = modes[currentMode].intervals[currentStep];

  if (currentMillis - prevTime >= stepDuration) {
    prevTime = currentMillis;
    currentStep++;

    // Проверка конца массива теперь использует modes[currentMode].length
    if (currentStep >= modes[currentMode].length) {
      currentStep = 0;
    }

    isPwmActive = (currentStep % 2 == 0);
  }

  // --- 3. ЧТЕНИЕ ПОТЕНЦИОМЕТРОВ И УПРАВЛЕНИЕ ШИМ ---
  int rawFreq = analogRead(freqPot);
  // Исправленный диапазон: 40 Гц (3125) до 400 Гц (313)
  unsigned int icrValue = map(rawFreq, 0, 1023, 3125, 313);

  if (ICR1 != icrValue) {
    ICR1 = icrValue;
  }

  int rawDuty = analogRead(dutyPot);
  float dutyRatio = rawDuty / 1023.0;
  float nonLinearDuty = dutyRatio * dutyRatio;

  unsigned int maxSafeDuty = (icrValue * 3) / 4;  // 75%
  unsigned int ocrValue = nonLinearDuty * maxSafeDuty;

  if (isPwmActive) {
    OCR1A = ocrValue;
  } else {
    OCR1A = 0;
  }
}
