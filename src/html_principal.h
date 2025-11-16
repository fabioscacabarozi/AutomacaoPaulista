#include <Arduino.h>

String  html_home = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <title>Automação Paulista</title>
  <style>
    body { font-family: Arial; background: #f4f4f4; margin:0; padding:0;}
    h1 { background: #003366; color: #fff; margin:0; padding:20px;}
    .menu { margin: 40px auto; width: 300px; text-align: center;}
    .menu a { display:block; margin:20px 0; padding:20px; background:#003366; color:#fff; text-decoration:none; border-radius:8px; font-size:1.2em;}
    .menu a:hover { background:#0055aa; }
  </style>
</head>
<body>
  <h1>Automação Paulista</h1>
  <div class="menu">
    <a href="/saida">Portão de Saída</a>
    <a href="/entrada">Portão de Entrada</a>
  </div>
</body>
</html>
)rawliteral";

/**
 * 
 * 
 * String html = R"rawliteral(
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

void webPage2(){
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
 * 
 * 
 */