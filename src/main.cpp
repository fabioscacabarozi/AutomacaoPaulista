#include <Arduino.h>

// Leitura das Portas
#include <PortExpander_I2C.h>

// WIFI
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

// Objetos para comunicação com as portas
PortExpander_I2C relecontrol(0x27);
PortExpander_I2C entradas(0x20);

// GPIO ENTRADA
#define PIN_FIM_CURSO_FECHADO 3 // Entrada 1 - Fim de curso Fechado
#define PIN_FIM_CURSO_ABERTO 1  // Entrada 2 - Fim de curso Aberto
#define PIN_FOTOCELULA 0        // Entrada 3 - Fotocelula
#define PIN_LACO_MAGNETICO 2    // Entrada 4 - Laço

// GPIO SAIDA
#define PIN_RELE_SEMAFORO 0 // Rele 1
#define PIN_RELE_UHF 1      // Rele 2

// WIFI
const char *wifi_ssid = "automacao_paulista";
const char *wifi_password = "1234567890";

// Variáveis global
bool ACIONOU_PHOTOCELULA = false;

WiFiServer server(80);

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

  // Defaul inibido leitura e sinal vermelho
  relecontrol.digitalWrite(PIN_RELE_UHF, 0);
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO, 0);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.softAP(wifi_ssid, wifi_password);
  IPAddress myIP = WiFi.softAPIP(); // 192.168.4.1
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Setup done");

}

// Funções auxiliares
void inibirUHF()
{
  relecontrol.digitalWrite(PIN_RELE_UHF, 0);
}

void liberarUHF()
{
  relecontrol.digitalWrite(PIN_RELE_UHF, 1);
}

void setSemaforoVermelho()
{
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO, 0);
}

void setSemaforoVerde()
{
  relecontrol.digitalWrite(PIN_RELE_SEMAFORO, 1);
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

Status lastStatus = {0,0,0,0,0,0};

Status readStatus() {
  Status s;
  s.fotocelula = (entradas.digitalRead(PIN_FOTOCELULA) == 0);
  s.laco = (entradas.digitalRead(PIN_LACO_MAGNETICO) == 0);
  s.fim_aberto = (entradas.digitalRead(PIN_FIM_CURSO_ABERTO) == 0);
  s.fim_fechado = (entradas.digitalRead(PIN_FIM_CURSO_FECHADO) == 0);
  s.rele_uhf = (relecontrol.digitalRead(PIN_RELE_UHF) == 1);
  s.rele_semaforo = (relecontrol.digitalRead(PIN_RELE_SEMAFORO) == 1);
  return s;
}

bool statusChanged(Status &a, Status &b) {
  return memcmp(&a, &b, sizeof(Status)) != 0;
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

String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <meta charset='utf-8'>
  <title>Automação Paulista</title>
  <style>
  body { font-family: Arial; background: #f4f4f4; margin:0; padding:0;}
  h1 { background: #003366; color: #fff; margin:0; padding:20px;}
  table { border-collapse: collapse; width:90%; margin:30px auto; background:#fff;}
  th, td { border:1px solid #ccc; padding:10px; text-align:center;}
  th { background:#003366; color:#fff;}
  tr:nth-child(even){background:#f2f2f2;}
  .status-on { color:green; font-weight:bold;}
  .status-off { color:red; font-weight:bold;}
  </style>
  <script>
  let lastData = {};
  function atualizar() {
      fetch('/status').then(r=>{
          if(r.status==200) return r.json();
          else throw 'nochange';
      }).then(data=>{
          if(JSON.stringify(data)!==JSON.stringify(lastData)){
              lastData=data;
              let now=new Date();
              let row='<tr>';
              row+='<td>'+now.toLocaleString()+'</td>';
              row+='<td class="'+(data.fotocelula?'status-on':'status-off')+'">'+(data.fotocelula?'ATIVADO':'DESATIVADO')+'</td>';
              row+='<td class="'+(data.laco?'status-on':'status-off')+'">'+(data.laco?'ATIVADO':'DESATIVADO')+'</td>';
              row+='<td class="'+(data.fim_aberto?'status-on':'status-off')+'">'+(data.fim_aberto?'ATIVADO':'DESATIVADO')+'</td>';
              row+='<td class="'+(data.fim_fechado?'status-on':'status-off')+'">'+(data.fim_fechado?'ATIVADO':'DESATIVADO')+'</td>';
              row+='<td class="'+(data.rele_uhf?'status-on':'status-off')+'">'+(data.rele_uhf?'LENDO':'NÃO LENDO')+'</td>';
              row+='<td class="'+(data.rele_semaforo?'status-on':'status-off')+'">'+(data.rele_semaforo?'VERDE':'VERMELHO')+'</td>';
              row+='</tr>';
              document.getElementById('tbody').innerHTML=row+document.getElementById('tbody').innerHTML;
          }
      }).catch(()=>{});
  }
  setInterval(atualizar,1000);
  window.onload=atualizar;
  </script>
  </head>
  <body>
  <h1>Automação Paulista</h1>
  <table>
  <thead>
  <tr>
  <th>Data e Hora</th>
  <th>Sensor Fotocelula</th>
  <th>Laço Magnético</th>
  <th>Fim Curso Aberto</th>
  <th>Fim Curso Fechado</th>
  <th>Leitora UFH</th>
  <th>Semáforo</th>
  </tr>
  </thead>
  <tbody id="tbody">
  </tbody>
  </table>
  </body>
  </html>
  )rawliteral";

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
    if (request.indexOf("GET /status") >= 0) {
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
    } else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println();
      client.print(html);
    }
    client.stop();
  } 

}

void loop()
{
  bool fotocelula = (entradas.digitalRead(PIN_FOTOCELULA) == 0);
  bool laco = (entradas.digitalRead(PIN_LACO_MAGNETICO) == 0);
  bool fim_aberto = (entradas.digitalRead(PIN_FIM_CURSO_ABERTO) == 0);
  bool fim_fechado = (entradas.digitalRead(PIN_FIM_CURSO_FECHADO) == 0); 

  // Protecao 1: caso tenha alguem no sensor, a saida será proibida
  if (fotocelula)
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
    webPage();
    return;
  }

  // Protecao 2: se acionada a photocelula, só liberar novamente com o portao 100% fechado
  if (fim_fechado)
  {
    ACIONOU_PHOTOCELULA = false;
  }

  // Protecao 3: Portão em movimento
  if (!fim_aberto && !fim_fechado)
  {
    inibirUHF();
    setSemaforoVermelho();
    webPage();
    return;
  }

  // Cenário 1: Portão fechado, sem carro no laco, Portaria sem ninguem
  if (fim_fechado && !laco)
  {
    inibirUHF();
    setSemaforoVermelho();
    webPage();
    return;
  }

  // Cenário 2: Carro chegou para sair e parou na frente da TAG
  if (fim_fechado && laco)
  {
    liberarUHF();
    setSemaforoVermelho();
    webPage();
    return;
  }

  // Cenário 3: Portão aberto, fotocelula desligada (carro parado na TAG esperando para sair)
  if (fim_aberto && !ACIONOU_PHOTOCELULA)
  {
    inibirUHF();
    setSemaforoVerde();
    webPage();
    return;
  }

  // Cenário 4: Portão aberto, carro ja saiu
  if (fim_aberto && ACIONOU_PHOTOCELULA)
  {
    inibirUHF();
    setSemaforoVermelho();
    webPage();
    return;
  }

  // Default: segurança
  inibirUHF();
  setSemaforoVermelho();
  webPage();

  vTaskDelay(300 / portTICK_PERIOD_MS); // Delay de 100ms
}


