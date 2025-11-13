#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// --- Configurações do Wi-Fi ---
const char* ssid = "S24 de Lawtrel";
const char* password = "qwertyuiop";

const long utcOffsetInSeconds = -10800; 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// --- Configurações do Display ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP8266WebServer server(80);

// --- Pinos ---
const int pinoTouch = 14;  // Pino D5 (GPIO14) para o TTP223
const int pinoBuzzer = 12; // <-- SOM: Pino D6 (GPIO12) para o Buzzer

// =========================================================================
// VARIÁVEIS DE ESTADO PRINCIPAIS
// =========================================================================
#define MODE_TAMAGOTCHI 0
#define MODE_GAME       1
int currentMode = MODE_TAMAGOTCHI; // Controla o modo atual

// --- Estado do Tamagotchi ---
int energia = 100;
int felicidade = 100;
int fome = 100;
bool estaDormindo = false;
unsigned long previousMillisStats = 0;
const long intervalStats = 30000;
unsigned long lastTouchTime = 0;
const long touchCooldown = 2000;
volatile int requestedAnimation = 0;
// 0=Nenhum, 1=Wakeup, 2=Blink, 3=HappyEye(doce), 4=Sleep, 5=Left, 6=Right, 7=HappyTouch(^^)

unsigned long lastInteractionTime = 0;
const long boredomInterval = 60000; // 1 minuto
String currentEmotionString = "contente";
String timeString = "--:--";


// --- Estado do Jogo --- //
int score = 0;
int high_score = 0;
float player_y = SCREEN_HEIGHT - 10;
float player_vel_y = 0;
bool player_is_jumping = false;
int obstacle_x = SCREEN_WIDTH;
int obstacle_width = 5;
int obstacle_height = 10;
int game_speed = 3;
#define PLAYER_X 10
#define PLAYER_SIZE 8
#define GROUND_Y (SCREEN_HEIGHT - 1)
#define GRAVITY -0.4
#define JUMP_FORCE 6

// --- Constantes dos Olhos e UI ---
#define UI_HEIGHT 10
const int REF_EYE_HEIGHT = 40;
const int REF_EYE_WIDTH = 40;
const int REF_SPACE_BETWEEN_EYE = 10;
const int REF_CORNER_RADIUS = 10;
int left_eye_height = REF_EYE_HEIGHT;
int left_eye_width = REF_EYE_WIDTH;
int left_eye_x = 0, left_eye_y = 0;
int right_eye_height = REF_EYE_HEIGHT;
int right_eye_width = REF_EYE_WIDTH;
int right_eye_x = 0, right_eye_y = 0;

// =========================================================================
// FUNÇÃO PARA DESENHAR A INTERFACE (BARRAS DE STATUS) (MODIFICADA)
// =========================================================================
void drawInterface() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int barWidth = 32; // Largura de cada barra
  int barHeight = 7;

  // --- Energia (E) ---
  display.setCursor(0, 0);
  display.print("E");
  display.drawRect(9, 0, barWidth + 2, barHeight, SSD1306_WHITE); // Borda
  int energiaWidth = map(energia, 0, 100, 0, barWidth);
  display.fillRect(10, 1, energiaWidth, barHeight - 2, SSD1306_WHITE); // Preenchimento

  // --- Felicidade (H) ---
  display.setCursor(44, 0);
  display.print("H"); 
  display.drawRect(53, 0, barWidth + 2, barHeight, SSD1306_WHITE); // Borda
  int felicidadeWidth = map(felicidade, 0, 100, 0, barWidth);
  display.fillRect(54, 1, felicidadeWidth, barHeight - 2, SSD1306_WHITE); // Preenchimento

  // --- Fome (F) ---
  display.setCursor(88, 0);
  display.print("F");
  display.drawRect(97, 0, barWidth + 2, barHeight, SSD1306_WHITE); // Borda
  int fomeWidth = map(fome, 0, 100, 0, barWidth);
  display.fillRect(98, 1, fomeWidth, barHeight - 2, SSD1306_WHITE); // Preenchimento
}

// =========================================================================
// FUNÇÕES DE ANIMAÇÃO
// =========================================================================

void draw_eyes(bool update = true) {
  display.clearDisplay();
  drawInterface();  // Chama a nova função de UI com 3 barras
  int x1 = left_eye_x - left_eye_width / 2;
  int y1 = left_eye_y - left_eye_height / 2;
  display.fillRoundRect(x1, y1, left_eye_width, left_eye_height, REF_CORNER_RADIUS, SSD1306_WHITE);
  int x2 = right_eye_x - right_eye_width / 2;
  int y2 = right_eye_y - right_eye_height / 2;
  display.fillRoundRect(x2, y2, right_eye_width, right_eye_height, REF_CORNER_RADIUS, SSD1306_WHITE);
  if (update) {
    display.display();
  }
}

void center_eyes(bool update = true) {
  left_eye_width = REF_EYE_WIDTH;
  left_eye_height = REF_EYE_HEIGHT;
  right_eye_width = REF_EYE_WIDTH;
  right_eye_height = REF_EYE_HEIGHT;
  int centerY = (SCREEN_HEIGHT - UI_HEIGHT) / 2 + UI_HEIGHT;
  left_eye_x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  left_eye_y = centerY;
  right_eye_x = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
  right_eye_y = centerY;
  if (update) {
    draw_eyes(true);
  }
}

// --- Animação ^^ (Carinho/Agradecimento) ---
void draw_happy_eyes_replacement() {
  //Toca um som feliz
  tone(pinoBuzzer, 988, 100); // Nota B5
  delay(100);
  tone(pinoBuzzer, 1318, 150); // Nota E6
  display.clearDisplay();
  drawInterface();
  int centerY = (SCREEN_HEIGHT - UI_HEIGHT) / 2 + UI_HEIGHT;
  int centerX_Left = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  int centerX_Right = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
  int eyeSize = 10;
  int eyeTopY = centerY - (eyeSize / 2);
  int eyeBottomY = centerY + (eyeSize / 2);
  display.drawLine(centerX_Left - eyeSize, eyeBottomY, centerX_Left, eyeTopY, SSD1306_WHITE);
  display.drawLine(centerX_Left, eyeTopY, centerX_Left + eyeSize, eyeBottomY, SSD1306_WHITE);
  display.drawLine(centerX_Right - eyeSize, eyeBottomY, centerX_Right, eyeTopY, SSD1306_WHITE);
  display.drawLine(centerX_Right, eyeTopY, centerX_Right + eyeSize, eyeBottomY, SSD1306_WHITE);
  display.display();
  delay(1500);
  center_eyes(true);
  noTone(pinoBuzzer);
}

// --- Animações "Doente" e "Sonolento Feliz" ---
void draw_sick_face() {
  tone(pinoBuzzer, 880, 150); delay(150);
  noTone(pinoBuzzer); delay(50);
  tone(pinoBuzzer, 880, 150);
  display.clearDisplay();
  drawInterface(); // Manter a UI
  int centerY = (SCREEN_HEIGHT - UI_HEIGHT) / 2 + UI_HEIGHT;
  int centerX_Left = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  int centerX_Right = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
  int eyeSize = 10; // Tamanho do X

  display.drawLine(centerX_Left - eyeSize/2, centerY - eyeSize/2, centerX_Left + eyeSize/2, centerY + eyeSize/2, SSD1306_WHITE);
  display.drawLine(centerX_Left - eyeSize/2, centerY + eyeSize/2, centerX_Left + eyeSize/2, centerY - eyeSize/2, SSD1306_WHITE);
  display.drawLine(centerX_Right - eyeSize/2, centerY - eyeSize/2, centerX_Right + eyeSize/2, centerY + eyeSize/2, SSD1306_WHITE);
  display.drawLine(centerX_Right - eyeSize/2, centerY + eyeSize/2, centerX_Right + eyeSize/2, centerY - eyeSize/2, SSD1306_WHITE);
  display.display();
  noTone(pinoBuzzer);
}

void draw_sleepy_happy_face() {
  tone(pinoBuzzer, 659, 200);
  center_eyes(false);
  left_eye_height = 5;
  right_eye_height = 5;
  draw_eyes(false);

  int leftBrowTopY = left_eye_y - (REF_EYE_HEIGHT / 2) - 8;
  int leftBrowBottomY = left_eye_y - (REF_EYE_HEIGHT / 2) - 2;
  display.drawLine(left_eye_x - 10, leftBrowBottomY, left_eye_x, leftBrowTopY, SSD1306_WHITE);
  display.drawLine(left_eye_x, leftBrowTopY, left_eye_x + 10, leftBrowBottomY, SSD1306_WHITE);

  int rightBrowTopY = right_eye_y - (REF_EYE_HEIGHT / 2) - 8;
  int rightBrowBottomY = right_eye_y - (REF_EYE_HEIGHT / 2) - 2;
  display.drawLine(right_eye_x - 10, rightBrowBottomY, right_eye_x, rightBrowTopY, SSD1306_WHITE);
  display.drawLine(right_eye_x, rightBrowTopY, right_eye_x + 10, rightBrowBottomY, SSD1306_WHITE);
  display.display();
  delay(100);
  noTone(pinoBuzzer);
}

void blink() {
  center_eyes(false);
  draw_eyes();
  for (int h = REF_EYE_HEIGHT; h >= 0; h -= 10) {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes();
  }
  delay(100);
  for (int h = 0; h <= REF_EYE_HEIGHT; h += 10) {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes();
  }
  center_eyes(true);
}

void wakeup() {
  estaDormindo = false;
  // Fanfarra de despertar: Tu-tu-tu-TUUUU!
  tone(pinoBuzzer, 1046, 100); delay(120); // C6
  tone(pinoBuzzer, 1046, 100); delay(120); // C6
  tone(pinoBuzzer, 1046, 100); delay(120); // C6
  tone(pinoBuzzer, 1318, 400); delay(400); // E6 (Longo)
  noTone(pinoBuzzer);

  center_eyes(false);
  for (int h = 0; h <= REF_EYE_HEIGHT; h += 2) {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes(true);
  }
}

void sleep_anim() {
  estaDormindo = true;
  center_eyes(false);
  for (int h = REF_EYE_HEIGHT; h >= 2; h -= 2) {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes(true);
  }
}

void happy_eye() {
  center_eyes(false);
  draw_eyes(false);
  int offset = REF_EYE_HEIGHT / 2;
  for (int i = 0; i < 10; i++) {
    display.fillTriangle(left_eye_x - left_eye_width / 2 - 1, left_eye_y + offset, left_eye_x + left_eye_width / 2 + 1, left_eye_y + 5 + offset, left_eye_x - left_eye_width / 2 - 1, left_eye_y + left_eye_height + offset, SSD1306_BLACK);
    display.fillTriangle(right_eye_x + right_eye_width / 2 + 1, right_eye_y + offset, right_eye_x - left_eye_width / 2 - 1, right_eye_y + 5 + offset, right_eye_x + right_eye_width / 2 + 1, right_eye_y + right_eye_height + offset, SSD1306_BLACK);
    offset -= 2;
    display.display();
    delay(10);
  }
  delay(1000);
  center_eyes(true);
}

void move_big_eye(int direction) {
  int direction_oversize = 1;
  int direction_movement_amplitude = 2;
  int blink_amplitude = 5;
  for (int i = 0; i < 6; i++) {
    left_eye_x += direction_movement_amplitude * direction;
    right_eye_x += direction_movement_amplitude * direction;
    if (i < 3) {
      right_eye_height -= blink_amplitude;
      left_eye_height -= blink_amplitude;
    } else {
      right_eye_height += blink_amplitude;
      left_eye_height += blink_amplitude;
    }
    if (direction > 0) {
      right_eye_width += direction_oversize;
      right_eye_height += direction_oversize;
    } else {
      left_eye_width += direction_oversize;
      left_eye_height += direction_oversize;
    }
    draw_eyes();
    delay(10);
  }
  delay(1000);
  for (int i = 0; i < 6; i++) {
    left_eye_x -= direction_movement_amplitude * direction;
    right_eye_x -= direction_movement_amplitude * direction;
    if (i < 3) {
      right_eye_height -= blink_amplitude;
      left_eye_height -= blink_amplitude;
    } else {
      right_eye_height += blink_amplitude;
      left_eye_height += blink_amplitude;
    }
    if (direction > 0) {
      right_eye_width -= direction_oversize;
      right_eye_height -= direction_oversize;
    } else {
      left_eye_width -= direction_oversize;
      left_eye_height -= direction_oversize;
    }
    draw_eyes();
    delay(10);
  }
  center_eyes();
}

void move_right_big_eye() {
  move_big_eye(1);
}
void move_left_big_eye() {
  move_big_eye(-1);
}

void draw_bored_animation() {
  tone(pinoBuzzer, 262, 100);
  move_left_big_eye();
  noTone(pinoBuzzer); // <-- SOM
  delay(500);
  tone(pinoBuzzer, 330, 100);
  move_right_big_eye();
  noTone(pinoBuzzer);
}

void sad_eye() {
  tone(pinoBuzzer, 220, 500);
  center_eyes(false);
  draw_eyes(false);
  display.fillTriangle(
    left_eye_x - left_eye_width / 2 + 5, left_eye_y - REF_EYE_HEIGHT / 2 + 10,
    left_eye_x + left_eye_width / 2 - 5, left_eye_y - REF_EYE_HEIGHT / 2 + 10,
    left_eye_x, left_eye_y - REF_EYE_HEIGHT / 2,
    SSD1306_WHITE);
  display.fillTriangle(
    right_eye_x - right_eye_width / 2 + 5, right_eye_y - REF_EYE_HEIGHT / 2 + 10,
    right_eye_x + right_eye_width / 2 - 5, right_eye_y - REF_EYE_HEIGHT / 2 + 10,
    right_eye_x, right_eye_y - REF_EYE_HEIGHT / 2,
    SSD1306_WHITE);
  display.display();
  delay(1500);
  center_eyes(true);
  noTone(pinoBuzzer);
}

void play_snore() {
  // Faz um som grave e vibrante (Rrr...)
  for(int i=0; i<3; i++) {
    tone(pinoBuzzer, 60); delay(100);
    tone(pinoBuzzer, 50); delay(100);
  }
  noTone(pinoBuzzer);
}

void play_whistle() {
  // Primeira nota (grave -> aguda)
  for (int f = 1000; f < 2000; f += 50) {
    tone(pinoBuzzer, f);
    delay(5);
  }
  noTone(pinoBuzzer);
  delay(100); // Pausa
  
  // Segunda nota (aguda e longa)
  tone(pinoBuzzer, 2000, 300);
  delay(300);
  noTone(pinoBuzzer);
}

// =========================================================================
// FUNÇÕES DO SERVIDOR WEB
// =========================================================================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Digitama Voz</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #2c3e50; color: white; display: flex; flex-direction: column; align-items: center; padding: 10px; margin: 0; }
    h1 { margin-bottom: 10px; }
    .face-display { font-size: 4.5em; color: #ecf0f1; text-align: center; margin: 10px 0; height: 80px; line-height: 80px; }
    .time-display { font-size: 1.2em; color: #bdc3c7; margin-bottom: 15px; }
    .stats { background-color: #34495e; padding: 10px; border-radius: 8px; margin-bottom: 20px; width: 90%; max-width: 400px; text-align: center; font-size: 1.1em; }
    .button-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; max-width: 400px; width: 90%; }
    button { background-color: #3498db; color: white; border: none; border-radius: 8px; padding: 20px; font-size: 1.2em; cursor: pointer; transition: background-color 0.3s; }
    button:hover { background-color: #2980b9; }
    .full-width { grid-column: 1 / -1; }
    .btn-mic { background-color: #e74c3c; font-weight: bold; }
    .btn-mic.listening { background-color: #c0392b; animation: pulse 1s infinite; }
    .btn-test { background-color: #9b59b6; font-size: 0.9em; margin-top: 10px; }
    @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.05); } 100% { transform: scale(1); } }
    #voice-status { font-size: 0.9em; color: #f1c40f; margin-top: 5px; min-height: 20px;}
  </style>
</head>
<body>
  <h1>Controle Digitama</h1>
  
  <div id="tamagotchi-face" class="face-display">(•‿•)</div>
  <div id="time-now" class="time-display">--:--</div>
  
  <div style="width: 90%; max-width: 400px; margin-bottom: 10px; text-align:center;">
    <button id="btn-voice" class="full-width btn-mic" onclick="toggleVoice()">🎙️ Falar com ele</button>
    <p id="voice-status">Toque para falar...</p>
    <button class="full-width btn-test" onclick="testAudio()">🔊 Testar Som do Telemóvel</button>
  </div>

  <div class="stats">
    E: <span id="energia">--</span>% | H: <span id="felicidade">--</span>% | F: <span id="fome">--</span>%
  </div>
  
  <div class="button-grid">
    <button data-action="jogar" class="full-width" style="background-color:#e67e22">Jogar</button>
    <button data-action="alimentar" style="background-color:#27ae60">Comer</button>
    <button data-action="happy">Brincar</button>
    <button data-action="wakeup">Acordar</button>
    <button data-action="sleep">Dormir</button>
    <button data-action="blink">Piscar</button>
    <button data-action="left">Esq.</button>
    <button data-action="right">Dir.</button>
  </div>
  
  <script>
    // --- Configuração de Voz (Web Speech API) ---
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    let recognition = null;
    if (SpeechRecognition) {
      recognition = new SpeechRecognition();
      recognition.lang = 'pt-BR';
      recognition.continuous = false;
    } else {
      document.getElementById('voice-status').innerText = "Seu navegador não suporta voz.";
    }
    
    let lastEmotion = "";
    
    // ===============================================
    // --- FUNÇÃO 'SPEAK' (VOZ) MODIFICADA (PLANO B) ---
    // ===============================================
    function speak(text) {
      if ('speechSynthesis' in window) {
        window.speechSynthesis.cancel();
        const u = new SpeechSynthesisUtterance(text);
        // Removemos a exigência do pt-BR para usar a voz padrão
        // u.lang = 'pt-BR'; 
        window.speechSynthesis.speak(u);
      } else {
        alert("API de Fala (Speech API) não encontrada!");
      }
    }

    function testAudio() {
      speak("Teste de áudio. Se ouvir isto, está a funcionar.");
    }

    function toggleVoice() {
      const statusEl = document.getElementById('voice-status');
      if (!recognition) {
        statusEl.innerText = "Erro: Navegador não suporta API de voz.";
        return;
      }
      try {
        recognition.start();
        document.getElementById('btn-voice').classList.add('listening');
        statusEl.innerText = "Ouvindo... Fale agora!";
      } catch (e) {
        statusEl.innerText = "Erro ao iniciar: " + e.message;
        if (e.message && e.message.includes("permission")) {
           alert("Permissão negada! No Android/Chrome, precisa ativar 'Insecure origins' em chrome://flags para IPs locais.");
        }
      }
    }

    if (recognition) {
      recognition.onresult = function(event) {
        const cmd = event.results[0][0].transcript.toLowerCase();
        document.getElementById('voice-status').innerText = "Ouvi: " + cmd;
        processVoiceCommand(cmd);
        document.getElementById('btn-voice').classList.remove('listening');
      };
      recognition.onerror = function(e) {
        let msg = e.error;
        if (e.error === 'not-allowed') msg = "Microfone bloqueado. Verifique permissões.";
        if (e.error === 'network') msg = "Erro de rede (HTTPS necessário?).";
        document.getElementById('voice-status').innerText = "Erro: " + msg;
        document.getElementById('btn-voice').classList.remove('listening');
      };
    }

    function processVoiceCommand(cmd) {
      if (cmd.includes("alimentar") || cmd.includes("comer")) triggerAction('alimentar', "Nham nham!");
      else if (cmd.includes("brincar") || cmd.includes("feliz")) triggerAction('happy', "Brincar!");
      else if (cmd.includes("dormir") || cmd.includes("sono")) triggerAction('sleep', "Zzz...");
      else if (cmd.includes("acordar") || cmd.includes("bom dia")) triggerAction('wakeup', "Bom dia!");
      else speak("Não entendi.");
    }

    function triggerAction(action, resp) {
      if (action === 'alimentar' || action === 'happy') { 
          document.getElementById('tamagotchi-face').innerText = "(^o^)"; 
      }
      speak(resp);
      fetch('/' + action).then(() => { getStats(); });
    }

    document.querySelectorAll('button[data-action]').forEach(b => {
      b.addEventListener('click', () => {
        fetch('/' + b.dataset.action).then(() => getStats());
      });
    });

    function getStats(){
      fetch('/stats').then(r => r.json()).then(d => {
        document.getElementById('energia').innerText = d.energia;
        document.getElementById('felicidade').innerText = d.felicidade;
        document.getElementById('fome').innerText = d.fome;
        document.getElementById('time-now').innerText = d.hora
        
        const faceEl = document.getElementById('tamagotchi-face');
        let face = "(•‿•)";

        if (d.emocao !== lastEmotion) {
           switch (d.emocao) {
             case "faminto": speak("Preciso de comida!"); break;
             case "triste": speak("Estou muito triste."); break;
             case "doente": speak("Não me sinto nada bem... Estou doente."); break;
             case "sonolento": speak("Que sono..."); break;
             case "entediado": speak("Não há nada para fazer?"); break;
           }
           lastEmotion = d.emocao;
        }
        
        switch (d.emocao) {
          case "faminto": case "triste": face = "(T_T)"; break;
          case "doente": face = "(X_X)"; break;
          case "sonolento": face = "(-_-)"; break;
          case "sonolento_feliz": face = "(^.^)"; break;
          case "entediado": face = "(•_•)"; break;
          case "dormindo": face = "(-_-) zZz"; break;
        }
        faceEl.innerText = face;
      });
    }
    setInterval(getStats, 4000); // <-- Atualiza a cada 4s para reduzir lag
    document.addEventListener('DOMContentLoaded', getStats);
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// --- Rota de Status ---
void handleStats() {
  String json = "{\"energia\":" + String(energia) + ", \"felicidade\":" + String(felicidade) + ", \"fome\":" + String(fome) + ", \"emocao\":\"" + currentEmotionString + "\", \"hora\":\"" + timeString + "\"}";
  server.send(200, "application/json", json);
}

void game_reset(); // Protótipo
void handleJogar() { // <-- RESTAURADO (Fase 2)
  server.send(200, "text/plain", "OK");
  if (currentMode == MODE_TAMAGOTCHI) {
    currentMode = MODE_GAME;
    game_reset();
    lastInteractionTime = millis();
  }
}
void handleAlimentar() {
  fome = 100;
  requestedAnimation = 7; // Animação ^^
  server.send(200, "text/plain", "OK");
  lastInteractionTime = millis();
}

void handleWakeup() { estaDormindo = false; requestedAnimation = 1; server.send(200, "text/plain", "OK"); }
void handleBlink() { requestedAnimation = 2; server.send(200, "text/plain", "OK"); }
void handleHappy() {
  felicidade = 100;
  requestedAnimation = 3;
  server.send(200, "text/plain", "OK");
  lastInteractionTime = millis();
}
void handleSleep() {
  energia = 100;
  requestedAnimation = 4;
  server.send(200, "text/plain", "OK");
}
void handleMoveLeft() {
  felicidade = min(100, felicidade + 15);
  requestedAnimation = 5;
  server.send(200, "text/plain", "OK");
  lastInteractionTime = millis();
}
void handleMoveRight() {
  felicidade = min(100, felicidade + 15);
  requestedAnimation = 6;
  server.send(200, "text/plain", "OK");
  lastInteractionTime = millis();
}
void handleNotFound() { server.send(404, "text/plain", "404: Nao encontrado"); }

void setup() {
  Serial.begin(115200);
  pinMode(pinoTouch, INPUT);
  pinMode(pinoBuzzer, OUTPUT);
  Wire.begin(4, 5);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Falha ao iniciar display SSD1306"));
    for (;;);
  }
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE);
  display.setCursor(0, 0); display.println("Conectando ao WiFi..."); display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado!"); Serial.print("IP: "); Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.update();

  display.clearDisplay(); display.setCursor(0, 0);
  display.println("Conectado!"); display.println("IP:"); display.println(WiFi.localIP());
  display.display();
  delay(2000);

  // Configura as rotas do servidor
  server.on("/", handleRoot);
  server.on("/stats", handleStats);
  server.on("/jogar", handleJogar);       // <-- RESTAURADO (Fase 2)
  server.on("/alimentar", handleAlimentar); // <-- RESTAURADO (Fase 1)
  server.on("/wakeup", handleWakeup);
  server.on("/blink", handleBlink);
  server.on("/happy", handleHappy);
  server.on("/sleep", handleSleep);
  server.on("/left", handleMoveLeft);
  server.on("/right", handleMoveRight);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor HTTP iniciado");
  
  lastInteractionTime = millis(); // <-- RESTAURADO (Fase 3)
  center_eyes(true);
  // <-- SOM: Toca um som de "boot"
  tone(pinoBuzzer, 523, 100); delay(100);
  tone(pinoBuzzer, 784, 150); delay(150);
  noTone(pinoBuzzer);
}

// =========================================================================
// FUNÇÕES DO MINI-JOGO
// =========================================================================
void game_reset() {
  tone(pinoBuzzer, 262, 50); delay(50);
  tone(pinoBuzzer, 523, 100);
  score = 0;
  game_speed = 3;
  player_y = GROUND_Y - PLAYER_SIZE;
  player_vel_y = 0;
  player_is_jumping = false;
  obstacle_x = SCREEN_WIDTH;
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(35, 30);
  display.print("INICIO!");
  display.display();
  delay(1000);
  noTone(pinoBuzzer);
}

void game_over() {
  tone(pinoBuzzer, 330, 500); delay(500);
  tone(pinoBuzzer, 262, 700);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 10);
  display.print("GAME OVER");
  display.setTextSize(1);
  display.setCursor(20, 35);
  display.print("Score: " + String(score));
  if (score > high_score) {
    high_score = score;
    display.setCursor(20, 50);
    display.print("NOVO RECORDE!");
  }
  display.display();
  delay(3000);
  currentMode = MODE_TAMAGOTCHI; // Volta ao modo normal
  lastInteractionTime = millis(); // Voltar do jogo conta como interação
  noTone(pinoBuzzer);
}

void game_loop() {
  // 1. LER INPUT (Sensor de Toque)
  int touchState = digitalRead(pinoTouch);
  if (touchState == HIGH && !player_is_jumping) {
    player_vel_y = JUMP_FORCE;
    player_is_jumping = true;

    tone(pinoBuzzer, 1046, 30);
  }

  // 2. ATUALIZAR FÍSICA E LÓGICA
  player_y -= player_vel_y;
  player_vel_y += GRAVITY;
  if (player_y > GROUND_Y - PLAYER_SIZE) {
    player_y = GROUND_Y - PLAYER_SIZE;
    player_vel_y = 0;
    player_is_jumping = false;
  }
  obstacle_x -= game_speed;
  if (obstacle_x < 0 - obstacle_width) {
    obstacle_x = SCREEN_WIDTH;
    score++;
    if (score % 5 == 0) {
      game_speed++;
    }
  }

  // 3. VERIFICAR COLISÃO
  bool x_overlap = (PLAYER_X + PLAYER_SIZE > obstacle_x) && (PLAYER_X < obstacle_x + obstacle_width);
  bool y_overlap = (player_y + PLAYER_SIZE > GROUND_Y - obstacle_height);
  if (x_overlap && y_overlap) {
    game_over();
    return;
  }

  // 4. DESENHAR TUDO
  display.clearDisplay();
  display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);
  display.fillRect(PLAYER_X, player_y, PLAYER_SIZE, PLAYER_SIZE, SSD1306_WHITE);
  display.fillRect(obstacle_x, GROUND_Y - obstacle_height, obstacle_width, obstacle_height, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: " + String(score));
  display.setCursor(70, 0);
  display.print("HI: " + String(high_score));
  display.display();
  delay(10);
}

// =========================================================================
// LOOP DO TAMAGOTCHI
// =========================================================================
void tamagotchi_loop() {
  unsigned long currentMillis = millis();
  timeClient.update();
  timeString = timeClient.getFormattedTime();
  int currentHour = timeClient.getHours();

  // --- Lógica de Decadência de Status ---
  if (currentMillis - previousMillisStats >= intervalStats) {
    previousMillisStats = currentMillis;
    if (!estaDormindo) {
      if (energia > 0) energia--;
      if (felicidade > 0) felicidade -= 2;
      if (fome > 0) fome--; // <-- RESTAURADO (Fase 1)
    }
  }

  // --- Lógica do Sensor de Toque (Carinho) ---
  int touchState = digitalRead(pinoTouch);
  if (touchState == HIGH && (currentMillis - lastTouchTime > touchCooldown)) {
    lastTouchTime = currentMillis;
    Serial.println("Toque detectado!");
    lastInteractionTime = currentMillis; // <-- RESTAURADO (Fase 3)
    
    if (estaDormindo) {
      // Se for de noite (entre 22h e 8h), ele recusa-se a acordar!
      if (currentHour >= 22 || currentHour < 8) {
      } else {
        requestedAnimation = 1; // Acorda se for dia
      }
    } else {
      felicidade = min(100, felicidade + 25);
      requestedAnimation = 7; // Expressão ^^ (RESTAURADO)
    }
  }

  // --- Lógica de Animações Solicitadas (Web) ---
  if (requestedAnimation != 0) {
    switch (requestedAnimation) {
      case 1: wakeup(); break;
      case 2: blink(); break;
      case 3: happy_eye(); break;
      case 4: sleep_anim(); break;
      case 5: move_left_big_eye(); break;
      case 6: move_right_big_eye(); break;
      case 7: draw_happy_eyes_replacement(); break; // <-- RESTAURADO
    }
    requestedAnimation = 0;
  }
  
  // ===========================================================
  // --- Lógica de Comportamento Autônomo (O MOTOR de Emoções) ---
  // ===========================================================
  else if (!estaDormindo) {
    if (currentHour >= 22 || currentHour < 8) {
      sleep_anim(); // Vai dormir
      currentEmotionString = "dormindo";
      delay(1000);
      return; // Sai da função para não executar o resto
    }
    
    // --- ESTADO CRÍTICO (Prioridade Máxima) ---
    if (fome < 10 && energia < 10 && felicidade < 10) {
      // 1. "DOENTE" (Tudo baixo)
      currentEmotionString = "doente";
      draw_sick_face(); // (X X)
      delay(2000);
    
    // --- ESTADOS DE NECESSIDADE (Prioridade Média) ---
    } else if (fome < 30) {
      // 2. "FAMINTO"
      currentEmotionString = "faminto";
      sad_eye();
      delay(1000);
    
    } else if (energia < 20) {
      // 3. "SONOLENTO" (Lógica Combinatória)
      if (felicidade > 80) {
        // 3a. Sonolento mas FELIZ
        currentEmotionString = "sonolento_feliz";
        draw_sleepy_happy_face(); // (olhos fechados + ^^)
      } else {
        // 3b. Sonolento normal
        center_eyes(false);
        left_eye_height = 5; right_eye_height = 5;
        currentEmotionString = "sonolento";
        draw_eyes(true);
      }
      delay(1000);

    // --- ESTADOS EMOCIONAIS (Prioridade Baixa) ---
    } else if (felicidade < 30) {
      // 4. "INFELIZ"
      currentEmotionString = "triste";
      sad_eye();
      delay(1000);
    
    } else if (currentMillis - lastInteractionTime > boredomInterval) {
      // 5. "ENTEDIADO"
      currentEmotionString = "entediado";
      draw_bored_animation(); // (olha pros lados)
      lastInteractionTime = currentMillis; // Reseta o timer
    
    } else {
      // 6. "CONTENTE" (Default)
      center_eyes(true);
      
      // Sorteia um número de 0 a 1000
      long sorteio = random(1000);

      if (sorteio < 20) { 
        // 2% de chance de Piscar (Aprox. a cada 5-10 seg)
        blink();
        delay(2000);
      } 
      else if (sorteio > 995) { 
        // 0.5% de chance de Assobiar (Aprox. a cada 1-2 min)
        play_whistle();
        // Opcional: Faz uma cara feliz rápida enquanto assobia
        draw_happy_eyes_replacement();
      }
    }
  } else {
    // Se estiver dormindo, amanheceu (ex: 08:00) e ele estava a dormir, ACORDA!
    if (currentHour >= 8 && currentHour < 22 && estaDormindo) {
        // Acorda automaticamente de manhã!
        wakeup(); 
        return;
    }
    center_eyes(false); 
    left_eye_height = 2; 
    right_eye_height = 2; 
    draw_eyes(true);
    currentEmotionString = "dormindo";

    if (currentHour >= 22 || currentHour < 8) {
       play_snore();
       delay(2000);  // Pausa entre roncos
    } else {
       delay(1000);
    }
  }
}
void loop() {
  server.handleClient();

  // Verifica em qual modo estamos e chama a função de loop correta
  switch (currentMode) {
    case MODE_TAMAGOTCHI:
      tamagotchi_loop();
      break;
    case MODE_GAME:
      game_loop();
      break;
  }
}
