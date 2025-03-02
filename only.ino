#include <PubSubClient.h>
#include <WiFi.h>
#include <Keypad.h>

#define RELAY_PIN   2 
#define ROW_NUM     4  
#define COLUMN_NUM  4  

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {13, 12, 14, 27};
byte pin_column[COLUMN_NUM] = {4, 5, 17, 18};

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String password_1 = "1234567";
const String password_2 = "987654";
const String password_3 = "55555";
String input_password;

char ssid[] = ""; //Wi-Fi Username
char pass[] = ""; //Wi-Fi Password

const char* mqtt_server = "";
const int mqtt_port = 1883;
const char* mqtt_user = "NULL";
const char* mqtt_password = "NULL";
const char* mqtt_topic = "door/state";
const char* mqtt_attempts_topic = "door/attempts";
const char* mqtt2 = "keypad/input";

WiFiClient espClient;
PubSubClient client(espClient);

int entryAttempts = 0;
const int maxAttempts = 3;    // Change this to your desired maximum attempts
const int lockoutDuration = 10; // Lockout duration in seconds

unsigned long lockoutStartTime = 0;

void setup() {
  Serial.begin(9600);
  input_password.reserve(32);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  // Connect to MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}

void loop() {
  client.loop();

  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
    publishKeytoMQTT(key);

    if (key == '*') {
      input_password = "";
    } else if (key == '#') {
      // Check if in lockout period
      if (millis() - lockoutStartTime < lockoutDuration * 1000) {
        Serial.println("Locked out. Please wait before entering the password again.");
      } else {
        entryAttempts++;
        if (input_password == password_1 || input_password == password_2 || input_password == password_3) {
          Serial.println("The password is correct, door is unlocked for 10 seconds");
          digitalWrite(RELAY_PIN, HIGH);
          client.publish(mqtt_topic, "1");
          delay(10000);

          digitalWrite(RELAY_PIN, LOW);
          Serial.println("Door Locked");
          client.publish(mqtt_topic, "0");
          resetAttempts(); // Reset the entry attempts count
        } else {
          Serial.println("The password is incorrect, try again");
          client.publish(mqtt_topic, "0");
          if (entryAttempts >= maxAttempts) {
            Serial.println("Max attempts reached. Locking out for 10 seconds.");
            lockoutStartTime = millis(); // Record the lockout start time
            resetAttempts(); // Reset the entry attempts count
          }
        }

        client.publish(mqtt_attempts_topic, String(entryAttempts).c_str()); // Publish attempts count
        
        input_password = "";
      }
    } else {
      input_password += key;
    }
  }

  // Check if the lockout period has ended
  if (millis() - lockoutStartTime >= lockoutDuration * 1000) {
  }
}

void resetAttempts() {
  entryAttempts = 0;
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Connected to MQTT");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void publishKeytoMQTT(char key) {
  String mqttMessage = "Key Pressed: ";
  mqttMessage += key;
  client.publish(mqtt2, mqttMessage.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle MQTT messages if needed
}
