#include <Arduino.h>

String  html_entrada = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <title>Portão de Entrada</title>
  <style>
    body { font-family: Arial; background: #f4f4f4; margin:0; padding:0;}
    h1 { background: #003366; color: #fff; margin:0; padding:20px;}
    table { border-collapse: collapse; width:90%; margin:30px auto; background:#fff;}
    th, td { border:1px solid #ccc; padding:10px; text-align:center;}
    th { background:#003366; color:#fff;}
    tr:nth-child(even){background:#f2f2f2;}
    .status-on { color:green; font-weight:bold;}
    .status-off { color:red; font-weight:bold;}
    .back-btn { display:block; width:120px; margin:30px auto 0 auto; padding:10px 0; background:#003366; color:#fff; text-align:center; border-radius:8px; text-decoration:none;}
    .back-btn:hover { background:#0055aa;}
  </style>
  <script>
    let lastData = {};
    function atualizar() {
      fetch('/status_entrada').then(r=>{
        if(r.status==200) return r.json();
        else throw 'nochange';
      }).then(data=>{
        if(JSON.stringify(data)!==JSON.stringify(lastData)){
          lastData=data;
          let now=new Date();
          let row='<tr>';
          row+='<td>'+now.toLocaleString()+'</td>';
          row+='<td class="'+(data.fotocelula?'status-on':'status-off')+'">'+(data.fotocelula?'ATIVADO':'DESATIVADO')+'</td>';
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
    //window.onload=atualizar;
  </script>
</head>
<body>
  <h1>Portão de Entrada</h1>
  <a href="/" class="back-btn">Voltar</a>
  <table>
    <thead>
      <tr>
        <th>Data e Hora</th>
        <th>Sensor Fotocelula</th>
        <th>Fim Curso Aberto</th>
        <th>Fim Curso Fechado</th>
        <th>Leitora UFH</th>
        <th>Semáforo</th>
      </tr>
    </thead>
    <tbody id="tbody"></tbody>
  </table>
</body>
</html>
)rawliteral";