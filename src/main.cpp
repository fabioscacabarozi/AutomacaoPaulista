#include <Arduino.h>

// Leitura das Portas
#include <PortExpander_I2C.h>

// WIFI
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#include "html_principal.h"
#include "html_saida.h"
#include "html_entrada.h"

// Objetos para comunicação com as portas
PortExpander_I2C relecontrol(0x27);
PortExpander_I2C entradas(0x20);

// GPIO ENTRADA
#define PIN_FIM_CURSO_FECHADO 3 // Entrada 1 - Fim de curso Fechado Portao Saida
#define PIN_FIM_CURSO_ABERTO 1  // Entrada 2 - Fim de curso Aberto Portao Saida
#define PIN_FOTOCELULA 0        // Entrada 3 - Fotocelula Portao Saida
#define PIN_LACO_MAGNETICO 2    // Entrada 4 - Laço Portao Saida

#define PIN_FIM_CURSO_FECHADO_ENTRADA 4 // Entrada 5 - Fim de curso Fechado Portao Entrada
#define PIN_FIM_CURSO_ABERTO_ENTRADA 5  // Entrada 6 - Fim de curso Aberto Portão Entrada
#define PIN_FOTOCELULA_ENTRADA 6        // Entrada 7 - Fotocelula Portao Entrada

// GPIO SAIDA
#define PIN_RELE_SEMAFORO 0 // Rele 1 - Semaforo
#define PIN_RELE_UHF 1      // Rele 2 - Trigger leitor TAG

#define PIN_RELE_SEMAFORO_ENTRADA 5 // Rele 3 - Semaforo Portão Entrada
#define PIN_RELE_UHF_ENTRADA 4 // Rele 4 - Trigger leitor TAG Entrada

// WIFI
const char *wifi_ssid = "automacao_paulista";
const char *wifi_password = "1234567890";

// Variáveis global
bool ACIONOU_PHOTOCELULA = false;
unsigned long phototimer_start = 0;
const unsigned long PHOTOCELULA_TIMEOUT = 5000; // 5 segundos

bool ACIONOU_PHOTOCELULA_ENTRADA = false;
unsigned long phototimer_start_entrada = 0;
const unsigned long PHOTOCELULA_TIMEOUT_ENTRADA = 5000; // 5 segundos

WiFiServer server(80);

/**
 * 
 * Funções Auxiliares
 * 
 */
void inibirUHF()
{
  relecontrol.digitalWrite(PIN_RELE_UHF, 1);
}

void liberarUHF()
{
  relecontrol.digitalWrite(PIN_RELE_UHF, 0);
}

void setSemaforoVermelho()
{
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO, 0);
}

void setSemaforoVerde()
{
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO, 1);
}

void setSemaforoVermelhoEntrada()
{
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO_ENTRADA, 0);
}

void setSemaforoVerdeEntrada()
{
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO_ENTRADA, 1);
}

void inibirUHFEntrada()
{
  relecontrol.digitalWrite(PIN_RELE_UHF_ENTRADA, 1);
}

void liberarUHFEntrada()
{
  relecontrol.digitalWrite(PIN_RELE_UHF_ENTRADA, 0);
}


/**
 * 
 * Funcoes para a pagina WEB
 * 
 */
struct Status {
  bool fotocelula, laco, fim_aberto, fim_fechado;
  bool rele_uhf, rele_semaforo;
};

struct StatusEntrada {
  bool fotocelula, fim_aberto, fim_fechado;
  bool rele_uhf, rele_semaforo;
};

Status lastStatus = {0,0,0,0,0,0};
StatusEntrada lastStatusEntrada = {0,0,0,0,0};

Status readStatus() {
  Status s;
  s.fotocelula = (entradas.digitalRead(PIN_FOTOCELULA) == 0);
  s.laco = (entradas.digitalRead(PIN_LACO_MAGNETICO) == 0);
  s.fim_aberto = (entradas.digitalRead(PIN_FIM_CURSO_ABERTO) == 0);
  s.fim_fechado = (entradas.digitalRead(PIN_FIM_CURSO_FECHADO) == 0);
  s.rele_uhf = (relecontrol.digitalRead(PIN_RELE_UHF) == 0); // era 1 mas alterado para ajustar rele sempre aberto
  s.rele_semaforo = (relecontrol.digitalRead(PIN_RELE_SEMAFORO) == 1);
  return s;
}

StatusEntrada readStatusEntrada() {
  StatusEntrada s;
  s.fotocelula = (entradas.digitalRead(PIN_FOTOCELULA_ENTRADA) == 0);
  //s.laco = (entradas.digitalRead(PIN_LACO_MAGNETICO) == 0);
  s.fim_aberto = (entradas.digitalRead(PIN_FIM_CURSO_ABERTO_ENTRADA) == 0);
  s.fim_fechado = (entradas.digitalRead(PIN_FIM_CURSO_FECHADO_ENTRADA) == 0);
  s.rele_uhf = (relecontrol.digitalRead(PIN_RELE_UHF_ENTRADA) == 0); // era 1 ajustado para o rele sempre aberto
  s.rele_semaforo = (relecontrol.digitalRead(PIN_RELE_SEMAFORO_ENTRADA) == 1);
  return s;
}

bool statusChanged(Status &a, Status &b) {
  return memcmp(&a, &b, sizeof(Status)) != 0;
}

bool statusChangedEntrada(StatusEntrada &a, StatusEntrada &b) {
  return memcmp(&a, &b, sizeof(StatusEntrada)) != 0;
}

String statusToJson(Status &s) {
  String json = "{";
  json += "\"fotocelula\":" + String(s.fotocelula) + ",";
  json += "\"laco\":" + String(s.laco) + ",";
  json += "\"fim_aberto\":" + String(s.fim_aberto) + ",";
  json += "\"fim_fechado\":" + String(s.fim_fechado) + ",";
  json += "\"rele_uhf\":" + String(s.rele_uhf) + ",";
  json += "\"rele_semaforo\":" + String(s.rele_semaforo) + ",";
  json += "\"datetime\":\"" + String(__DATE__) + " " + String(__TIME__) + "\"";
  json += "}";
  return json;
}

String statusEntradaToJson(StatusEntrada &s) {
  String json = "{";
  json += "\"fotocelula\":" + String(s.fotocelula) + ",";
  //json += "\"laco\":" + String(s.laco) + ",";
  json += "\"fim_aberto\":" + String(s.fim_aberto) + ",";
  json += "\"fim_fechado\":" + String(s.fim_fechado) + ",";
  json += "\"rele_uhf\":" + String(s.rele_uhf) + ",";
  json += "\"rele_semaforo\":" + String(s.rele_semaforo) + ",";
  json += "\"datetime\":\"" + String(__DATE__) + " " + String(__TIME__) + "\"";
  json += "}";
  return json;
}

void webPage(){
  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n' && request.endsWith("\r\n\r\n")) break;
      }
    }
    if (request.indexOf("GET / ") >= 0 || request.indexOf("GET /HTTP") >= 0) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.print(html_home); // string da página inicial
    } else if (request.indexOf("GET /saida") >= 0) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.print(html_saida); // string da página do portão de saída
    } else if (request.indexOf("GET /entrada") >= 0) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.print(html_entrada); // string da página do portão de entrada
    } else if (request.indexOf("GET /status_saida") >= 0) {
        // Retorne JSON apenas com os sensores/reles do portão de saída
        Status current = readStatus();
        static unsigned long lastSend = 0;
        static Status lastSentStatus = {0,0,0,0,0,0};
        bool changed = statusChanged(current, lastSentStatus);

        if (changed || millis() - lastSend > 10000) { // força envio a cada X segundos
          lastSentStatus = current;
          lastSend = millis();
          String json = statusToJson(current);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println();
          client.print(json);
        } else {
          client.println("HTTP/1.1 204 No Content");
          client.println();
        }
    } else if (request.indexOf("GET /status_entrada") >= 0) {
        // Retorne JSON apenas com os sensores/reles do portão de entrada
        StatusEntrada current = readStatusEntrada();
        static unsigned long lastSend = 0;
        static StatusEntrada lastSentStatus = {0,0,0,0,0};
        bool changed = statusChangedEntrada(current, lastSentStatus);

        if (changed || millis() - lastSend > 10000) { // força envio a cada X segundos
          lastSentStatus = current;
          lastSend = millis();
          String json = statusEntradaToJson(current);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println();
          client.print(json);
        } else {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.print(html_home);
        }
    }
    client.stop();
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando o controlador");

  entradas.init();
  relecontrol.init();

  // Define as portas de entrada
  entradas.pinMode(PIN_FIM_CURSO_FECHADO, INPUT); // Fim de Curso Fechado
  entradas.pinMode(PIN_FIM_CURSO_ABERTO, INPUT);  // Fim de Curso Aberto
  entradas.pinMode(PIN_FOTOCELULA, INPUT);        // Fotocelula
  entradas.pinMode(PIN_LACO_MAGNETICO, INPUT);    // Laco Magnetico

  // Define as portas de saida
  relecontrol.pinMode(PIN_RELE_SEMAFORO, OUTPUT); // RELE1 - Semaforo Verde
  relecontrol.pinMode(PIN_RELE_UHF, OUTPUT);      // RELE2 - Inibicao UHF

  // Default inibido leitura e sinal vermelho
  inibirUHF();
  setSemaforoVermelho();

  // Default sinal vermelho portao entrada
  liberarUHFEntrada(); // Como nao temos sensor de laco, fica lendo direto
  setSemaforoVermelhoEntrada();

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.softAP(wifi_ssid, wifi_password);
  IPAddress myIP = WiFi.softAPIP(); // 192.168.4.1
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Setup done");

}

void loop()
{
  // Portao Saida
  bool fotocelula = (entradas.digitalRead(PIN_FOTOCELULA) == 0);
  bool laco = (entradas.digitalRead(PIN_LACO_MAGNETICO) == 0);
  bool fim_aberto = (entradas.digitalRead(PIN_FIM_CURSO_ABERTO) == 0);
  bool fim_fechado = (entradas.digitalRead(PIN_FIM_CURSO_FECHADO) == 0); 

  // Portao Entrada
  bool fim_fechado_entrada = (entradas.digitalRead(PIN_FIM_CURSO_FECHADO_ENTRADA) == 0);
  bool fim_aberto_entrada = (entradas.digitalRead(PIN_FIM_CURSO_ABERTO_ENTRADA) == 0);
  bool fotocelula_entrada = (entradas.digitalRead(PIN_FOTOCELULA_ENTRADA) == 0);

  bool processo_saida = true;
  bool processo_entrada = true;
  

  //---------------------------------------------------------------
  // Condicoes portao de saida
  //---------------------------------------------------------------

  // Protecao 1: caso tenha alguem no sensor, a saida será proibida
  if (processo_saida && fotocelula)
  {
    inibirUHF();
    // Colocar um delay caso o portao esteja aberto
    // Para evitar que quem abriu veja a luz vermelha ao passar
    if (fim_aberto)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay de 100ms
    }
    setSemaforoVermelho();
    ACIONOU_PHOTOCELULA = true;
    phototimer_start = millis(); // Inicia o contador
    processo_saida = false;
  }

    // Timeout para ACIONOU_PHOTOCELULA
  if (processo_saida && ACIONOU_PHOTOCELULA && (millis() - phototimer_start >= PHOTOCELULA_TIMEOUT)) {
    ACIONOU_PHOTOCELULA = false;
  }

  // Protecao 2: se acionada a photocelula, só liberar novamente com o portao 100% fechado
  if (processo_saida && fim_fechado)
  {
    ACIONOU_PHOTOCELULA = false;
  }

  // Protecao 3: Portão em movimento
  if (processo_saida && !fim_aberto && !fim_fechado)
  {
    inibirUHF();
    setSemaforoVermelho();
    processo_saida = false;
  }

  // Cenário 1: Portão fechado, sem carro no laco, Portaria sem ninguem
  if (processo_saida && fim_fechado && !laco)
  {
    inibirUHF();
    setSemaforoVermelho();
    processo_saida = false;
  }

  // Cenário 2: Carro chegou para sair e parou na frente da TAG
  if (processo_saida && fim_fechado && laco)
  {
    liberarUHF();
    setSemaforoVermelho();
    processo_saida = false;
  }

  // Cenário 3: Portão aberto, fotocelula desligada (carro parado na TAG esperando para sair)
  if (processo_saida && fim_aberto && !ACIONOU_PHOTOCELULA)
  {
    inibirUHF();
    setSemaforoVerde();
    processo_saida = false;
  }

  // Cenário 4: Portão aberto, carro ja saiu
  if (processo_saida && fim_aberto && ACIONOU_PHOTOCELULA)
  {
    inibirUHF();
    setSemaforoVermelho();
    processo_saida = false;
  }

  // Default: segurança
  if(processo_saida){
    inibirUHF();
    setSemaforoVermelho();
  }

   //---------------------------------------------------------------
  // Condicoes portao de entrada
  //---------------------------------------------------------------

  // Protecao 1: caso tenha alguem no sensor, a saida será proibida
  if (processo_entrada && fotocelula_entrada)
  {
    //inibirUHF(); // Se colocar laco habilitar
    // Colocar um delay caso o portao esteja aberto
    // Para evitar que quem abriu veja a luz vermelha ao passar
    if (fim_aberto_entrada)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay de 100ms
    }
    setSemaforoVermelhoEntrada();
    ACIONOU_PHOTOCELULA_ENTRADA = true;
    phototimer_start_entrada = millis(); // Inicia o contador
    processo_entrada = false;
  }

    // Timeout para ACIONOU_PHOTOCELULA
  if (processo_entrada && ACIONOU_PHOTOCELULA_ENTRADA && (millis() - phototimer_start_entrada >= PHOTOCELULA_TIMEOUT_ENTRADA)) {
    ACIONOU_PHOTOCELULA_ENTRADA = false;
  }

  // Protecao 2: se acionada a photocelula, só liberar novamente com o portao 100% fechado
  if (processo_entrada && fim_fechado_entrada)
  {
    ACIONOU_PHOTOCELULA_ENTRADA = false;
  }

  // Protecao 3: Portão em movimento
  if (processo_entrada && !fim_aberto_entrada && !fim_fechado_entrada)
  {
    inibirUHFEntrada();
    setSemaforoVermelhoEntrada();
    processo_entrada = false;
  }

  // Descomentar se colocar laco
  // Cenário 1: Portão fechado, sem carro no laco, Portaria sem ninguem
  //if (processo_entrada && fim_fechado && !laco)
  // {
  //  inibirUHF();
  //  setSemaforoVermelho();
  //  webPage();
  //  processo_entrada = false;
  // }

  // Cenário 2: Carro chegou para sair e parou na frente da TAG
  if (processo_entrada && fim_fechado_entrada) // && laco - Descomentar caso coloque laco
  {
    liberarUHFEntrada();
    setSemaforoVermelhoEntrada();
    processo_entrada = false;
  }

  // Cenário 3: Portão aberto, fotocelula desligada (carro parado na TAG esperando para sair)
  if (processo_entrada && fim_aberto_entrada && !ACIONOU_PHOTOCELULA_ENTRADA)
  {
    inibirUHFEntrada();
    setSemaforoVerdeEntrada();
    processo_entrada = false;
  }

  // Cenário 4: Portão aberto, carro ja saiu
  if (processo_entrada && fim_aberto_entrada && ACIONOU_PHOTOCELULA_ENTRADA)
  {
    inibirUHFEntrada();
    setSemaforoVermelhoEntrada();
    processo_entrada = false;
  }

  // Default: segurança
  if(processo_entrada){
    inibirUHFEntrada();
    setSemaforoVermelhoEntrada();
  }
  webPage();
  vTaskDelay(300 / portTICK_PERIOD_MS); // Delay de 100ms
}


