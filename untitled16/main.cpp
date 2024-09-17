#include <LiquidCrystal.h>

const int startButtonPin = 6;
const int stopButtonPin = 7;
const int sensorPin = A0;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
bool collectingData = false;
bool startButtonPressed = false;
bool stopButtonPressed = false;
int* dataArray = nullptr;
int dataSize = 0;
int maxValue = 0;
int minValue = 1023;
unsigned long lastCrossingTime = 0;
unsigned long period = 0;
int amplitude = 0;
float frequency = 0;
String signalType = "No id.";
void setup() {
    pinMode(startButtonPin, INPUT_PULLUP);
    pinMode(stopButtonPin, INPUT_PULLUP);
    lcd.begin(16, 2);  // Inicializar la pantalla LCD de 16x2
    lcd.clear();
    lcd.print("Esperando...");
    Serial.begin(9600);
}
void loop() {
    handleButtons();
    if (collectingData) {
        collectData();
    } else {
        if (dataSize > 0) {
            displayResults();
            delay(2000);
            resetState();
        }
    }
}
void handleButtons() {
    bool currentStartButtonState = digitalRead(startButtonPin);
    bool currentStopButtonState = digitalRead(stopButtonPin);
    if (currentStartButtonState == LOW && !startButtonPressed) {
        startButtonPressed = true;
        if (!collectingData) {
            startDataCollection();
        }
    } else if (currentStartButtonState == HIGH) {
        startButtonPressed = false;
    }
    if (currentStopButtonState == LOW && !stopButtonPressed) {
        stopButtonPressed = true;
        if (collectingData) {
            stopDataCollection();
        }
    } else if (currentStopButtonState == HIGH) {
        stopButtonPressed = false;
    }
}
void startDataCollection() {
    collectingData = true;
    dataSize = 0;
    maxValue = 0;
    minValue = 1023;
    lastCrossingTime = 0;
    period = 0;
    lcd.clear();
    lcd.print("Capturando...");
    Serial.println("Iniciando captura de datos...");
}
void stopDataCollection() {
    collectingData = false;
    lcd.clear();
    lcd.print("Datos capturados");
    Serial.println("Captura de datos detenida.");
    amplitude = calculateAmplitude();
    frequency = calculateFrequency();
    signalType = identifySignalType();
    free(dataArray);
    dataArray = nullptr;
}
void collectData() {
    int sensorValue = analogRead(sensorPin);
    if (sensorValue > maxValue) maxValue = sensorValue;
    if (sensorValue < minValue) minValue = sensorValue;
    detectZeroCrossing(sensorValue);
    storeData(sensorValue);
    delay(10);
}
void detectZeroCrossing(int sensorValue) {
    int threshold = (maxValue + minValue) / 2;

    if (sensorValue >= threshold && lastCrossingTime == 0) {
        lastCrossingTime = millis();
    } else if (sensorValue >= threshold && millis() - lastCrossingTime > 10) {
        unsigned long currentTime = millis();
        period = currentTime - lastCrossingTime;
        lastCrossingTime = currentTime;
    }
}
void storeData(int sensorValue) {
    int* tempArray = (int*) realloc(dataArray, (dataSize + 1) * sizeof(int));
    if (tempArray != nullptr) {
        dataArray = tempArray;
        dataArray[dataSize] = sensorValue;
        dataSize++;
    } else {
        Serial.println("Error al asignar memoria.");
        collectingData = false;
    }
}
int calculateAmplitude() {
    return (maxValue - minValue) / 2;
}
float calculateFrequency() {
    if (period > 0) {
        return 1000.0 / period;
    }
    return 0;
}
String identifySignalType() {
    int sinusoidalCount = 0;
    int squareCount = 0;
    int triangularCount = 0;
    for (int i = 2; i < dataSize; i++) {
        int diff1 = dataArray[i] - dataArray[i - 1];
        int diff2 = dataArray[i - 1] - dataArray[i - 2];
        if (isSineWave(diff1, diff2)) {
            sinusoidalCount++;
        }
        else if (isSquareWave(dataArray[i], dataArray[i - 1])) {
            squareCount++;
        }
        else if (isTriangularWave(diff1, diff2)) {
            triangularCount++;
        }
    }
    if (sinusoidalCount > squareCount && sinusoidalCount > triangularCount) {
        return "Sinusoidal";
    } else if (squareCount > sinusoidalCount && squareCount > triangularCount) {
        return "Cuadrada";
    } else if (triangularCount > sinusoidalCount && triangularCount > squareCount) {
        return "Triangular";
    } else {
        return "No id.";
    }
}
void displayResults() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Amp: ");
    lcd.print(amplitude);

    lcd.setCursor(0, 1);
    lcd.print("Freq: ");
    lcd.print(frequency);
    lcd.print(" Hz");

    delay(2000); // Espera 2 segundos

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tipo: ");
    lcd.print(signalType);

    delay(2000); // Espera 2 segundos
}
bool isSineWave(int diff1, int diff2) {
    // Cambios suaves en ambas direcciones, y valores absolutos similares
    int threshold = (maxValue - minValue) / 10;
    return (abs(diff1) < threshold && abs(diff2) < threshold && (diff1 * diff2) < 0);
}
bool isSquareWave(int current, int previous) {
    int thresholdHigh = maxValue - (maxValue - minValue) / 4;
    int thresholdLow = minValue + (maxValue - minValue) / 4;
    return (current >= thresholdHigh && previous <= thresholdLow) || (current <= thresholdLow && previous >= thresholdHigh);
}
bool isTriangularWave(int diff1, int diff2) {
    // Cambios constantes en la misma dirección y patrones lineales
    return (diff1 > 0 && diff2 > 0) || (diff1 < 0 && diff2 < 0);
}
void resetState() {
    dataArray = nullptr;
    dataSize = 0;
    maxValue = 0;
    minValue = 1023;
    lastCrossingTime = 0;
    period = 0;
    amplitude = 0;
    frequency = 0;
    signalType = "No id.";
    lcd.clear();
    lcd.print("Esperando...");
}
