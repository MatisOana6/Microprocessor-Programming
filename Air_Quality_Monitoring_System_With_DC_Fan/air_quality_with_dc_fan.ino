#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Initializarea obiectului pentru ecran OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pinul pentru senzorul de gaz
int SENSOR_PIN = 34;
#define DHT_PIN 4
#define DHT_TYPE DHT11

// Variabile pentru nivelul de gaz si calitatea aerului
int gasLevel = 0;
String quality = "";
DHT dht(DHT_PIN, DHT_TYPE);

const char *ssid = "AirQuality";
const char *password = "airquality1234";

AsyncWebServer server(80);

// Pinii si proprietatile pentru controlul motorului DC
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;

const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

// Functia pentru afisarea valorilor de la senzor pe ecran OLED
void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 43);
  display.println("Temp  :");
  display.setCursor(80, 43);
  display.println(t);
  display.setCursor(114, 43);
  display.println("C");
  display.setCursor(0, 56);
  display.println("RH    :");
  display.setCursor(80, 56);
  display.println(h);
  display.setCursor(114, 56);
  display.println("%");
  display.setCursor(0, 30);
  display.print("Gas   : ");
  display.setCursor(80, 30);
  display.print(gasLevel);
  display.setCursor(104, 30);
  display.print("PPM");
}

// Functia pentru citirea nivelului de gaz si evaluarea calitatii aerului
void air_sensor()
{
  gasLevel = analogRead(SENSOR_PIN);

  if (gasLevel < 200)
  {
    quality = "  Good!";
  }
  else if (gasLevel > 200 && gasLevel < 300)
  {
    quality = "  Poor!";
  }
  else if (gasLevel > 300 && gasLevel < 500)
  {
    quality = "  Bad!";
  }
  else if (gasLevel > 500 && gasLevel < 1000)
  {
    quality = "Deadly!";
  }
  else
  {
    quality = "Toxic!";
  }

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(1, 5);
  display.print("Air Quality: ");
  display.print(quality);
}

void setup()
{
   // Initializarea comunicatiei seriale si afisarea mesajelor de configurare
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // Setarea modului de intrare pentru pinul senzorului de gaz
  pinMode(SENSOR_PIN, INPUT);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c))
  {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextColor(WHITE);

 // Afisarea unui mesaj de start pe ecran OLED
  display.setTextSize(2);
  display.setCursor(50, 0);
  display.println("Air");
  display.setTextSize(2);
  display.setCursor(23, 20);
  display.println("Quality");
  display.setTextSize(2);
  display.setCursor(24, 40);
  display.println("Monitor");
  display.display();
  delay(1200);
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("By Oana");
  display.display();
  delay(1000);
  display.clearDisplay();

 // Conectarea la reteaua WiFi
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

 // Configurarea rutelor pentru serverul web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css' integrity='sha384-B4gt1jrGC7Jh4AgTPSdUtOBvfO8sh+Wy/ZiAL8dFpOj8cO1a4IHw985l7Zb4u5+7' crossorigin='anonymous'><link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.3/css/all.min.css' integrity='sha384-0vUMnBY7fZHCSqPCrS9MrmFTgTkEvFZSKDBXvZOPO9OQGymG+1qCpY8UH8bMSnpF' crossorigin='anonymous'></head><body style='background-color: #BAF1EF;' class='text-white'><div class='container'><h1 class='display-4 text-center mt-4 mb-4'>Air Quality Monitor</h1><div class='row justify-content-center'><div class='col-md-6'><div class='card'><div class='card-body'><h3 class='card-title'><i class='fas fa-thermometer-half'></i> Temperature</h3><p class='card-text'>" + String(dht.readTemperature()) + " &deg;C</p></div></div></div><div class='col-md-6'><div class='card'><div class='card-body'><h3 class='card-title'><i class='fas fa-tint'></i> Humidity</h3><p class='card-text'>" + String(dht.readHumidity()) + "% <i class='fas fa-cloud-rain'></i></p></div></div></div><div class='col-md-6 mt-4'><div class='card'><div class='card-body'><h3 class='card-title'><i class='fas fa-cloud'></i> Gas Level</h3><p class='card-text'>" + String(gasLevel) + " PPM</p></div></div></div></div></div></body></html>";
    request->send(200, "text/html", html);
  });

  // Setarea pinilor pentru controlul motorului DC
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

 // Configurarea functionalitatilor LED PWM
  ledcSetup(pwmChannel, freq, resolution);

   // Atasarea canalului la pinul GPIO care controleaza motorul DC
  ledcAttachPin(enable1Pin, pwmChannel);

 // Pornirea serverului web
  server.begin();
}

void loop()
{ 
  // Curatarea ecranului OLED si citirea datelor de la senzor
  display.clearDisplay();
  air_sensor();
  sendSensor();

 // Verificarea nivelului de gaz si controlul motorului DC in functie de valorile date de senzorul MQ135
  if (gasLevel > 200)
  {
    // Pornirea motorului DC inainte la viteza maxima
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);

     // Ajustarea ciclului de lucru al LED PWM in functie de nivelul de gaz
    if (gasLevel > 500)
    {
      dutyCycle = 255; 
    }
    else
    {
      dutyCycle = 200; 
    }

    ledcWrite(pwmChannel, dutyCycle);
  }
  else
  {
   // Oprirea motorului DC
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    ledcWrite(pwmChannel, 0); 
  }
 // Afisarea datelor pe ecranul OLED
  display.display();
  delay(1000); 
}
