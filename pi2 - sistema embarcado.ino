// SENSOR KY-013 SENSOR DE TEMPERATURA DA BATERIA
//#include<Thermistor.h>                      //Inclusão da biblioteca Thermistor
//Thermistor temp(18);                        //Atribui o pino analógico 18, em que o termistor está conectado
//int calor = 0;

 
// WIFI
#include <WiFi.h>
const char* ssid     = "2G_GABRIEL";
const char* password = "84122163";
WiFiServer server(80);


// DHT 11
#include "DHT.h"
#define DHTPIN 4  
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
float localHum = 0;
float localTemp = 0;

// Termistor
#define pinTermistor 13
float readTemperatureNTC(float resistenciaTermistor, float resistenciaResistorSerie, float voltageUc, float Beta);
float getResistencia(int pin, float voltageUc, float adcResolutionUc, float resistenciaEmSerie);
float calcularCoeficienteBetaTermistor();
float temperatura;


// RELE
#define led_pin 5


// Sensor de Corrente ACS712
#define sensor_correnteACS712 21
float voltage;
float leitura_corrente=0.0, Samples=0.0, AvgAcs=0.0, corrente_medidaACS712=0.0;


// Sensor de Corrente ZMC
#define correnteZMC 26
float leitura_zmc = 0.0;


// Sensor de Tensão
#define sensor_tensao 25 //PINO ANALÓGICO EM QUE O SENSOR ESTÁ CONECTADO
float tensaoEntrada = 0.0; //VARIÁVEL PARA ARMAZENAR O VALOR DE TENSÃO DE ENTRADA DO SENSOR
float tensao_medida = 0.0; //VARIÁVEL PARA ARMAZENAR O VALOR DA TENSÃO MEDIDA PELO SENSOR
float valorR1 = 30000.0; //VALOR DO RESISTOR 1 DO DIVISOR DE TENSÃO
float valorR2 = 7500.0; // VALOR DO RESISTOR 2 DO DIVISOR DE TENSÃO
int leitura_tensao = 0; //VARIÁVEL PARA ARMAZENAR A LEITURA DO PINO ANALÓGICO



void setup()
{
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);  // rele
  pinMode(correnteZMC, INPUT);
  pinMode(sensor_tensao, INPUT);
  pinMode(sensor_correnteACS712, INPUT);
  pinMode(pinTermistor, INPUT);

  delay(1000);

  connectWiFi();
  dht.begin();  
}


void loop()
{
  
  getDHT();
  WiFiLocalWebPageCtrl();
  thermistor();
  sensordetensao();
  sensor_ACS712();
  sensor_ZMC();

 
}

void sensor_ZMC(void){

  // Sensor de Corrente ZMC
  leitura_zmc = analogRead(correnteZMC);

}

void sensordetensao(void)
{
// Sensor de Tensão
  leitura_tensao = analogRead(sensor_tensao); //FAZ A LEITURA DO PINO ANALÓGICO E ARMAZENA NA VARIÁVEL O VALOR LIDO
   tensaoEntrada = (leitura_tensao * 5.0) / 1024.0; //VARIÁVEL RECEBE O RESULTADO DO CÁLCULO
   tensao_medida = tensaoEntrada / (valorR2/(valorR1+valorR2)); //VARIÁVEL RECEBE O VALOR DE TENSÃO DC MEDIDA PELO SENSOR  
  //Serial.print(tensao_medida);
  
}

void sensor_ACS712(void)
{
  // Sensor de Corrente ACS712
  unsigned int x=0;

  for (int x = 0; x < 10; x++)          //Get 10 samples
  {
    leitura_corrente = analogRead(21);           //Read current sensor values   
    Samples = Samples + leitura_corrente;        //Add samples together
    delay (10);                           // let ADC settle before next sample 3ms
  }
  AvgAcs=Samples/10.0;                   //Taking Average of Samples
  voltage=AvgAcs*(5.0 / 1024.0);         //((AvgAcs * (5.0 / 1024.0)) is converitng the read voltage in 0-5 volts
  corrente_medidaACS712 = (2.5 - voltage)*1000/0.185; //2.5 is offset,,,   0.185v is rise in output voltage when 1A current flows at input
  //Serial.print(corrente_medida);

  Serial.print(leitura_corrente);

}

void getDHT(void)
{
  float tempIni = localTemp;
  float humIni = localHum;
  localTemp = dht.readTemperature();
  localHum = dht.readHumidity();
  if (isnan(localHum) || isnan(localTemp))   // Check if any reads failed and exit early (to try again).
  {
    localTemp = tempIni;
    localHum = humIni;
  }
    delay(1000);
}



void thermistor(void){
  float resistencia = getResistencia(13, 3.3, 4095.0, 10000.0);
  float beta = calcularCoeficienteBetaTermistor();
  float temperatura_bateria = readTemperatureNTC(resistencia, 10000.0, 3.3, beta);
  temperatura = temperatura_bateria;
  //Serial.print("Temperatura:");
 // Serial.print(temperatura_bateria);

}

float readTemperatureNTC(float resistenciaTermistor,
                         float resistenciaResistorSerie,
                         float voltageUc,
                         float Beta) {
  // Constantes locais
  const double To = 298.15;    // Temperatura em Kelvin para 25 graus Celsius
  const double Ro = 10000.0;   // Resistência do termistor a 25 graus Celsius

  // Variáveis locais
  double Vout = 0; // Tensão lida da porta analógica do termistor.
  double Rt = 0; // Resistência lida da porta analógica.
  double T = 0; // Temperatura em Kelvin.
  double Tc = 0; // Temperatura em Graus Celsius.

  Vout = resistenciaResistorSerie /
         (resistenciaResistorSerie + resistenciaTermistor) *
         voltageUc; // Cálculo da tensão lida da porta analógica do termistor.

  Rt = resistenciaResistorSerie * Vout /
       (voltageUc - Vout); // Cálculo da resistência da porta analógica.
  T = 1 /
      (
        1 / To + log(Rt / Ro) / Beta
      ); // Aplicação da equação de parâmetro Beta para obtenção da Temperatura em Kelvin.

  Tc = T - 273.15; // Conversão de Kelvin para Celsius.
  return Tc;
}

/**
  Calcule o valor Beta de um termistor, com os valores de resistência (RT1 e RT2)
    obtidos do datasheet nas temperaturas correspondentes (T1 e T2).

  @return O valor Beta.
*/
float calcularCoeficienteBetaTermistor() {
  float beta;
  const float T1 = 273.15;  // valor de temperatura 0º C convertido em Kelvin.
  const float T2 = 373.15;  // valor de temperatura 100º C convertido em Kelvin.
  const float RT1 = 27.219; // valor da resistência (em ohm) na temperatura T1.
  const float RT2 = 0.974;  // valor da resistência (em ohm) na temperatura T2.
  beta = (log(RT1 / RT2)) / ((1 / T1) - (1 / T2));  // cálculo de beta.
  return beta;
}


float getResistencia(int pin, float voltageUc, float adcResolutionUc, float resistenciaEmSerie) {
  float resistenciaDesconhecida = 0;

  resistenciaDesconhecida =
    resistenciaEmSerie *
    (voltageUc /
     (
       (analogRead(pin) * voltageUc) /
       adcResolutionUc
     ) - 1
    );

  return resistenciaDesconhecida;
}


void WiFiLocalWebPageCtrl(void)
{
  
  WiFiClient client = server.available();   // listen for incoming clients
  //client = server.available();
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            //WiFiLocalWebPageCtrl(); 
              client.print("Temperatura ambiente: ");
              client.print(localTemp);
              client.print("  oC<br>");
              client.print("Umidade ambiente: ");
              client.print(localHum);
              client.print(" % <br>");
              client.print("Temperatura da bateria: ");
              client.print(temperatura);
              client.print (" oC <br>");
              client.print("Sensor de Corrente ACS712: ");
              client.print(corrente_medidaACS712);
              client.print(" mA <br>");  
                          
              client.print("Sensor de Tensao: ");
              client.print(tensao_medida);
              client.print(" V <br>");

              client.print("Sensor de Corrente ZMC: ");
              client.print(leitura_zmc);
              client.print(" mA <br>"); 


              client.print("Clique <a href=\"/H\">aqui</a> para iniciar o recarregamento.<br>");
              client.print("Clique <a href=\"/L\">aqui</a> para interromper o recarregamento.<br>");         

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // CONTROLE DO RELE 
        if (currentLine.endsWith("GET /H")) {
          //if (estado_rele == LOW) {
            digitalWrite(led_pin, HIGH);
            //} else {
            //digitalWrite(rele, HIGH);
           // estado_rele = HIGH;
          //          }  
        }
        
        if (currentLine.endsWith("GET /L")) {
          //if (estado_rele == HIGH) {
            digitalWrite(led_pin, LOW);
           // } else {
           // digitalWrite(rele, LOW);
           // estado_rele = LOW;
           //         }
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}


void connectWiFi(void)
{
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

