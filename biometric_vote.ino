#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Adafruit_SSD1306.h>
#include <WiFiS3.h>

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- Fingerprint ----------------
#define FINGER_RX 2
#define FINGER_TX 3
SoftwareSerial fingerSerial(FINGER_RX, FINGER_TX);
Adafruit_Fingerprint finger(&fingerSerial);

// ---------------- Push Button ----------------
#define BUTTON_PIN 4

// ---------------- WiFi ----------------
char ssid[] = "Indushree’s iPhone";
char pass[] = "tiflabs@123";
WiFiServer server(80);

// ---------------- Votes ----------------
int voteA = 0;
int voteB = 0;
bool voted[7] = {false, false, false, false, false, false, false}; // index = Finger ID (1–6)

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  fingerSerial.begin(57600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found!");
    while (1);
  }
  display.clearDisplay();
  display.display();
  displayMessage("Initializing...");

  // Fingerprint init
  if (!finger.verifyPassword()) {
    displayMessage("Sensor not found!");
    Serial.println("❌ Fingerprint sensor not found!");
    while (1);
  }
  displayMessage("Ready to Vote");
  Serial.println("✅ Fingerprint sensor ready!");

  // WiFi init
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("✅ Connected to WiFi");
  Serial.print("Server IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

// ---------------- Loop ----------------
void loop() {
  int id = getFingerprintID();

  if (id >= 0) {
    if (voted[id]) {
      displayMessage("ID " + String(id) + "\nAlready Voted!");
      delay(2000);
      displayMessage("Ready to Vote");
      return;
    }

    displayMessage("ID: " + String(id) + "\nPress Button");

    // Wait for button press
    while (digitalRead(BUTTON_PIN) == HIGH) {
      delay(50);
    }

    castVote(id);
  }

  handleWebServer();
}

// ---------------- Functions ----------------
void castVote(int id) {
  if (id == 1 || id == 2) {
    voteA++;
  } else if (id >= 3 && id <= 6) {
    voteB++;
  } else {
    displayMessage("Unknown ID!");
    return;
  }

  voted[id] = true; // prevent duplicate voting
  displayMessage("Vote Casted!\nID: " + String(id));
  Serial.println("Vote casted! ID: " + String(id));
  delay(2000);
  displayMessage("Ready to Vote");
}

void displayMessage(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(msg);
  display.display();
}

int getFingerprintID() {
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) return;

  while (!client.available()) delay(1);

  String req = client.readStringUntil('\r');
  client.flush();

  String html = "<!DOCTYPE html><html><head><title>Voting Machine</title></head><body>";
  html += "<h2>Live Voting Results</h2>";
  html += "<p>Candidate A: " + String(voteA) + "</p>";
  html += "<p>Candidate B: " + String(voteB) + "</p>";
  html += "</body></html>";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
}
