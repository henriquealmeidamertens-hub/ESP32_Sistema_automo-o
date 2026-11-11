#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <sys/time.h>
#include <time.h>

// --- MAPEAMENTO DOS PINOS ---
#define RST_PIN       4   
#define SS_PIN        5   
#define SCK_PIN       18  
#define MISO_PIN      19  
#define MOSI_PIN      23  

#define LED_VERDE     2   
#define LED_VERMELHO  22  
#define SENSOR_MAG    15  
#define RELE_PIN      21  

MFRC522 rfid(SS_PIN, RST_PIN);
WebServer server(80);

// --- VARIÁVEIS UNIFICADAS DO SISTEMA ---
String ultimoUid = "-- -- -- --";
String ultimaAcao = "Sistema iniciado (Aguardando)";
bool portaDestrancada = false; // false = Trancada, true = Destrancada
const String tagAutorizada = "F1 49 27 66";
bool relogioSincronizado = false;

// --- OBTENÇÃO DO IP DO CLIENTE WEB ---
String getIpClienteWeb() {
  return "[Cliente IP: " + server.client().remoteIP().toString() + "]";
}

// --- OBTENÇÃO DA DATA E HORA REAL (FUSO HORÁRIO DE BRASÍLIA) ---
String getObterDataHora() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    char timeStringBuff[35];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S (BRT)", &timeinfo);
    return String(timeStringBuff);
  }
  
  unsigned long totalSegundos = millis() / 1000;
  unsigned long horas = totalSegundos / 3600;
  unsigned long minutos = (totalSegundos % 3600) / 60;
  unsigned long segundos = totalSegundos % 60;
  
  char uptimeBuff[40];
  snprintf(uptimeBuff, sizeof(uptimeBuff), "[Sem Sinc. BRT] Uptime %02lu:%02lu:%02lu", horas, minutos, segundos);
  return String(uptimeBuff);
}

// --- SISTEMA DE LOGS PERSISTENTE NA FLASH ---
void registrarLog(String tipo, String mensagem) {
  String timestamp = getObterDataHora();
  String entradaLog = "[" + timestamp + "] [" + tipo + "] " + mensagem + "\n";
  
  Serial.print(entradaLog);

  File file = SPIFFS.open("/log.txt", FILE_APPEND);
  if (!file) {
    Serial.println("Erro ao abrir arquivo de log para escrita em Flash!");
    return;
  }
  file.print(entradaLog);
  file.close();
}

String lerLogsFlash() {
  if (!SPIFFS.exists("/log.txt")) {
    return "Nenhum log gravado até o momento.";
  }
  File file = SPIFFS.open("/log.txt", FILE_READ);
  if (!file) {
    return "Erro ao ler histórico da memória Flash.";
  }
  String conteudo = file.readString();
  file.close();
  return conteudo;
}

void limparLogsFlash(String infoCliente) {
  SPIFFS.remove("/log.txt");
  registrarLog("INFO", "Histórico de logs limpo via Web " + infoCliente);
}

// --- INTERFACE WEB UNIFICADA ---
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle de Acesso</title>
  <style>
    * { box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
    body { background-color: #f1f5f9; margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
    .container { background: #ffffff; width: 100%; max-width: 520px; border-radius: 16px; padding: 25px; box-shadow: 0 10px 25px rgba(0,0,0,0.08); }
    h1 { font-size: 22px; color: #0f172a; margin-top: 0; margin-bottom: 5px; text-align: center; }
    .sys-info { font-size: 11px; color: #64748b; text-align: center; margin-bottom: 20px; font-family: monospace; }
    
    .section-title { font-size: 12px; font-weight: 700; color: #64748b; text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 8px; border-bottom: 1px solid #e2e8f0; padding-bottom: 4px; }
    
    .sensor-box { padding: 12px; border-radius: 10px; font-weight: bold; font-size: 15px; margin-bottom: 20px; text-align: center; border: 1px solid transparent; transition: all 0.3s; }
    .bg-green { background-color: #dcfce7; color: #166534; border-color: #bbf7d0; }
    .bg-red { background-color: #fee2e2; color: #991b1b; border-color: #fecaca; }
    
    .card-block { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 12px; padding: 15px; margin-bottom: 20px; }
    .status-lbl { font-size: 15px; font-weight: 700; color: #334155; text-align: center; }
    
    .btn-group { display: flex; gap: 10px; margin-bottom: 20px; }
    .btn { flex: 1; border: none; padding: 12px; font-size: 15px; font-weight: 600; border-radius: 8px; cursor: pointer; transition: all 0.2s; color: #ffffff; text-align: center; text-decoration: none; display: inline-block; }
    .btn-abrir { background-color: #16a34a; }
    .btn-abrir:hover { background-color: #15803d; }
    .btn-fechar { background-color: #dc2626; }
    .btn-fechar:hover { background-color: #b91c1c; }
    .btn-log { background-color: #475569; }
    .btn-log:hover { background-color: #334155; }

    textarea { width: 100%; height: 180px; background: #0f172a; color: #38bdf8; font-family: monospace; font-size: 11px; padding: 10px; border-radius: 8px; border: none; resize: vertical; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Painel de Controle</h1>
    <div class="sys-info">IP ESP32: <span id="ipTxt">--</span></div>
    
    <!-- 1. Sensor Magnético -->
    <div class="section-title">1. Sensor Magnético (Porta Física)</div>
    <div id="sensorCard" class="sensor-box bg-green">
      <span id="sensorTxt">Aguardando...</span>
    </div>

    <!-- 2. Relé / Trava Eletrônica -->
    <div class="section-title">2. Trava Solenoide (Relé)</div>
    <div id="releCard" class="sensor-box bg-red">
      <span id="releTxt">Aguardando...</span>
    </div>

    <!-- 3. Controles Manuais -->
    <div class="section-title">3. Controle Manual Web</div>
    <div class="btn-group">
      <button class="btn btn-abrir" onclick="abrirWeb()">Destrancar</button>
      <button class="btn btn-fechar" onclick="fecharWeb()">Trancar</button>
    </div>

    <!-- 4. Registro da Ação -->
    <div class="section-title">4. Última Ação do Sistema</div>
    <div class="card-block">
      <div id="acaoTxt" class="status-lbl">Sistema Iniciado</div>
      <div style="font-size: 12px; margin-top:8px; color: #64748b; text-align: center;">Tag Lida: <span id="uidTxt" style="font-family: monospace; font-weight: bold; color: #2563eb;">-- -- -- --</span></div>
    </div>

    <!-- 5. Sistema de Logs Persistentes -->
    <div class="section-title">5. Histórico de Logs (Com IP do Cliente)</div>
    <textarea id="logArea" readonly>Carregando logs da memória Flash...</textarea>
    <div class="btn-group" style="margin-top: 10px; margin-bottom: 0;">
      <button class="btn btn-log" onclick="carregarLogs()">Atualizar Logs</button>
      <button class="btn btn-fechar" style="font-size: 13px;" onclick="limparLogs()">Limpar Flash</button>
    </div>
  </div>

  <script>
    function sincronizarHorarioReal() {
      let epochSegundos = Math.floor(Date.now() / 1000);
      fetch('/sync-tempo?epoch=' + epochSegundos)
        .then(() => console.log('Relógio do ESP32 sincronizado.'))
        .catch(err => console.error(err));
    }

    function atualizarStatus() {
      fetch('/dados')
        .then(res => res.json())
        .then(data => {
          document.getElementById('ipTxt').innerText = data.ip;

          let sensorBox = document.getElementById('sensorCard');
          let sensorTxt = document.getElementById('sensorTxt');
          if (data.sensor === 'ABERTA') {
            sensorTxt.innerText = 'PORTA ABERTA (Desalinhada)';
            sensorBox.className = 'sensor-box bg-red';
          } else {
            sensorTxt.innerText = 'PORTA FECHADA (Encostada)';
            sensorBox.className = 'sensor-box bg-green';
          }

          let releBox = document.getElementById('releCard');
          let releTxt = document.getElementById('releTxt');
          if (data.tranca === 'DESTRANCADA') {
            releTxt.innerText = 'DESTRANCADA (Pino Recolhido)';
            releBox.className = 'sensor-box bg-green';
          } else {
            releTxt.innerText = 'TRANCADA (Pino para Fora)';
            releBox.className = 'sensor-box bg-red';
          }

          document.getElementById('acaoTxt').innerText = data.acao;
          document.getElementById('uidTxt').innerText = data.uid;
        }).catch(err => console.error(err));
    }

    function carregarLogs() {
      fetch('/ver-logs')
        .then(res => res.text())
        .then(text => {
          let logArea = document.getElementById('logArea');
          logArea.value = text;
          logArea.scrollTop = logArea.scrollHeight;
        });
    }

    function abrirWeb() { fetch('/abrir').then(() => { atualizarStatus(); carregarLogs(); }); }
    function fecharWeb() { fetch('/fechar').then(() => { atualizarStatus(); carregarLogs(); }); }
    function limparLogs() {
      if(confirm("Tem certeza que deseja apagar o histórico de logs da Flash?")) {
        fetch('/limpar-logs').then(() => carregarLogs());
      }
    }

    sincronizarHorarioReal();
    setInterval(atualizarStatus, 1000);
    carregarLogs();
  </script>
</body>
</html>
)rawliteral";

// --- FUNÇÃO PARA MANTER O LEITOR ACORDADO ---
void reiniciarRFID() {
  rfid.PCD_Init();
  delay(10);
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
}

// --- FUNÇÕES DE HARDWARE ---
void destrancarPorta() {
  portaDestrancada = true;
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(RELE_PIN, LOW);
  
  delay(50);
  reiniciarRFID(); 
}

void trancarPorta() {
  portaDestrancada = false;
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(RELE_PIN, HIGH);
  
  delay(50);
  reiniciarRFID(); 
}

void piscarVermelhoNegado() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_VERMELHO, LOW);
    delay(100);
    digitalWrite(LED_VERMELHO, HIGH);
    delay(100);
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  setenv("TZ", "BRT3", 1);
  tzset();

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao inicializar o SPIFFS!");
  } else {
    registrarLog("INFO", "Sistema SPIFFS montado com sucesso.");
  }

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(RELE_PIN, OUTPUT);
  pinMode(SENSOR_MAG, INPUT_PULLUP);
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  trancarPorta(); 

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  reiniciarRFID();

  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAP("Trabalho_ESP32", "12345678");
  delay(100);

  registrarLog("INFO", "ESP32 Conectado AP [IP ESP32: " + WiFi.softAPIP().toString() + "]");

  server.on("/", []() { server.send(200, "text/html", htmlPage); });

  server.on("/sync-tempo", []() {
    if (server.hasArg("epoch")) {
      time_t epoch = server.arg("epoch").toInt();
      struct timeval tv = { epoch, 0 };
      settimeofday(&tv, NULL);
      
      if (!relogioSincronizado) {
        relogioSincronizado = true;
        registrarLog("INFO", "Relógio sincronizado via " + getIpClienteWeb());
      }
      server.send(200, "text/plain", "Sincronizado");
    } else {
      server.send(400, "text/plain", "Parâmetro ausente");
    }
  });

  server.on("/dados", []() {
    bool aberta = (digitalRead(SENSOR_MAG) == HIGH);
    String estadoSensor = aberta ? "ABERTA" : "FECHADA";
    String estadoTranca = portaDestrancada ? "DESTRANCADA" : "TRANCADA";

    String json = "{";
    json += "\"uid\":\"" + ultimoUid + "\",";
    json += "\"tranca\":\"" + estadoTranca + "\",";
    json += "\"acao\":\"" + ultimaAcao + "\",";
    json += "\"sensor\":\"" + estadoSensor + "\",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
    json += "}";

    server.send(200, "application/json", json);
  });

  server.on("/abrir", []() {
    destrancarPorta();
    String clienteIp = getIpClienteWeb();
    ultimaAcao = "Comando WEB: Porta Destrancada";
    registrarLog("INFO", "Porta destrancada via WEB " + clienteIp);
    server.send(200, "text/plain", "OK");
  });

  server.on("/fechar", []() {
    trancarPorta();
    String clienteIp = getIpClienteWeb();
    ultimaAcao = "Comando WEB: Porta Trancada";
    registrarLog("INFO", "Porta trancada via WEB " + clienteIp);
    server.send(200, "text/plain", "OK");
  });

  server.on("/ver-logs", []() {
    server.send(200, "text/plain", lerLogsFlash());
  });

  server.on("/limpar-logs", []() {
    limparLogsFlash(getIpClienteWeb());
    server.send(200, "text/plain", "Logs Limpos");
  });

  server.begin();
  registrarLog("INFO", "Sistema de Controle de Acesso iniciado.");
}

// --- PROCESSAMENTO ISOLADO DO RFID ---
void verificarRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uidLido = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidLido += (rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uidLido.toUpperCase();
    uidLido += String(rfid.uid.uidByte[i], HEX);
  }
  uidLido.trim();

  ultimoUid = uidLido;

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  if (uidLido == tagAutorizada) {
    if (!portaDestrancada) {
      destrancarPorta();
      ultimaAcao = "Tag Autorizada: Porta DESTRANCADA";
      registrarLog("INFO", "Acesso LIBERADO por Tag [" + uidLido + "]. Porta destrancada.");
    } else {
      trancarPorta();
      ultimaAcao = "Tag Autorizada: Porta TRANCADA";
      registrarLog("INFO", "Acesso LIBERADO por Tag [" + uidLido + "]. Porta trancada.");
    }
  } else {
    ultimaAcao = "Tag RECUSADA (Acesso Negado)";
    registrarLog("ERRO", "Tentativa de acesso NEGADA. Tag não reconhecida: [" + uidLido + "].");
    piscarVermelhoNegado();
  }

  delay(1000); 
}

// --- MONITORAMENTO DO SENSOR MAGNÉTICO (ALERTA DE SEGURANÇA) ---
unsigned long ultimoChequeSensor = 0;
bool estadoSensorAnterior = false;

void verificarSensorMag() {
  if (millis() - ultimoChequeSensor > 500) {
    ultimoChequeSensor = millis();
    bool estaAberta = (digitalRead(SENSOR_MAG) == HIGH);

    if (estaAberta != estadoSensorAnterior) {
      if (estaAberta && !portaDestrancada) {
        registrarLog("AVISO", "Alerta: Porta foi forçada ou aberta fisicamente enquanto trancada!");
      } else if (estaAberta) {
        registrarLog("INFO", "Porta aberta fisicamente.");
      } else {
        registrarLog("INFO", "Porta fechada fisicamente.");
      }
      estadoSensorAnterior = estaAberta;
    }
  }
}

// --- LOOP PRINCIPAL ---
void loop() {
  server.handleClient(); 
  verificarRFID();       
  verificarSensorMag();
}