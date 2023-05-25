// SENSOR KY-013 SENSOR DE TEMPERATURA DA BATERIA
#include<Thermistor.h>                      //Inclusão da biblioteca Thermistor
Thermistor temp(18);                        //Atribui o pino analógico 18, em que o termistor está conectado
int calor = 0;

 
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


// RELE
#define rele 5
bool estado_rele = LOW;


// Sensor de Corrente
/*
#define sensor_corrente = 26
float voltage;
*/


// Sensor de Tensão
/*
#define sensor_tensao = 25; //PINO ANALÓGICO EM QUE O SENSOR ESTÁ CONECTADO

float tensaoEntrada = 0.0; //VARIÁVEL PARA ARMAZENAR O VALOR DE TENSÃO DE ENTRADA DO SENSOR
float tensao_medida = 0.0; //VARIÁVEL PARA ARMAZENAR O VALOR DA TENSÃO MEDIDA PELO SENSOR

float valorR1 = 30000.0; //VALOR DO RESISTOR 1 DO DIVISOR DE TENSÃO
float valorR2 = 7500.0; // VALOR DO RESISTOR 2 DO DIVISOR DE TENSÃO
int leitura_tensao = 0; //VARIÁVEL PARA ARMAZENAR A LEITURA DO PINO ANALÓGICO
*/


void setup()
{
  Serial.begin(115200);
  pinMode(5, OUTPUT); 
  delay(10);


//pinMode(sensor_tensao, INPUT);
//pinMode(sensor_corrente, INPUT);
//pinMode(rele, OUTPUT);
  

  connectWiFi();
 

  dht.begin();  
}


void loop()
{
  
  getDHT();
  WiFiLocalWebPageCtrl();
  temperatura();


/* AINDA VOU ARRUMAR ESSAS COISAS
// Sensor de Tensão
leitura_tensao = analogRead(sensor_tensao); //FAZ A LEITURA DO PINO ANALÓGICO E ARMAZENA NA VARIÁVEL O VALOR LIDO
   tensaoEntrada = (leitura_tensao * 5.0) / 1024.0; //VARIÁVEL RECEBE O RESULTADO DO CÁLCULO
   tensao_medida = tensaoEntrada / (valorR2/(valorR1+valorR2)); //VARIÁVEL RECEBE O VALOR DE TENSÃO DC MEDIDA PELO SENSOR  


// Sensor de Corrente
  unsigned int x=0;
  float leitura_corrente=0.0, Samples=0.0, AvgAcs=0.0, corrente_medida=0.0;
  for (int x = 0; x < 10; x++)          //Get 10 samples
  {
    leitura_corrente = analogRead(sensor_corrente);           //Read current sensor values   
    Samples = Samples + leitura_corrente;        //Add samples together
    delay (10);                           // let ADC settle before next sample 3ms
  }
  AvgAcs=Samples/10.0;                   //Taking Average of Samples
  voltage=AvgAcs*(5.0 / 1024.0);         //((AvgAcs * (5.0 / 1024.0)) is converitng the read voltage in 0-5 volts
  corrente_medida = (2.5 - voltage)*1000/0.185; //2.5 is offset,,,   0.185v is rise in output voltage when 1A current flows at input
  */
}


void getDHT()
{
  float tempIni = localTemp;
  float humIni = localHum;
  localTemp = dht.readTemperature();
  localHum = dht.readHumidity();
  if (isnan(localHum) || isnan(localTemp))   // Check if any reads failed and exit early (to try again).
  {
    localTemp = tempIni;
    localHum = humIni;
    return;
  }
}


void temperatura(){
  int calor = temp.getTemp();//Variável do tipo inteiro que recebe o valor da temperatura calculado pela biblioteca
  return;//Intervalo de 1 segundo
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
              client.print("Temperature now is: ");
              client.print(localTemp);
              client.print("  oC<br>");
              client.print("Humidity now is:     ");
              client.print(localHum);
              client.print(" % <br>");
              client.print("Sensor de temperatura KY-013: ");
              client.print(calor);
              client.print (" C <br>");
              //client.print("Sensor de Corrente: ");
              //client.print(corrente_medida);
              //client.print(" mA <br>");  
                          
              //client.print("Sensor de Tensão: ");
              //client.print(tensao_medida);
              //client.print(" V <br>");


              client.print("Click <a href=\"/H\">here</a> to turn the LED on.<br>");
              client.print("Click <a href=\"/L\">here</a> to turn the LED off.<br>");         

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
          
          if (estado_rele == LOW) {
            digitalWrite(rele, HIGH);
            estado_rele = HIGH;
            } else {
            digitalWrite(rele, HIGH);
            estado_rele = HIGH;
                    }  
        }
        
        if (currentLine.endsWith("GET /L")) {
          if (estado_rele == HIGH) {
            digitalWrite(rele, LOW);
            estado_rele = LOW;
            } else {
            digitalWrite(rele, LOW);
            estado_rele = LOW;
                    }
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
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}