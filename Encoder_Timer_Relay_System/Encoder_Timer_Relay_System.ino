#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>    
#include <PubSubClient.h> 

#define sw 0
#define dt 1
#define clk 2

#define i2c_scl 5
#define i2c_sda 6

#define buzzer 7
#define led 8
#define rele 9
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// --- CONFIGURAÇÕES MQTT ---
const char* mqtt_broker = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topic_comandos = "meu_esp32_c3_supermini/comandos";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long ultimoTentoMQTT = 0;
// --------------------------

// --- VARIÁVEIS DE SENHA E TEMPO ---
int senha[4] = {0, 0, 0, 0};          // Guarda o que está sendo digitado no hardware
int senhaMestra[4] = {5, 5, 5, 5};    // A senha real do sistema (pode ser alterada via site)
int senhaCorreta = 0;
int errouSenha = 0;
int estadoSenha = 0;

int minutos = 10;
int segundos = 0;
int estado = 0;
int estadoLigado = 0;
int estadoUltimoCLK;
int estadoAtualCLK;

unsigned long tempoInicioClique = 0;
unsigned long tempoSegundosAnterior = 0;
unsigned long ultimoTempoGiro = 0;
int intervalo = 1000;
int estadoAnteriorBotao = HIGH;

void desenharTela() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if(senhaCorreta == 0){
    if(errouSenha == 0){
      display.print("Informe a Senha");
      
      char senhaFormatada[16]; 
      sprintf(senhaFormatada, "%d %d %d %d", senha[0], senha[1], senha[2], senha[3]); 
      
      display.setTextSize(2);
      display.setCursor(15, 20);
      display.print(senhaFormatada);

      int alturaLinha = 40; 
      int xInicio = 15 + (estadoSenha * 24); 
      display.drawLine(xInicio, alturaLinha, xInicio + 10, alturaLinha, SSD1306_WHITE);
    }else{
      display.setTextSize(2);
      display.setCursor(34, 15);
      display.print("Senha");
      display.setCursor(4, 35);
      display.print("Incorreta!");
      errouSenha = 0;
    }
  }
  else{
    if(estadoLigado == 0){
      if(estado == 0){
        display.print("Editando: Minutos");
      }else{
        display.print("Editando: Segundos");
      }
    }else{
      display.print("Ligado...");
    }
    char tempoFormatado[6]; 
    sprintf(tempoFormatado, "%02d:%02d", minutos, segundos); 
    
    display.setTextSize(3);
    display.setCursor(15, 20);
    display.print(tempoFormatado);

    int alturaLinha = 45;
    if(estadoLigado == 0){
      if(estado == 0) {
        display.drawLine(15, alturaLinha, 45, alturaLinha, SSD1306_WHITE);
      } else {
        display.drawLine(70, alturaLinha, 100, alturaLinha, SSD1306_WHITE);
      }
    }
  }
  display.display();
}

// --- FUNÇÃO QUE RECEBE COMANDOS DA WEB ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  // Limpa caracteres ocultos que fazem o código falhar
  msg.trim(); 
  
  Serial.print("Mensagem Limpa Recebida: [");
  Serial.print(msg);
  Serial.println("]");

  // 1. Comando para LIGAR o relé
  if (msg.startsWith("LIGAR")) {
    int espaco1 = msg.indexOf(' ');
    int espaco2 = msg.lastIndexOf(' ');
    if (espaco1 != -1 && espaco2 != -1 && espaco1 != espaco2) {
      minutos = msg.substring(espaco1 + 1, espaco2).toInt();
      segundos = msg.substring(espaco2 + 1).toInt();
      
      senhaCorreta = 1; // Desbloqueia
      estadoLigado = 1; // Liga o relé
      tempoSegundosAnterior = millis();
      digitalWrite(rele, HIGH);
      desenharTela();
    }
  } 
  // 2. Comando para DESLIGAR o relé
  else if (msg == "DESLIGAR") {
    estadoLigado = 0;
    minutos = 10;
    segundos = 0;
    digitalWrite(rele, LOW);
    digitalWrite(led, LOW);
    digitalWrite(buzzer, LOW);
    desenharTela();
  }
  
  // --- NOVO: Comando apenas para DESBLOQUEAR O ECRÃ ---
  else if (msg == "DESBLOQUEAR") {
    senhaCorreta = 1; // Salta a verificação da palavra-passe física
    estadoLigado = 0; // Mantém o relé desligado, vai para edição de tempo
    desenharTela();
  }
  // --- NOVO: Comando para TRANCAR TUDO (Bloquear) ---
  else if (msg == "BLOQUEAR") {
    senhaCorreta = 0; // Volta a pedir palavra-passe
    estadoLigado = 0;
    minutos = 10;
    segundos = 0;
    digitalWrite(rele, LOW);
    digitalWrite(led, LOW);
    digitalWrite(buzzer, LOW);
    for(int i = 0; i < 4; i++){ senha[i] = 0; }
    estadoSenha = 0;
    desenharTela();
  }
  
  // 3. Comando para atualizar a SENHA MESTRA
  else if (msg.startsWith("SENHA ")) {
    String novaSenha = msg.substring(6); 
    if(novaSenha.length() >= 4) {
      senhaMestra[0] = novaSenha.charAt(0) - '0';
      senhaMestra[1] = novaSenha.charAt(1) - '0';
      senhaMestra[2] = novaSenha.charAt(2) - '0';
      senhaMestra[3] = novaSenha.charAt(3) - '0';
      Serial.print("Nova senha mestra definida via MQTT: ");
      Serial.println(novaSenha);
    }
  }
}

// --- FUNÇÃO PARA CONECTAR NO MQTT SEM TRAVAR O CÓDIGO ---
void conectarMQTT() {
  // Tenta conectar apenas a cada 10 segundos para não congelar o modo manual
  if (millis() - ultimoTentoMQTT > 10000) {
    Serial.println("Tentando conectar ao MQTT...");
    if (client.connect("ESP32_C3_TimerClient")) {
      Serial.println("MQTT Conectado!");
      client.subscribe(topic_comandos);
    }
    ultimoTentoMQTT = millis();
  }
}

void setup() {
  Serial.begin(115200); 
  Wire.begin(i2c_sda, i2c_scl);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    for(;;);
  }
  pinMode(clk, INPUT_PULLUP);
  pinMode(dt, INPUT_PULLUP);
  pinMode(sw, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(rele, OUTPUT);
  pinMode(led, OUTPUT);
  estadoUltimoCLK = digitalRead(clk);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Iniciando...");
  display.display();

  // --- CONFIGURAÇÃO WI-FI PARA NÃO TRAVAR O MODO MANUAL ---
  WiFiManager wm;
  // Se não conectar no Wi-Fi em 60 segundos, ele desiste e vai pro modo manual offline
  wm.setConfigPortalTimeout(60); 
  
  if (!wm.autoConnect("ESP32_Config")) {
    Serial.println("Falha ao conectar no WiFi. Entrando no modo Manual Offline.");
  } else {
    Serial.println("WiFi Conectado!");
  }
  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  
  desenharTela();
}

void loop() {

  // --- MANTER CONEXÃO WEB APENAS SE TIVER INTERNET ---
  // Se o Wi-Fi cair, ignoramos o MQTT e o modo manual voa liso!
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      conectarMQTT();
    }
    client.loop();
  }
  // ----------------------------------------------------
  
  int estadoAtualBotao = digitalRead(sw);

  if (estadoAtualBotao != estadoAnteriorBotao) {
      
    if (estadoAtualBotao == LOW) {
      tempoInicioClique = millis();
    } 
    
    else {
      unsigned long tempoPressionado = millis() - tempoInicioClique;
      if(senhaCorreta == 0){
        if(tempoPressionado >= 5000){
          // Verifica se o que foi digitado bate com a SENHA MESTRA atual
          if(senha[0] == senhaMestra[0] && senha[1] == senhaMestra[1] && senha[2] == senhaMestra[2] && senha[3] == senhaMestra[3]){
            senhaCorreta = 1;
          }
          else{
            senhaCorreta = 0;
            errouSenha = !errouSenha;
            estadoSenha = 0;
            for(int i =0; i < 4; i++){
              senha[i] = 0;
            }
            desenharTela();
            digitalWrite(buzzer, HIGH);
            delay(2000);
            digitalWrite(buzzer, LOW);
          }
          desenharTela();
        }
        else if (tempoPressionado > 50) { 
          estadoSenha++;
          if(estadoSenha == 4) estadoSenha = 0;
          desenharTela();
        }
      }
      
      //Senha confirmada
      else if(senhaCorreta == 1){
        if(tempoPressionado >= 10000){
          senhaCorreta = 0;
          estadoLigado = 0;
          estadoSenha = 0;
          digitalWrite(led, LOW);
          digitalWrite(buzzer, LOW);
          digitalWrite(rele, LOW);
          for(int i =0; i < 4; i++){senha[i] = 0;}
          desenharTela();
        }
        else if (tempoPressionado >= 4000) { 
          estadoLigado = !estadoLigado;
          if(estadoLigado == 1) tempoSegundosAnterior = millis();
          desenharTela();
        }
        else if (tempoPressionado > 50 && estadoLigado == 0) { 
          estado = !estado;
          desenharTela();
        }
      }
    }
  }
  estadoAnteriorBotao = estadoAtualBotao;

  if(senhaCorreta == 0){
    estadoAtualCLK = digitalRead(clk);
    if(estadoAtualCLK != estadoUltimoCLK && estadoAtualCLK == 1 ){
      if(millis() - ultimoTempoGiro > 5){
        int direcao = (digitalRead(dt) != estadoAtualCLK) ? 1 : -1;
        senha[estadoSenha] += direcao;
        if(senha[estadoSenha] > 9) senha[estadoSenha] = 0;
        if(senha[estadoSenha] < 0) senha[estadoSenha] = 9;
        ultimoTempoGiro = millis();
      }
      delay(10);
      desenharTela();
    }
    estadoUltimoCLK = estadoAtualCLK;
  }

  if(senhaCorreta == 1){
    if(estadoLigado == 0){
      digitalWrite(led, LOW);
      digitalWrite(buzzer, LOW);
      digitalWrite(rele, LOW);
      estadoAtualCLK = digitalRead(clk);
      if(estadoAtualCLK != estadoUltimoCLK && estadoAtualCLK == 1 ){
        if(millis() - ultimoTempoGiro > 5){
          int direcao = (digitalRead(dt) != estadoAtualCLK) ? 1 : -1;
          if(estado == 0){
            minutos += direcao;
            if(minutos > 30) minutos = 0;
            if(minutos < 0) minutos = 30;
          }
          else if(estado == 1){
            segundos += direcao;
            if(segundos > 59) segundos = 0;
            if(segundos < 0) segundos = 59;
          }
          ultimoTempoGiro = millis();
          delay(10);
          desenharTela();
        }
      }
      estadoUltimoCLK = estadoAtualCLK;
    }
    else{
      bool atualizarTela = false;
      if(minutos == 0 && segundos == 0){
        digitalWrite(rele, LOW);
        digitalWrite(led, LOW);
        digitalWrite(buzzer, LOW);
        estadoLigado = 0;
        minutos = 10;
        desenharTela();
      }
      else{
        digitalWrite(rele, HIGH);
        unsigned long tempoSegundosAtual = millis();
        if(tempoSegundosAtual - tempoSegundosAnterior >= intervalo){
          tempoSegundosAnterior = tempoSegundosAtual;
          segundos--;
          atualizarTela = true;
        }
        if(segundos < 0){
          segundos = 59;
          minutos--;
          atualizarTela = true;
        }
        estadoAtualCLK = digitalRead(clk);
        if (estadoAtualCLK != estadoUltimoCLK && estadoAtualCLK == 1) {
          if (millis() - ultimoTempoGiro > 5) {
            int direcao = (digitalRead(dt) != estadoAtualCLK) ? 1 : -1;
            minutos = constrain(minutos + direcao, 0, 30);
            ultimoTempoGiro = millis();
            atualizarTela = true;
          }
        }
        estadoUltimoCLK = estadoAtualCLK;
        if(minutos < 5){
          digitalWrite(led, HIGH);
          if(segundos == 59) digitalWrite(buzzer, HIGH);
          else digitalWrite(buzzer, LOW);
        }
        else{
          digitalWrite(led, LOW);
          digitalWrite(buzzer, LOW);
        }
      }
      if(atualizarTela == true) desenharTela();
    }
  }
}