#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

const char* ssid = "ESP32_Rover_Control";
const char* password = "12345678";
unsigned long lastUpdateTime = 0;
bool motorRunning = false;

WebServer server(80);

//streams the spiffs file into the server

void handleRoot() {
  File file = SPIFFS.open("/Joystick.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

//collects the x and y vals from the http GET request

void direction() {
  int x = server.arg("x").toInt();  // Horizontal joystick input
  int y = server.arg("y").toInt();  // Vertical joystick input
  if (x == 0 && y == 0) {
  // stop motors
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  server.send(200, "text/plain", "stopped");
  return;
}
  lastUpdateTime = millis();
  motorRunning = true;

  int left_motor  = y + x;
  int right_motor = y - x;

    // restrains values to -255 to 255
  left_motor = constrain(left_motor, -255, 255);  
  right_motor = constrain(right_motor, -255, 255);

    // LEFT MOTOR: GPIO 15 = direction, GPIO 2 = PWM 
    if (left_motor < 0) {
      digitalWrite(15, HIGH);  // Reverse
    } 
    else {
      digitalWrite(15, LOW);   // Forward
    }
    ledcWrite(0, min(abs(left_motor), 255));

    // RIGHT MOTOR: GPIO 4 = direction, GPIO 16 = PWM 
    if (right_motor < 0) {
      digitalWrite(4, HIGH);   // Reverse
    } else {
      digitalWrite(4, LOW);    // Forward
    }
  ledcWrite(1, min(abs(right_motor), 255));
}

void read_data(){
  server.send(200, "text/plain", "radio=100&infared=50&magnetic=up");
}


void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  ledcSetup(0, 5000, 8); // 5kHz, 8-bit resolution
  ledcAttachPin(2, 0); // Pin 15, channel 0
  
  pinMode(4, OUTPUT);
  ledcAttachPin(16, 1); // Pin 15, channel 0
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  WiFi.softAP(ssid, password);
  Serial.println("AP started. IP address: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.on("/joystick", HTTP_GET, direction);
  server.on("/read_data", HTTP_GET, read_data);

    
  //makes sure lib open correct
  server.onNotFound([]() {
  String path = server.uri();
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    String contentType = "text/plain";

    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".css")) contentType = "text/css";

    server.streamFile(file, contentType);
    file.close();
  } 
  });


  server.begin();
}

void loop() {
  server.handleClient();
}
