#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include "secrets.h"
#include <BlynkSimpleEsp32.h>

BlynkTimer timer;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 26
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define BTN_TELA 33
#define BTN_RESET 25

#define LED_VERDE 5
#define LED_VERMELHO 18

#define LED_BI_VERDE 12
#define LED_BI_VERMELHO 19

#define SW1 16
#define SW2 4
#define SW3 2
#define SW4 15

int telaAtual = 0;

float temperatura = 0;
float umidade = 0;

float tempMin = 999;
float tempMax = -999;

float umidMin = 999;
float umidMax = -999;

bool estadoBotaoTelaAnterior = false;
bool estadoBotaoResetAnterior = false;

bool estadoLedVerde = false;
bool estadoLedVermelho = false;

bool estadoAnteriorSW4 = false;
bool estadoAnteriorControle = false;

bool estadoAnteriorSW2 = false;
bool estadoAnteriorSW3 = false;

bool remotoHabilitado = false;

unsigned long ultimaLeitura = 0;
unsigned long ultimaTrocaTela = 0;
unsigned long pausaAte = 0;

float historicoTemp[60];
float historicoUmid[60];

byte indiceHistorico = 0;

unsigned long ultimoHistorico = 0;

void mostraTela();

void atualizaHistorico() {

  if (temperatura < tempMin)
    tempMin = temperatura;

  if (temperatura > tempMax)
    tempMax = temperatura;

  if (umidade < umidMin)
    umidMin = umidade;

  if (umidade > umidMax)
    umidMax = umidade;
}

void atualizarHistorico60Min()
{
    if(millis() - ultimoHistorico >= 60000)
    {
        ultimoHistorico = millis();

        historicoTemp[indiceHistorico] = temperatura;
        historicoUmid[indiceHistorico] = umidade;

        indiceHistorico++;

        if(indiceHistorico >= 60)
            indiceHistorico = 0;
    }
}

void verificarConexao(){
    if (WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, pass);
    }
    if (!Blynk.connected()){
        Blynk.connect();
        if (Blynk.connected()){
            Blynk.syncAll();
        }
    }
}

void mostraTela() {

  lcd.clear();

  switch (telaAtual) {

    case 0:

      lcd.setCursor(0, 0);
      lcd.print("Temp:");
      lcd.print(temperatura, 1);
      lcd.print(" C");

      lcd.setCursor(0, 1);
      lcd.print("Umid:");
      lcd.print(umidade, 0);
      lcd.print("%");

      break;

    case 1:

      lcd.setCursor(0, 0);
      lcd.print("Temp:");
      lcd.print((temperatura * 1.8) + 32, 1);
      lcd.print(" F");

      lcd.setCursor(0, 1);
      lcd.print("Umid:");
      lcd.print(umidade, 0);
      lcd.print("%");

      break;

    case 2:

      lcd.setCursor(0, 0);
      lcd.print("TMin:");
      lcd.print(tempMin, 1);

      lcd.setCursor(0, 1);
      lcd.print("TMax:");
      lcd.print(tempMax, 1);

      break;

    case 3:

      lcd.setCursor(0, 0);
      lcd.print("UMin:");
      lcd.print(umidMin, 0);

      lcd.setCursor(0, 1);
      lcd.print("UMax:");
      lcd.print(umidMax, 0);

      break;

    case 4:

      lcd.setCursor(0,0);

      if(WiFi.status()==WL_CONNECTED){

          if(Blynk.connected())
              lcd.print("WiFi/Blynk OK");
          else
              lcd.print("WiFi OK");

      }else{

          lcd.print("Sem WiFi");
      }

      lcd.setCursor(0,1);

      lcd.print("RSSI:");

      if(WiFi.status()==WL_CONNECTED)
          lcd.print(WiFi.RSSI());
      else
          lcd.print("---");

      break;
  }
}

void setup() {

  timer.setInterval(10000L, verificarConexao);

  Wire.begin(22, 23);

  lcd.init();
  lcd.backlight();

  dht.begin();

  pinMode(BTN_TELA, INPUT);
  pinMode(BTN_RESET, INPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  pinMode(LED_BI_VERDE, OUTPUT);
  pinMode(LED_BI_VERMELHO, OUTPUT);

  digitalWrite(LED_BI_VERDE, LOW);
  digitalWrite(LED_BI_VERMELHO, LOW);

  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);

  estadoAnteriorSW2 = digitalRead(SW2);
  estadoAnteriorSW3 = digitalRead(SW3);

  Serial.begin(115200);

  WiFi.begin(ssid, pass);

  lcd.clear();
  lcd.print("Conectando");

  while (WiFi.status() != WL_CONNECTED) {

      delay(500);

      Serial.print(".");
  }

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  estadoAnteriorSW4 = digitalRead(SW4);

  if (estadoAnteriorSW4){
    Blynk.virtualWrite(V9, "Fahrenheit");
  }else{
    Blynk.virtualWrite(V9, "Celsius");
  }

  estadoLedVerde = false;
  estadoLedVermelho = false;

  digitalWrite(LED_VERDE, estadoLedVerde);
  digitalWrite(LED_VERMELHO, estadoLedVermelho);

  Blynk.virtualWrite(V10, digitalRead(LED_VERDE) ? 255 : 0);
  Blynk.virtualWrite(V11, digitalRead(LED_VERMELHO) ? 255 : 0);
    
  estadoAnteriorSW2 = digitalRead(SW2);
  estadoAnteriorSW3 = digitalRead(SW3);

  mostraTela();
}

void loop() {
  remotoHabilitado = digitalRead(SW1);
  Blynk.run();
  timer.run();
  if (Blynk.connected())
  {
      Blynk.virtualWrite(V10, digitalRead(LED_VERDE) ? 255 : 0);
      Blynk.virtualWrite(V11, digitalRead(LED_VERMELHO) ? 255 : 0);
  }
  if (millis() - ultimaLeitura >= 2000) {

    ultimaLeitura = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {

      temperatura = t;
      umidade = h;

      Blynk.virtualWrite(V0, temperatura);
      Blynk.virtualWrite(V1, temperatura * 1.8 + 32);
      Blynk.virtualWrite(V2, umidade);
      Blynk.virtualWrite(V3, WiFi.RSSI());
      bool estadoSW4 = digitalRead(SW4);
      if (estadoSW4 != estadoAnteriorSW4){
        estadoAnteriorSW4 = estadoSW4;
        if (estadoSW4) {
          Blynk.virtualWrite(V9, "Fahrenheit");
        }else{
          Blynk.virtualWrite(V9, "Celsius");
        }
      }
      atualizaHistorico();
      atualizarHistorico60Min();
      mostraTela();
    }
  }

  bool leituraTela = digitalRead(BTN_TELA);

  if (leituraTela && !estadoBotaoTelaAnterior) {

    telaAtual++;

    if (telaAtual > 4)
      telaAtual = 0;

    // pausa o modo automático por 10 segundos
    pausaAte = millis() + 10000;

    mostraTela();
  }

  estadoBotaoTelaAnterior = leituraTela;

  bool leituraReset = digitalRead(BTN_RESET);

  if (leituraReset && !estadoBotaoResetAnterior) {

    tempMin = temperatura;
    tempMax = temperatura;

    umidMin = umidade;
    umidMax = umidade;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Historico");

    lcd.setCursor(0, 1);
    lcd.print("Resetado!");

    delay(1000);

    mostraTela();
  }

  estadoBotaoResetAnterior = leituraReset;

  // Atualiza o indicador no Blynk somente quando mudar
  if (remotoHabilitado != estadoAnteriorControle) {
      estadoAnteriorControle = remotoHabilitado;
      Blynk.virtualWrite(V8, remotoHabilitado);
  }

  bool leituraSW2 = digitalRead(SW2);
  bool leituraSW3 = digitalRead(SW3);

  // Se o switch verde mudou, ele passa a ser o último comando
  if (leituraSW2 != estadoAnteriorSW2)
  {
      estadoAnteriorSW2 = leituraSW2;

      estadoLedVerde = leituraSW2;

      digitalWrite(LED_VERDE, estadoLedVerde);

      if (Blynk.connected())
          Blynk.virtualWrite(V10, digitalRead(LED_VERDE) ? 255 : 0);
  }

  // Se o switch vermelho mudou, ele passa a ser o último comando
  if (leituraSW3 != estadoAnteriorSW3)
  {
      estadoAnteriorSW3 = leituraSW3;

      estadoLedVermelho = leituraSW3;

      digitalWrite(LED_VERMELHO, estadoLedVermelho);

      if (Blynk.connected())
        Blynk.virtualWrite(V11, digitalRead(LED_VERMELHO) ? 255 : 0);
  }

  if (millis() > pausaAte) {

    if (millis() - ultimaTrocaTela >= 3000) {

      ultimaTrocaTela = millis();

      telaAtual++;

      if (telaAtual > 4)
        telaAtual = 0;

      mostraTela();
    }
  }
}

BLYNK_CONNECTED(){
    Blynk.syncAll();

    Blynk.virtualWrite(V10, digitalRead(LED_VERDE) ? 255 : 0);
    Blynk.virtualWrite(V11, digitalRead(LED_VERMELHO) ? 255 : 0);

    if (digitalRead(SW4)){
        Blynk.virtualWrite(V9, "Fahrenheit");
    }else{
        Blynk.virtualWrite(V9, "Celsius");
    }
}

BLYNK_WRITE(V4)
{
    if (remotoHabilitado)
    {
        estadoLedVerde = param.asInt();

        digitalWrite(LED_VERDE, estadoLedVerde);
    }
}

BLYNK_WRITE(V5)
{
    if (remotoHabilitado)
    {
        estadoLedVermelho = param.asInt();

        digitalWrite(LED_VERMELHO, estadoLedVermelho);
    }
}

BLYNK_WRITE(V6)
{
    if (!remotoHabilitado)
        return;

    int cor = param.asInt();

    switch(cor)
    {
        case 0:
            digitalWrite(LED_BI_VERDE, LOW);
            digitalWrite(LED_BI_VERMELHO, LOW);
            break;

        case 1:
            digitalWrite(LED_BI_VERDE, HIGH);
            digitalWrite(LED_BI_VERMELHO, LOW);
            break;

        case 2:
            digitalWrite(LED_BI_VERDE, LOW);
            digitalWrite(LED_BI_VERMELHO, HIGH);
            break;

        case 3:
            digitalWrite(LED_BI_VERDE, HIGH);
            digitalWrite(LED_BI_VERMELHO, HIGH);
            break;
    }
}

BLYNK_WRITE(V7){
  if(!remotoHabilitado){
    return;
  }
  if(param.asInt()){
    tempMin = temperatura;
    tempMax = temperatura;

    umidMin = umidade;
    umidMax = umidade;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Reset remoto");

    lcd.setCursor(0,1);
    lcd.print("Concluido");

    delay(1000);

    mostraTela();

    Blynk.virtualWrite(V7,0);
  }
}