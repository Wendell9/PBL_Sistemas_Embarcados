#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configurations - editable variables
const char* default_SSID = "HORIZON"; // Wi-Fi network name
const char* default_PASSWORD = "1234567890"; // Wi-Fi network password
const char* default_BROKER_MQTT = "191.235.241.244"; // IP of the MQTT Broker
const int default_BROKER_PORT = 1883; // Port of the MQTT Broker
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp002/cmd"; // MQTT topic to listen to commands
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp002/attrs"; // MQTT topic for sending LED state info to Broker
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp002/attrs/l"; // MQTT topic for sending luminosity data to Broker
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp002/attrs/t"; // MQTT topic for sending temperature data to Broker
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp002/attrs/h"; // MQTT topic for sending humidity data to Broker

const char* default_ID_MQTT = "fiware_002"; // MQTT ID
const int default_D4 = 2; // Pin for onboard LED
const char* topicPrefix = "lamp002"; // Prefix for topic formation

// Editable configuration variables
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

int buzzer = 5; // Buzzer pin

#define DHTPIN 15     
#define DHTTYPE DHT11  // DHT22 sensor type (AM2302, AM2321)
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient MQTT(espClient);
char EstadoSaida = '0'; // LED state

void initSerial() {
    Serial.begin(115200); // Initializes serial communication
}

// Function to read temperature and humidity values from the DHT sensor
std::vector<float> handleUmityTemperature(){
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  std::vector<float> readings = {temperature, humidity};
  return readings;
}

// Function to initialize Wi-Fi connection
void initWiFi() {
    delay(10);
    Serial.println("------Wi-Fi Connection------");
    Serial.print("Connecting to network: ");
    Serial.println(SSID);
    Serial.println("Please wait");
    reconectWiFi();
}

// Function to initialize MQTT connection
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

// Setup function, runs once at startup
void setup() {
    dht.begin(); // Initialize DHT sensor
    InitOutput(); // Initialize LED output
    initSerial(); // Initialize Serial
    initWiFi(); // Initialize Wi-Fi
    initMQTT(); // Initialize MQTT
    delay(5000); // Wait before sending initial message
    MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Send initial LED state to broker
    std::vector<float> handleUmityTemperature(); // Declare the function
    pinMode(buzzer, OUTPUT); // Set buzzer as output
}

// Function to play sound using the buzzer when the LED is on
void tocasom(){
  if (digitalRead(D4) == HIGH){
    tone(buzzer, 261);
    delay(200);
    noTone(buzzer);
    tone(buzzer, 293);
    delay(200);
    noTone(buzzer);
    tone(buzzer, 329);
    delay(200);
    noTone(buzzer);
    tone(buzzer, 349);
    delay(200);
    noTone(buzzer);
    tone(buzzer, 392);
    delay(200);
    noTone(buzzer);
    delay(2000);
  }
}

void loop() {
    // Variables for calculating the average of temperature, humidity, and luminosity readings
    float avgtemperature = 0.0;
    float avghumidty = 0.0;
    int avgluminosity = 0;
    std::vector<float> valoresDHT;
    bool medidor = true;  // Control for the data collection loop
    unsigned long previousMillis = millis();  // Stores the last time an action was performed
    const long interval = 15000;  // 15-second interval for data collection
    unsigned long currentMillis = millis();
    int i = 0;  // Counter for the number of readings

    // Data collection loop until the 15-second interval is reached
    while (medidor) {
        currentMillis = millis(); // Updates the current time

        // Checks if the 15-second collection interval has been reached
        if (currentMillis - previousMillis >= interval) { 
            MQTT.loop();  // Keeps the MQTT connection active
            VerificaConexoesWiFIEMQTT();  // Verifies and reestablishes Wi-Fi and MQTT connections
            medidor = false;  // Ends the data collection loop
            tocasom();  // Plays sound if the LED is on
        }
        else {
            // If the interval hasn't been reached yet, continue collecting and accumulating readings
            MQTT.loop();  // Keeps the MQTT connection active
            VerificaConexoesWiFIEMQTT();  // Checks Wi-Fi and MQTT connections
            EnviaEstadoOutputMQTT();  // Sends the current LED state to the MQTT broker

            // Reads and accumulates the luminosity value
            avgluminosity += handleLuminosity();

            // Reads and accumulates temperature and humidity values
            valoresDHT = handleUmityTemperature();
            avgtemperature += valoresDHT[0];
            avghumidty += valoresDHT[1];
            
            i += 1;  // Increments the reading counter
            tocasom();  // Plays sound if the LED is on
        }
    }

    // After ending the data collection loop, verifies connections and sends the LED state
    VerificaConexoesWiFIEMQTT();
    EnviaEstadoOutputMQTT();

    // Calculates the average of temperature, humidity, and luminosity values
    avgtemperature = avgtemperature / i;
    avghumidty = avghumidty / i;
    avgluminosity = avgluminosity / i;

    // Converts and publishes the average luminosity value to the MQTT topic
    String mensagem = String(avgluminosity);
    Serial.print("Luminosity value: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());

    // Converts and publishes the average temperature value to the MQTT topic
    String mensagem2 = String(avgtemperature);
    String mensagem3 = String(avghumidty);
    Serial.print("Temperature value: ");
    Serial.println(mensagem2.c_str());
    MQTT.publish(TOPICO_PUBLISH_3, mensagem2.c_str());

    // Converts and publishes the average humidity value to the MQTT topic
    Serial.print("Humidity value: ");
    Serial.println(mensagem3.c_str());
    MQTT.publish(TOPICO_PUBLISH_4, mensagem3.c_str());

    // Keeps the MQTT connection active and plays a final sound if the LED is on
    MQTT.loop();
    tocasom();
}

// Function to reconnect Wi-Fi if disconnected
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Successfully connected to network ");
    Serial.print(SSID);
    Serial.println("Obtained IP: ");
    Serial.println(WiFi.localIP());

    // Ensure LED starts turned off
    digitalWrite(D4, LOW);
}

// Callback function for MQTT messages
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Message received: ");
    Serial.println(msg);

    // Form the pattern for topic comparison
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Compare received topic
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

// Check Wi-Fi and MQTT connections and reconnect if necessary
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

// Send LED state to MQTT broker
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- LED On");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- LED Off");
    }
    Serial.println("- LED state sent to broker!");
    delay(1000);
}

// Initialize LED output pin with blinking sequence
void InitOutput() {
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

// Reconnect to MQTT broker if disconnected
void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Trying to connect to MQTT Broker: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Successfully connected to MQTT broker!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Failed to reconnect to broker.");
            Serial.println("Retrying connection in 2 seconds");
            delay(2000);
        }
    }
}

// Handle luminosity readings
int handleLuminosity() {
    const int potPin = 34; // Pin for the luminosity sensor
    int sensorValue = analogRead(potPin);
    int luminosity = map(sensorValue, 0, 4095, 0, 100); // Map raw value to percentage
    return luminosity;
}
