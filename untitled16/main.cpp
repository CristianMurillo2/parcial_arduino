int startButtonPin = 2;  // Pin del pulsador para iniciar la toma de datos
int stopButtonPin = 4;   // Pin del pulsador para detener la toma de datos
int sensorPin = A0;      // Pin analógico para recibir datos del generador de funciones
bool collectingData = false;
bool startButtonPressed = false;
bool stopButtonPressed = false;
int* dataArray = nullptr;
int dataSize = 0;
int maxValue = 0;
int minValue = 1023;
unsigned long lastCrossingTime = 0;
unsigned long period = 0;
void setup() {
    pinMode(startButtonPin, INPUT);
    pinMode(stopButtonPin, INPUT);
    Serial.begin(9600);
}
void loop() {
    handleButtons();  // Maneja el estado de los botones
    if (collectingData) {
        collectData();  // Captura los datos si la colección está activa
    }
}

// Función para manejar los botones
void handleButtons() {
    bool currentStartButtonState = digitalRead(startButtonPin);
    bool currentStopButtonState = digitalRead(stopButtonPin);

    // Detectar el cambio de estado del botón de inicio
    if (currentStartButtonState == HIGH && !startButtonPressed) {
        startButtonPressed = true;
        if (!collectingData) {
            startDataCollection();
        }
    } else if (currentStartButtonState == LOW) {
        startButtonPressed = false;
    }

    // Detectar el cambio de estado del botón de parada
    if (currentStopButtonState == HIGH && !stopButtonPressed) {
        stopButtonPressed = true;
        if (collectingData) {
            stopDataCollection();
        }
    } else if (currentStopButtonState == LOW) {
        stopButtonPressed = false;
    }
}

// Función para iniciar la toma de datos
void startDataCollection() {
    collectingData = true;
    dataSize = 0;
    maxValue = 0;
    minValue = 1023;
    lastCrossingTime = 0;
    period = 0;
    Serial.println("Iniciando captura de datos...");
}

// Función para detener la toma de datos
void stopDataCollection() {
    collectingData = false;
    Serial.println("Captura de datos detenida.");

    int amplitude = calculateAmplitude();
    Serial.print("Amplitud: ");
    Serial.println(amplitude);

    float frequency = calculateFrequency();
    if (frequency > 0) {
        Serial.print("Frecuencia: ");
        Serial.print(frequency);
        Serial.println(" Hz");
    } else {
        Serial.println("No se pudo determinar la frecuencia.");
    }

    identifySignalType();  // Identificar el tipo de señal

    free(dataArray);  // Liberar la memoria asignada
    dataArray = nullptr; // Evitar que el puntero apunte a una memoria liberada
}

// Función para capturar los datos
void collectData() {
    int sensorValue = analogRead(sensorPin);
    if (sensorValue > maxValue) maxValue = sensorValue;
    if (sensorValue < minValue) minValue = sensorValue;
    detectZeroCrossing(sensorValue);
    storeData(sensorValue);
    delay(10);  // Ajustar el delay según la frecuencia de la señal
}

// Función para detectar cruces por el valor medio y calcular el periodo
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

// Función para almacenar datos dinámicamente
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

// Función para calcular la amplitud
int calculateAmplitude() {
    return (maxValue - minValue) / 2;
}

// Función para calcular la frecuencia
float calculateFrequency() {
    if (period > 0) {
        return 1000.0 / period;
    }
    return 0;
}

// Función para identificar el tipo de señal
void identifySignalType() {
    int sinusoidalCount = 0;
    int squareCount = 0;
    int triangularCount = 0;

    for (int i = 0; i < dataSize; i++) {
        if (isSineWave(dataArray, i)) {
            sinusoidalCount++;
        } else if (isSquareWave(dataArray, i)) {
            squareCount++;
        } else if (isTriangularWave(dataArray, i)) {
            triangularCount++;
        }
    }

    // Identificar la onda que aparece más veces
    if (sinusoidalCount > squareCount && sinusoidalCount > triangularCount) {
        Serial.println("La señal predominante es sinusoidal.");
    } else if (squareCount > sinusoidalCount && squareCount > triangularCount) {
        Serial.println("La señal predominante es digital (cuadrada).");
    } else if (triangularCount > sinusoidalCount && triangularCount > squareCount) {
        Serial.println("La señal predominante es triangular.");
    } else {
        Serial.println("No se pudo identificar la señal predominante.");
    }
}

// Función para comprobar si la señal es sinusoidal
bool isSineWave(int* array, int index) {
    if (index < 2) return false;
    // Transiciones suaves entre valores sucesivos
    return abs(array[index] - array[index - 1]) < (maxValue - minValue) / 10;
}

// Función para comprobar si la señal es digital (cuadrada)
bool isSquareWave(int* array, int index) {
    if (index < 2) return false;
    int thresholdHigh = maxValue - (maxValue - minValue) / 4;
    int thresholdLow = minValue + (maxValue - minValue) / 4;

    // La señal cuadrada cambia bruscamente entre valores altos y bajos
    return (array[index] >= thresholdHigh && array[index - 1] <= thresholdLow) ||
           (array[index] <= thresholdLow && array[index - 1] >= thresholdHigh);
}

// Función para comprobar si la señal es triangular
bool isTriangularWave(int* array, int index) {
    if (index < 2) return false;

    // La señal triangular tiene pendientes lineales en una dirección (creciente o decreciente)
    bool ascending = array[index] > array[index - 1] && array[index - 1] > array[index - 2];
    bool descending = array[index] < array[index - 1] && array[index - 1] < array[index - 2];

    return ascending || descending;
}
