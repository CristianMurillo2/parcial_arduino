// Pines de los pulsadores
int startButtonPin = 2;  // Pulsador para iniciar toma de datos
int stopButtonPin = 3;   // Pulsador para detener toma de datos
int sensorPin = A0;      // Pin generador de funciones
bool collectingData = false; //controlar el estado de los pulsadores
// Puntero para almacenar datos dinámicamente
int* dataArray = nullptr;
int dataSize = 0;// Cantidad de datos almacenados actualmente
// Variables para la amplitud y frecuencia
int maxValue = 0;
int minValue = 1023; // El valor máximo de una entrada analógica es 1023
unsigned long lastCrossingTime = 0; // Tiempo del último cruce
unsigned long period = 0;  // Período de la señal

void setup() {
    pinMode(startButtonPin, INPUT);
    pinMode(stopButtonPin, INPUT);
    Serial.begin(9600);
}

void loop() {
    handleButtons();
    if (collectingData) {
        collectData();
    }
}
// Función para manejar los botones
void handleButtons() {
    int startButtonState = digitalRead(startButtonPin);
    int stopButtonState = digitalRead(stopButtonPin);

    if (startButtonState == HIGH && !collectingData) {
        startDataCollection();
    }

    if (stopButtonState == HIGH && collectingData) {
        stopDataCollection();
    }
}

// Función para iniciar la toma de datos
void startDataCollection() {
    collectingData = true;
    dataSize = 0;          // Reiniciar el tamaño de datos almacenados
    maxValue = 0;          // Reiniciar valor máximo
    minValue = 1023;       // Reiniciar valor mínimo
    lastCrossingTime = 0;  // Reiniciar tiempo del último cruce
    period = 0;            // Reiniciar período
}

// Función para detener la toma de datos
void stopDataCollection() {
    collectingData = false;
    int amplitude = calculateAmplitude();
    Serial.println(amplitude);
    float frequency = calculateFrequency();
    /*
    if (frequency > 0) {
        Serial.print("Frecuencia: ");
        Serial.print(frequency);
        Serial.println(" Hz");
    } else {
        Serial.println("No se pudo determinar la frecuencia.");
    }
    */ //pruebas para la frecuencia
    // Liberar la memoria asignada
    free(dataArray);
    dataArray = nullptr; // Evitar que el puntero apunte a una memoria liberada
}

// Función para capturar los datos
void collectData() {
    int sensorValue = analogRead(sensorPin); // Leer el valor del sensor
    // Actualizar valores máximo y mínimo
    if (sensorValue > maxValue) maxValue = sensorValue;
    if (sensorValue < minValue) minValue = sensorValue;
    // Detectar cruce por el valor medio
    detectZeroCrossing(sensorValue);
    // Almacenar el nuevo dato en memoria dinámica
    storeData(sensorValue);// Variables para controlar el estado de los pulsadores
    delay(10);// Pequeño retraso para no sobrecargar el buffer
}

// Función para detectar cruces por el valor medio y calcular el periodo
void detectZeroCrossing(int sensorValue) {
    int threshold = (maxValue + minValue) / 2;
    if (sensorValue >= threshold && lastCrossingTime == 0) {
        lastCrossingTime = millis(); // Primer cruce
    } else if (sensorValue >= threshold && millis() - lastCrossingTime > 10) {
        unsigned long currentTime = millis();
        period = currentTime - lastCrossingTime; // Calcular el período
        lastCrossingTime = currentTime;
    }
}

// Función para almacenar datos dinámicamente
void storeData(int sensorValue) {
    int* tempArray = (int*) realloc(dataArray, (dataSize + 1) * sizeof(int));

    if (tempArray != nullptr) {
        dataArray = tempArray;
        dataArray[dataSize] = sensorValue; // Almacenar el nuevo dato
        dataSize++; // Incrementar el tamaño del arreglo
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
        return 1000.0 / period; // Convertir a Hz
    }
    return 0;
}

