// BIBLIOTECAS
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
#include <RTClib.h>
#include "DHT.h"
#include <Adafruit_NeoPixel.h>

// Instâncias
Adafruit_SSD1306 display(128, 64, &Wire, -1); // objeto do DISPLAY
RTC_DS1307 rtc;                               // objeto do sensor de data
DHT dht(15, DHT22);                           // objeto do sensor de temperatura e umidade

// DEFINIÇÕES MQ2 SENSOR DE GAS
#define PIN_GAS_SENSOR 34

// DEFINIÇÕES MATRIZ DE LEDs
#define PIN_LED 12  // Pino de dados
#define NUMPIXELS 3 // Quantidade de LEDs

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);

// FUNÇÕES DE INICIALIZAÇÃO
void iniciarLeds()
{
  pinMode(PIN_LED, OUTPUT);
  pixels.begin(); // Inicializa a matriz
  pixels.setBrightness(100);
}

void iniciarDisplay()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1.5);
}

void iniciarGasSensor()
{
  analogReadResolution(12); // Garante 0-4095
  analogSetAttenuation(ADC_11db);
  pinMode(PIN_GAS_SENSOR, INPUT);
}

// INICIALIZAÇÃO
void setup()
{
  Serial.begin(115200);
  iniciarLeds();
  dht.begin();
  iniciarDisplay();
  rtc.begin();
  iniciarGasSensor();
}

// FUNÇÕES DE EXECUÇÃO
void setDisplay(int x, int y, const char *text, float value)
{
  display.setCursor(x, y);
  if (value == -1.0)
  {
    display.printf(text);
  }
  else
  {
    display.printf(text, value);
  }
}

int processarQualidadeAr(int value)
{
  if (value > 3000) // MUITO POLUIDO
  {
    return 1;
  }
  else if (value >= 2000) // POUCO POLUIDO
  {
    return 2;
  }
  else // NORMAL
  {
    return 3;
  }
}

void exibirQualidadeAr(int opt)
{
  if (opt == 1)
  {
    display.setCursor(15, 35);
    display.println("- Muito poluido -");
  }
  else if (opt == 2)
  {
    display.setCursor(15, 35);
    display.println("- Pouco poluido -");
  }
  else if (opt == 3)
  {
    display.setCursor(35, 35);
    display.println("- Normal -");
  }
}

void mudarCorMatriz(int opt)
{
  pixels.clear(); // Limpar matriz de LEDs
  if (opt == 1)
  {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Vermelho
    pixels.setPixelColor(1, pixels.Color(255, 0, 0)); // Vermelho
    pixels.setPixelColor(2, pixels.Color(255, 0, 0)); // Vermelho
  }
  else if (opt == 2)
  {
    pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Amarelo
    pixels.setPixelColor(1, pixels.Color(255, 255, 0)); // Amarelo
    pixels.setPixelColor(2, pixels.Color(255, 255, 0)); // Amarelo
  }
  else if (opt == 3)
  {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Verde
    pixels.setPixelColor(1, pixels.Color(0, 255, 0)); // Verde
    pixels.setPixelColor(2, pixels.Color(0, 255, 0)); // Verde
  }
  pixels.show();
}

boolean verificarErro(float value)
{
  if (isnan(value)) // Se o dado estiver errado
  {
    return true;
  }
  return false;
}

boolean verificarTempo(long tempoAtual, long temAnt, long inter)
{
  if (tempoAtual - temAnt >= inter)
  {
    return true;
  }
  return false;
}

unsigned long tempoAnteriorTemp = 0;
unsigned long tempoAnteriorRel = 0;
unsigned long tempoAnteriorAr = 0;
unsigned long tempoAnteriorDisplay = 0;
int telaAtual = 0;
const long intervaloTemp = 600000;
const long intervaloRel = 60000;
const long intervaloAr = 300000;
const long intervaloDisplay = 4000;
float tempDHT;
float umidDHT;
DateTime agora;
int qualidadeAr;
int tentativasDHT = 0;
int tentativasAr = 0;
const int maxTentativas = 5;
bool iniciar = true;
void loop() // PROCESSAMENTO (INPUTs E OUTPUTs)
{
  unsigned long tempoAtual = millis();
  if (iniciar)
  {
    // PRIMEIRO PROECESSAMENTO TEMPERATURA
    tempDHT = dht.readTemperature(); // Lê e armazena dado obtido pelo sensor DHT22
    umidDHT = dht.readHumidity();    // Lê e armazena dado obtido pelo sensor DHT22

    // PRIMEIRO PROCESSAMENTO DATA
    agora = rtc.now();

    // PRIMEIRO PROCESSAMENTO SENSOR GÁS
    int qualidadeArLeitura = analogRead(PIN_GAS_SENSOR);
    qualidadeAr = processarQualidadeAr(qualidadeArLeitura);
    mudarCorMatriz(qualidadeAr);
    tentativasAr = 0;

    // Troca o init
    iniciar = false;
  }
  else
  {
    int intervaloAjustadoDHT = (tentativasDHT > 0) ? 2000 : intervaloTemp; // Ajusta o intervalo pra caso haja erro na leitura
    // PROCESSAMENTO TEMPERATURA
    if (verificarTempo(tempoAtual, tempoAnteriorTemp, intervaloAjustadoDHT))
    {
      float tempLeitura = dht.readTemperature(); // Lê e armazena dado obtido pelo sensor DHT22
      float umidLeitura = dht.readHumidity();    // Lê e armazena dado obtido pelo sensor DHT22
      if (!verificarErro(tempLeitura) && !verificarErro(umidLeitura))
      {
        tempDHT = tempLeitura;
        umidDHT = umidLeitura;
        tentativasDHT = 0; // Reseta o contador de erros
      }
      else
      {
        tentativasDHT++;

        if (tentativasDHT > maxTentativas)
        {
          tentativasDHT = 0;                 // Reseta contador de erros
          setDisplay(0, 55, "!DHT22", -1.0); // Exibe mensagem de erro no Display OLED
        }
      }
      tempoAnteriorTemp = tempoAtual; // Reseta o Timer
    }

    // PROCESSAMENTO RELÓGIO
    if (verificarTempo(tempoAtual, tempoAnteriorRel, intervaloRel))
    {
      agora = rtc.now(); // Variavel do tipo DateTime recebe o horario de agora
      tempoAnteriorRel = tempoAtual;
    }

    int intervaloAjustadoAr = (tentativasAr > 0) ? 2000 : intervaloAr; // Ajusta o intervalo pra caso haja erro na leitura

    // PROCESSAMENTO SENSOR DE GAS
    if (verificarTempo(tempoAtual, tempoAnteriorAr, intervaloAjustadoAr))
    {
      int qualidadeArLeitura = analogRead(PIN_GAS_SENSOR); // Lê o valor analógico (0-4090) do sensor de gás

      if (!verificarErro(qualidadeArLeitura))
      {
        qualidadeAr = processarQualidadeAr(qualidadeArLeitura);
        mudarCorMatriz(qualidadeAr);
        tentativasAr = 0;
      }
      else
      {
        tentativasAr++;

        if (tentativasAr > maxTentativas)
        {
          tentativasAr = 0;
          setDisplay(0, 55, "!MQ2", -1.0); // Exibe mensagem de erro no Display OLED
          display.display();
        }
      }
      tempoAnteriorAr = tempoAtual;
    }
  }
  // CARROSSEL DISPLAY OLED
  if (verificarTempo(tempoAtual, tempoAnteriorDisplay, intervaloDisplay))
  {
    tempoAnteriorDisplay = tempoAtual;
    telaAtual++;
    if (telaAtual > 3)
      telaAtual = 1;

    display.clearDisplay(); // Limpar Tela

    // SELETOR DE TELAS
    switch (telaAtual)
    {
    case 1:
      setDisplay(10, 27, "Temperatura:%0.1fC", tempDHT); // Exibe temperatura no display OLED
      break;
    case 2:
      setDisplay(30, 27, "Umidade:%0.0f%%", umidDHT); // Exibe umidade no display OLED
      break;
    case 3:
      setDisplay(18, 27, "Qualidade do ar", -1.0);
      exibirQualidadeAr(qualidadeAr); // Exibe Resultado do Processamento da qualidade do AR no Display OLED
      break;
    }

    // OBRIGATÓRIO EM QUALQUER INFORMAÇÃO QUE FOR EXIBIDA NO DISPLAY DE OLED
    char msg[9];
    snprintf(msg, sizeof(msg), "%02d:%02d", agora.hour(), agora.minute()); // FORMATA A DATA
    setDisplay(90, 55, msg, -1.0);                                         // Exibe data na tela

    // EXIBE AS INFORMAÇÕES NO DISPLAY DE OLED
    display.display();
  }
}