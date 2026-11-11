# ESP32 Sistema de Automação e Controle de Acesso 🔒📶

Projeto desenvolvido como um **Sistema Ciberfísico de Controle de Acesso** baseado no microcontrolador ESP32. O sistema realiza a leitura de credenciais RFID, controla uma trava solenoide de 12V via módulo relé, monitoriza a abertura física da porta com um sensor magnético e disponibiliza um painel Web interativo com registo persistente de logs na memória Flash (SPIFFS).

---

## 👥 Autores

* **Arthur Zanetti**
* **Henrique Mertens**
* **Vinicius Reis**
* **Local:** Curitiba / 2026

---

## 🚀 Funcionalidades

- **Identificação RFID:** Leitura de tags de 13.56 MHz com o módulo MFRC522[cite: 1, 14]. Credencial autorizada padrão: `F1 49 27 66
- **Modo Access Point (AP) / Wi-Fi:** Cria a sua própria rede sem fios (`Trabalho_ESP32`) para acesso direto no endereço IP `192.168.4.1`
- **Servidor Web Incorporado (Porta 80):** Interface desenvolvida em HTML/CSS/JS para trancar, destrancar e monitorizar os componentes em tempo real via navegador
- **Logs Persistentes em Flash (SPIFFS):** Histórico gravado no chip de memória Flash categorizado em `[INFO]`, `[AVISO]` e `[ERRO]`, contendo carimbo de data/hora (Fuso de Brasília) e IP do cliente.
- **Deteção de Intrusão / Alerta:** Gera alerta de segurança caso a porta seja aberta fisicamente sem autorização prévia.

---

## 🧩 Componentes do Sistema

| Componente | Função |
| :--- | :--- |
| **ESP32 Dev Module** | Processamento central, controlo de GPIO, rádio Wi-Fi e Servidor HTTP. |
| **Leitor RFID RC522** | Leitura sem fios da credencial do utilizador |
| **Tag RFID (13.56 MHz)** | Credencial de acesso cadastrada (`F1 49 27 66`) |
| **Módulo Relé 5V** | Interface de acionamento do circuito de potência da fechadura |
| **Mini Fechadura Solenoide 12V** | Atuador elétrico/mecânico de trancamento |
| **Sensor Magnético (Reed Switch)** | Deteção do alinhamento da porta física (Aberta/Fechada) |
| **LED Verde** | Sinalização visual de Acesso Autorizado (GPIO 2) |
| **LED Vermelho** | Sinalização visual de Acesso Negado / Alertas (GPIO 22) |
| **Fonte Externa 12V** | Alimentação dedicada para a fechadura solenoide. |
| **Protoboard e Jumpers** | Interligação dos sinais de baixa tensão |

---

## 🔌 Mapeamento de Pinos (Pinout)

| Componente / Sinal | Pino ESP32 (GPIO) |
| :--- | :--- |
| **RC522 RST** | GPIO 4 |
| **RC522 SDA / SS** | GPIO 5 |
| **RC522 SCK** | GPIO 18 |
| **RC522 MISO** | GPIO 19 |
| **RC522 MOSI** | GPIO 23 |
| **RC522 VCC / GND** | 3V3 / GND |
| **LED Verde** | GPIO 2 |
| **LED Vermelho** | GPIO 22 |
| **Sensor Magnético** | GPIO 15 |
| **Módulo Relé (IN)** | GPIO 21 |

---

## 🌐 Aplicação nas Camadas do Modelo TCP/IP

| Camada TCP/IP | Aplicação no Projeto |
| :--- | :--- |
| **Aplicação** | Interface Web servida via HTTP
| **Transporte** | Protocolo TCP na Porta 80 |
| **Internet** | Endereço IPv4 atribuído na rede (Ex: `192.168.4.1`) |
| **Acesso à Rede** | Conexão sem fios via Wi-Fi 

## TESTES E VALIDAÇÃO DA REDE
 ## Teste de alcance / conectividade (PING) - `ping 192.168.4.1` - Confirma a comunicação do computador/celular com o ESP 
<img width="1058" height="690" alt="image" src="https://github.com/user-attachments/assets/2ba3b6d7-157e-40bb-9e07-d79b91018486" />

 ## Teste da Porta HTTP -  `nc -zv 192.168.4.1 80 ` - Confirma que a porta 80 do servidor web está aberta e pronta para receber requisições TCP
 <img width="1068" height="190" alt="image" src="https://github.com/user-attachments/assets/6570b80e-b509-446c-bf66-758dbda08849" />

 





 
