#include <Wire.h>             // Display
#include <Adafruit_GFX.h>     // Display
#include <Adafruit_SSD1306.h> // Display 
#include <HTTPClient.h>       // Comunicação
#include <WiFi.h>             // Conexão WiFi
#include "DHT.h"              // Sensor de Temperatura DHT11
#include <math.h>
#include "EmonLib.h"          // Calculo de corrente 

//Pino do sensor 
#define pinoSensor   36           //Sensor de Tensão
#define sensor_Rosq  35           // Sensor de Corrente AC (Rosquinha) ZMC
#define pinTermistor 34           // Medição de temperatura por divisor de tensão
#define sensor_correnteACS712 32  // Sensor de Corrente DC (ainda não qualibrado)
#define led_pin 5

//#define OLED_RESET 4  // verificar se precisamos
#define DHTPIN 4
#define DHTTYPE DHT11  // para uso do DHT precissa criar 


// VARIAVEIS E CONSTANTES
const float     Zero = 14.79;                   // erro da medida de corrente AC, corrigindo o que seria a leitura 0 do sensor

const int       tensao = 217;                   // (127 ou 217)  Tensao da rede AC 110 Volts e na verdade (127 volts)

float           Irms = 0,                       // Calculo de Corrente AC do ZMC
                localHum = 0,                   // Guarda umidade do DHT11
                localTemp = 0,                  // Guarda Temperatura do DHT11
                leitura_corrente=0.0,           // variaveis para o ACS712
                AvgAcs=0.0,                     // variaveis para o ACS712
                corrente_medidaACS712=0.0,      // variaveis para o ACS712
                tensaoEntrada = 0.0,            // Armazenar a tensão do sensor
                tensaoMedida = 0.0,             // Media da tensão
                valorR1 = 30000.0,              // Valor do Resistor 1 do divisor de tensão (TALVEZ MUDAR PARA CONSTANTE)
                valorR2 = 7500.0,               // Valor do Resistor 2 do divisor de tensão (TALVEZ MUDAR PARA CONSTANTE)
                Temp2;              // Temperatura da Bateria

// Classes                
EnergyMonitor   emon;                        // Classe EnergyMonitor usada para calculo de Corrente em regime RMS
DHT             dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire);


/****************************************************************************************
*|||||||||         Sessão para controle de comunicacao web           ||||||||||||||||||
*****************************************************************************************/
const char* ssid     = "pedro";
const char* password = "pedro123";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

WiFiServer server(80);

//Your Domain name with URL path or IP address with path
// Endereço de conexão web
String serverName = "http://message.eletrogama.online/microcontroller";
// Connection with message
String stationIdPath = "/1";


/*************************************************************************
*|||||||||        Inicialização de Componetes           ||||||||||||||||||
**************************************************************************/
TaskHandle_t Task1;


void setup(){
  

  //Init Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  //Config dos pins para leitura e envio de dados
  pinMode(led_pin, OUTPUT);  // rele
  pinMode(sensor_Rosq, INPUT);
  pinMode(pinoSensor, INPUT);
  pinMode(sensor_correnteACS712, INPUT);
  pinMode(pinTermistor, INPUT);

  xTaskCreatePinnedToCore(
                    getDisplay,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  // Sensor ZMC
  emon.current(sensor_Rosq, 39); //Pino, calibracao - Corrente Constante= 4096/105. = 39. 
  // Tenta realizar conexão do wifi
  connectWiFi();
  // Inicializa modulo de temp e umidade
  dht.begin();  
  Serial.begin(9600);
}

/*************************************************************************
*|||||||||        Funções Para Funcionamento           ||||||||||||||||||
**************************************************************************/

String sendGetRequest(String path) {
  String response = "0";
  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    String serverPath = serverName + path;
    
    http.begin(serverPath.c_str());

    int httpResponseCode = http.GET();
    
    if (httpResponseCode>0) {
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpResponseCode);
      String payload = http.getString();
      response = payload;
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  lastTime = millis();
  return response;
}


void sendPostRequest(String path, String value) {
  if(WiFi.status()== WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
  
    http.begin(client, serverName + path);
    Serial.println(serverName + path);
    // http.addHeader("accept", "application/json");         
    int httpResponseCode = http.POST(value);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    // Serial.println(payload);
    
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}


int getChargeStatus() {
   String chargeStatus = sendGetRequest("/1/charge");
   return chargeStatus.toInt(); 
   delay(1000);
}

void rele(){
  int chargeStatus = getChargeStatus();

  if (chargeStatus) {
    digitalWrite(led_pin, HIGH);
  }else { 
    digitalWrite(led_pin, LOW);
  }

  Serial.println(chargeStatus);
}

void sendTemperaturaTotem(float value) {
  sendPostRequest("/1/totem/temperature?request=" + String(value), String(value));
}

void sendHumidadeTotem(float value) {
  sendPostRequest("/1/totem/humidity?request=" + String(value), String(value));
}

void sendTemperaturaBateria(float value) {
  sendPostRequest("/1/battery/temperature?request=" + String(value), String(value));
}

void sendSensorTensao(float value) {
  sendPostRequest("/1/battery/voltage?request=" + String(value), String(value));
}

void sendSensorCorrenteACS712(float value) {
  sendPostRequest("/1/battery/current?request=" + String(value), String(value));
}

void sendSensorCorrenteZMC(float value) {
  sendPostRequest("/1/inverter/current?request=" + String(value), String(value));
}


float sensor_ZMC(void){
  // Sensor de Corrente ZMC
  float leitura = 0;                   // Calculo de Corrente AC dentro da função

  //Calcula a media da corrente fazendo 10 amostragem 
  for(int index =0; index <10; index++){
    leitura =leitura+ emon.calcIrms(1480);
    delayMicroseconds(1);
  }
  leitura = leitura /10.0;
  
  //Tratamento de desvios e erros de leitura
  if(leitura < Zero){
    return 0.0;
  } else {
    leitura = leitura - Zero;
    return leitura;
  }
  //Serial.println(Irms); // para debug de valores
  return 0.0;
}

void sensordetensao(void){

int leituraSensor = 0; //VARIÁVEL PARA ARMAZENAR A LEITURA DO PINO ANALÓGICO
float coeficiente_de_calibracao = 1.32; // aproximadamente raiz de 1,7
  leituraSensor = analogRead(pinoSensor); //FAZ A LEITURA DO PINO ANALÓGICO E ARMAZENA NA VARIÁVEL O VALOR LIDO
  //Serial.println(leituraSensor);
  //tensaoEntrada = (leituraSensor * 5.0) / 4096.0; //VARIÁVEL RECEBE O RESULTADO DO CÁLCULO 4096 12 bits
  //tensaoMedida = leituraSensor / (valorR2/(valorR1+valorR2)); //VARIÁVEL RECEBE O VALOR DE TENSÃO DC MEDIDA PELO SENSOR

  /*calculo de tensão
  leitura dada por relação das resistencias r1 = 30k e r2 = 7.5k  vout = vin (0-25v) r2/(r1+r2)  vin = vout * (r1+r2)/r2
  real r1= 29.95k r2 = 7480   aproximadamente 5,00401
  (r1+r2)/r2 = 5 aproximadamente 
  vin = vout * 5 

  vout = (leituraSensor * 5.00401)/4095
  */
  tensaoEntrada = (leituraSensor * 4.8)/ 4095.0;
  tensaoMedida = tensaoEntrada * (5.00401/ coeficiente_de_calibracao);
 // Serial.println(tensaoMedida);     
}


// precisa ser revisado quando tudo estiver montado para fazer sentindo as leituras.

void sensor_ACS712(void){
  // Sensor de Corrente ACS712
  float Samples=0.0,                    // variaveis para o ACS712
        voltage,                        // Sensor de Corrente ACS712 (ainda nao calibrado/funcionando)
        amostras = 100;
  int offset_open = 0;                  // variavel de calibracao do 0
  
  for (int x = 0; x < amostras; x++){   //Coleta amostras
    leitura_corrente = analogRead(sensor_correnteACS712);   //Read current sensor values  
    Samples = Samples + leitura_corrente;                   //Add samples together
    //delay (10);  // so precisa para leituras AC           // let ADC settle before next sample 3ms
  }

  AvgAcs=Samples/amostras;                             // medias das leituras
  voltage=(AvgAcs)*(5.0 / 4096.0);                    //((AvgAcs * (5.0 / 4096)) is converitng the read voltage in 0-5 volts  2^12 12bits usado pela esp32
  corrente_medidaACS712 = (2.5 - voltage)*1000/0.185; //2.5 is offset,,,   0.185v is rise in output voltage when 1A current flows at input
  
  // offset_open seria o valor central que substitui o 3.36 no meu caso foi meu 0 aqui em casa

  if (voltage >= 3.36){
    corrente_medidaACS712 = (voltage-3.36)*1000/100; //para cada .100 distante de 2,5 significa uma corrente de 1000mA ou 1A
  }else{
    corrente_medidaACS712 = (3.36-voltage)*1000/100; //para cada .100 distante de 2,5 significa uma corrente de 1000mA ou 1A  
  }
  //Serial.println(corrente_medidaACS712); //p debug
  //Serial.print(leitura_corrente);        //p debug
}

void getDHT(void){
  float tempIni = dht.readTemperature();;
  float humIni = dht.readHumidity();

  if (isnan(tempIni) || isnan(humIni)){   // Check if any reads failed and exit early (to try again). 
    Serial.println("Umidade ou temperatura deram nan");
  } else {
    localTemp = tempIni;
    localHum = humIni;
  }
}

void Thermistor(){
  
  float RawAD = analogRead(pinTermistor);
  // float Res_thermistor = 1000.0;
  float Temp = 0;
  float c1 = 0.001129148;   //coeficientes de steinhart-hart para o termistor
  float c2 = 0.000234125; 
  float c3 = 0.0000000876741; 

  Temp = log(1000.0 * ((4095.0 / (RawAD - 1.0)))); 
  Temp = (1.0 / (c1 + c2*Temp + c3*Temp*Temp*Temp)); // temperature in Kelvin 
  Temp = Temp - 273.15; // convert from Kelvin to Celsius 
  Temp2 = Temp;

}

void connectWiFi(void){

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) 
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();

}

/******************************************************************
*   Get display    (faixa do menu de seleção pelo potenciometro)
*******************************************************************/
void getDisplay(void * pvParameters){

  for(;;){
        
    displayData(1);
    delay(2500);
    displayData(2);
    delay(2500);
    displayData(3);
    delay(2500);  
    displayData(4);
    delay(2500);        
  }    
}

/**************************************************************
*         Display Data    (menus do display)
***************************************************************/
void displayData(int displayNum) {
 
  //Serial.print(" Display Number: ==> ");
  //Serial.print(displayNum);  
  
  display.clearDisplay();   // clear the display
  switch (displayNum){
    case 0:
      display.clearDisplay();
      break;
    case 1:
      display.setCursor(0,20);
      display.println( "Temp. do Totem:");
      display.print(localTemp);
      display.println( "oC");
      display.println("Umidade:");
      display.print(localHum);
      display.println("%");
      break;
    case 2:
      display.setCursor(0,20);
      display.println("Temp. bateria:");
      display.print(Temp2);
      display.println("oC");
      break;
    case 3:
      display.setCursor(0,20);
      display.println("Corrente da baterias:");
      display.print(corrente_medidaACS712);
      display.println("mA");
      display.println("Tensão da baterias");
      display.print(tensaoMedida);      
      display.println("V");
      display.print("Potencia:");
      display.print(tensaoMedida * corrente_medidaACS712/1000.0);
      display.println("W");
      break;
    case 4:
      display.setCursor(0,20);
      display.println("Corrente no Inversor:");
      display.print(Irms);
      display.println("mA");
      break;
    default: 
      display.clearDisplay();
      break;
  }
  display.display();   // write the buffer to the display
  delay(10);
}


/*******************************
*   Parte Executavel do Codigo
*   Atualizada: 03/07
*   Status: Compilando
*   Testes: Incompletos
********************************/
void loop(){

  // Atualiza o display

  
  // displayData(getDisplay());

  // Coleta de informações 
  getDHT();
  //WiFiLocalWebPageCtrl();
  Thermistor();  // Calculo da Temperatura do NTC ou das baterias
  sensordetensao();                                              // Coleta a tensão das baterias
  sensor_ACS712();                                               // Corrente do ACS712 (ainda nao pronto)
  Irms = sensor_ZMC();                                           // Corrente do ZMCT ou rosquinha

  // envio de informações 
  sendTemperaturaTotem(localTemp);                               // Temperatura do ambiente do Totem
  sendHumidadeTotem(localHum);                                   // Umidade do Totem
  sendTemperaturaBateria(Temp2);                                 // Temperatura das Baterias
  sendSensorTensao(tensaoMedida);
  sendSensorCorrenteZMC(Irms);                                   // Corrente saindo do inversor
  //sendoSensorCorrenteACS712();
  
  // Controle
  rele();  
 
}
