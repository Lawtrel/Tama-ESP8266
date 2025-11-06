#include <ESP8266WiFi.h>     //
#include <ESP8266WebServer.h>  //
#include <Wire.h>            //
#include <Adafruit_GFX.h>      //
#include <Adafruit_SSD1306.h>  //

// --- Configurações do Wi-Fi (Verifique se está correto) ---
const char* ssid = "S24 de Lawtrel";  //
const char* password = "qwertyuiop";  //

// --- Configurações do Display ---
#define SCREEN_WIDTH 128     //
#define SCREEN_HEIGHT 64     //
#define OLED_RESET -1        //
#define SCREEN_ADDRESS 0x3C  //

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  //
ESP8266WebServer server(80);  //

// --- Pinos ---
const int pinoTouch = 14;  // Pino D5 (GPIO14) para o TTP223

// =========================================================================
// LÓGICA DO TAMAGOTCHI (RESTAURADO)
// =========================================================================
int energia = 100;
int felicidade = 100;
bool estaDormindo = false;

// --- Controle de Tempo ---
unsigned long previousMillisStats = 0;
const long intervalStats = 30000;  // A cada 30 segundos, as necessidades diminuem

// --- Controlo de Cooldown do Touch ---
unsigned long lastTouchTime = 0;
const long touchCooldown = 2000;  // Espera 2 segundos entre "carinhos"

// --- Constantes dos Olhos e UI ---
#define UI_HEIGHT 10  // Define a altura da área da UI no topo
const int REF_EYE_HEIGHT = 40;  //
const int REF_EYE_WIDTH = 40;   //
const int REF_SPACE_BETWEEN_EYE = 10;  //
const int REF_CORNER_RADIUS = 10;  //
int left_eye_height = REF_EYE_HEIGHT;  //
int left_eye_width = REF_EYE_WIDTH;  //
int left_eye_x = 0, left_eye_y = 0;  //
int right_eye_height = REF_EYE_HEIGHT;  //
int right_eye_width = REF_EYE_WIDTH;  //
int right_eye_x = 0, right_eye_y = 0;  //
volatile int requestedAnimation = 0;  //
// 0=Nenhum, 1=Wakeup, 2=Blink, 3=Happy, 4=Sleep, 5=Left, 6=Right

// =========================================================================
// FUNÇÃO PARA DESENHAR A INTERFACE (BARRAS DE STATUS) (RESTAURADO)
// =========================================================================
void drawInterface() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("E");
  display.drawRect(10, 0, 52, 7, SSD1306_WHITE);  // Borda da barra
  int energiaWidth = map(energia, 0, 100, 0, 50);  // Mapeia o valor 0-100 para a largura 0-50
  display.fillRect(11, 1, energiaWidth, 5, SSD1306_WHITE);  // Preenchimento

  display.setCursor(68, 0);
  display.print("F");
  display.drawRect(78, 0, 50, 7, SSD1306_WHITE);  // Borda da barra
  int felicidadeWidth = map(felicidade, 0, 100, 0, 48);
  display.fillRect(79, 1, felicidadeWidth, 5, SSD1306_WHITE);
}

// =========================================================================
// FUNÇÕES DE ANIMAÇÃO (do seu ficheiro, com modificações)
// =========================================================================

void draw_eyes(bool update = true) {  //
  display.clearDisplay();
  drawInterface();  // <-- Adiciona as barras de status

  int x1 = left_eye_x - left_eye_width / 2;  //
  int y1 = left_eye_y - left_eye_height / 2;  //
  display.fillRoundRect(x1, y1, left_eye_width, left_eye_height, REF_CORNER_RADIUS, SSD1306_WHITE);  //
  int x2 = right_eye_x - right_eye_width / 2;  //
  int y2 = right_eye_y - right_eye_height / 2;  //
  display.fillRoundRect(x2, y2, right_eye_width, right_eye_height, REF_CORNER_RADIUS, SSD1306_WHITE);  //
  if (update) {  //
    display.display();
  }
}

void center_eyes(bool update = true) {  //
  left_eye_width = REF_EYE_WIDTH;  //
  left_eye_height = REF_EYE_HEIGHT;  //
  right_eye_width = REF_EYE_WIDTH;  //
  right_eye_height = REF_EYE_HEIGHT;  //

  // <-- Ajusta a posição Y para centralizar abaixo da UI
  int centerY = (SCREEN_HEIGHT - UI_HEIGHT) / 2 + UI_HEIGHT;

  left_eye_x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;  //
  left_eye_y = centerY;  // <-- Modificado (original era SCREEN_HEIGHT / 2)
  right_eye_x = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;  //
  right_eye_y = centerY;  // <-- Modificado (original era SCREEN_HEIGHT / 2)

  if (update) {  //
    draw_eyes(true);
  }
}

void blink() {  //
  center_eyes(false);
  draw_eyes();
  for (int h = REF_EYE_HEIGHT; h >= 0; h -= 10) {  //
    left_eye_height = h;  //
    right_eye_height = h;  //
    draw_eyes();
  }
  delay(100);
  for (int h = 0; h <= REF_EYE_HEIGHT; h += 10) {  //
    left_eye_height = h;  //
    right_eye_height = h;  //
    draw_eyes();
  }
  center_eyes(true);
}

void wakeup() {  //
  estaDormindo = false;  // <-- Lógica do Tamagotchi
  center_eyes(false);  //
  for (int h = 0; h <= REF_EYE_HEIGHT; h += 2) {  //
    left_eye_height = h;  //
    right_eye_height = h;  //
    draw_eyes(true);
  }
}

void sleep_anim() {  //
  estaDormindo = true;  // <-- Lógica do Tamagotchi
  center_eyes(false);
  for (int h = REF_EYE_HEIGHT; h >= 2; h -= 2) {  //
    left_eye_height = h;  //
    right_eye_height = h;  //
    draw_eyes(true);
  }
}

void happy_eye() {  //
  center_eyes(false);
  draw_eyes(false);
  int offset = REF_EYE_HEIGHT / 2;  //
  for (int i = 0; i < 10; i++) {  //
    display.fillTriangle(left_eye_x - left_eye_width / 2 - 1, left_eye_y + offset, left_eye_x + left_eye_width / 2 + 1, left_eye_y + 5 + offset, left_eye_x - left_eye_width / 2 - 1, left_eye_y + left_eye_height + offset, SSD1306_BLACK);  //
    display.fillTriangle(right_eye_x + right_eye_width / 2 + 1, right_eye_y + offset, right_eye_x - left_eye_width / 2 - 1, right_eye_y + 5 + offset, right_eye_x + right_eye_width / 2 + 1, right_eye_y + right_eye_height + offset, SSD1306_BLACK);  //
    offset -= 2;  //
    display.display();
    delay(10);  //
  }
  delay(1000);  //
  center_eyes(true);
}

void move_big_eye(int direction) {  //
  // ... (código da função 'move_big_eye' copiado do seu ficheiro) ...
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

void move_right_big_eye() {  //
  move_big_eye(1);
}
void move_left_big_eye() {  //
  move_big_eye(-1);
}

// --- Nova animação para expressar tristeza (RESTAURADO) ---
void sad_eye() {
  center_eyes(false);
  draw_eyes(false);

  // Desenha uma "sobrancelha" triste sobre os olhos
  // (Esta é uma versão simplificada da animação 'happy_eye', invertida)
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
  delay(1500);  // Mostra a expressão por um tempo
  center_eyes(true);
}

void draw_happy_mouth() {
// 1. Limpa a tela TODA, mas redesenha a UI (barras de status)
  display.clearDisplay();
  drawInterface(); // Garante que as barras de status continuem visíveis

  // 2. Vamos calcular onde os ^^ devem ficar.
  // Usamos as mesmas posições centrais dos olhos originais.
  int centerY = (SCREEN_HEIGHT - UI_HEIGHT) / 2 + UI_HEIGHT;
  int centerX_Left = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  int centerX_Right = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;

  // 3. Definir o tamanho do ^
  int eyeSize = 10; // O "tamanho" do ^
  int eyeTopY = centerY - (eyeSize / 2);
  int eyeBottomY = centerY + (eyeSize / 2);

  // 4. Desenhar o ^ esquerdo (substituindo o olho esquerdo)
  // Linha 1 (/)
  display.drawLine(centerX_Left - eyeSize, eyeBottomY, centerX_Left, eyeTopY, SSD1306_WHITE);
  // Linha 2 (\)
  display.drawLine(centerX_Left, eyeTopY, centerX_Left + eyeSize, eyeBottomY, SSD1306_WHITE);

  // 5. Desenhar o ^ direito (substituindo o olho direito)
  // Linha 1 (/)
  display.drawLine(centerX_Right - eyeSize, eyeBottomY, centerX_Right, eyeTopY, SSD1306_WHITE);
  // Linha 2 (\)
  display.drawLine(centerX_Right, eyeTopY, centerX_Right + eyeSize, eyeBottomY, SSD1306_WHITE);

  // 6. Envia o desenho (UI + ^^) para a tela
  display.display();
  delay(1500); // Mostra a expressão por 1.5 segundos

  // 7. Restaura os olhos normais
  center_eyes(true);
}


// =========================================================================
// FUNÇÕES PARA O SERVIDOR WEB (Callbacks) (MODIFICADO)
// =========================================================================

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle Tamagotchi</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #2c3e50; color: white; display: flex; flex-direction: column; align-items: center; padding: 10px; margin: 0; }
    h1 { margin-bottom: 10px; }
    .stats { background-color: #34495e; padding: 10px; border-radius: 8px; margin-bottom: 20px; width: 90%; max-width: 400px; text-align: center; font-size: 1.1em; }
    .button-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; max-width: 400px; width: 90%; }
    button { background-color: #3498db; color: white; border: none; border-radius: 8px; padding: 20px; font-size: 1.2em; cursor: pointer; transition: background-color 0.3s; }
    button:hover { background-color: #2980b9; }
    p { grid-column: 1 / -1; text-align: center; margin: 10px 0 0 0; font-size: 1.1em; }
  </style>
</head>
<body>
  <h1>Controle Tamagotchi</h1>
  
  <div class="stats">
    Energia: <span id="energia">--</span>% | Felicidade: <span id="felicidade">--</span>%
  </div>
  
  <div class="button-grid">
    <button data-action="wakeup">Acordar</button>
    <button data-action="sleep">Dormir</button>
    <button data-action="blink">Piscar</button>
    <button data-action="happy">Brincar (Feliz)</button>
    <p>--- Movimento ---</p>
    <button data-action="left">Olhar Esquerda</button>
    <button data-action="right">Olhar Direita</button>
  </div>

  <script>
    document.querySelectorAll('button').forEach(button => {
      button.addEventListener('click', () => {
        const action = button.dataset.action;
        fetch('/' + action)
          .then(response => {
            if (!response.ok) { console.error('Falha ao enviar comando ' + action); }
            getStats(); // Atualiza os status logo após o clique
          });
      });
    });
    
    // Função para buscar e atualizar os status
    function getStats(){
      fetch('/stats')
        .then(response => response.json())
        .then(data => {
          document.getElementById('energia').innerText = data.energia;
          document.getElementById('felicidade').innerText = data.felicidade;
        });
    }
    // Atualiza os status a cada 2 segundos e também ao carregar a página
    setInterval(getStats, 2000);
    document.addEventListener('DOMContentLoaded', getStats);
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// --- Nova rota para a interface web ler os status (RESTAURADO) ---
void handleStats() {
  String json = "{\"energia\":" + String(energia) + ", \"felicidade\":" + String(felicidade) + "}";
  server.send(200, "application/json", json);
}

// --- Handlers modificados para o Tamagotchi ---
void handleWakeup() {  //
  estaDormindo = false;
  requestedAnimation = 1;
  server.send(200, "text/plain", "OK");
}

void handleBlink() {  //
  requestedAnimation = 2;
  server.send(200, "text/plain", "OK");
}

void handleHappy() {  //
  felicidade = 100;  // <-- Restaura a felicidade
  requestedAnimation = 3;
  server.send(200, "text/plain", "OK");
}

void handleSleep() {  //
  energia = 100;  // <-- Restaura a energia
  requestedAnimation = 4;
  server.send(200, "text/plain", "OK");
}

void handleMoveLeft() {  //
  felicidade = min(100, felicidade + 15);  // <-- Interagir aumenta a felicidade
  requestedAnimation = 5;
  server.send(200, "text/plain", "OK");
}

void handleMoveRight() {  //
  felicidade = min(100, felicidade + 15);  // <-- Interagir aumenta a felicidade
  requestedAnimation = 6;
  server.send(200, "text/plain", "OK");
}

void handleNotFound() {  //
  server.send(404, "text/plain", "404: Nao encontrado");
}

// =========================================================================
// SETUP: Executado uma vez (MODIFICADO)
// =========================================================================
void setup() {
  Serial.begin(115200);  //
  pinMode(pinoTouch, INPUT);  // <-- Define o pino de toque como entrada

  // Inicia o I2C nos pinos corretos (4=SDA, 5=SCL)
  Wire.begin(4, 5);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {  //
    Serial.println(F("Falha ao iniciar display SSD1306"));  //
    for (;;);  //
  }
  display.clearDisplay();  //
  display.setTextSize(1);  //
  display.setTextColor(WHITE);  //
  display.setCursor(0, 0);  //
  display.println("Conectando ao WiFi...");  //
  display.display();  //

  WiFi.begin(ssid, password);  //
  Serial.print("Conectando a ");  //
  Serial.println(ssid);  //
  while (WiFi.status() != WL_CONNECTED) {  //
    delay(500);  //
    Serial.print(".");  //
  }
  Serial.println("\nWiFi conectado!");  //
  Serial.print("Endereco de IP: ");  //
  Serial.println(WiFi.localIP());  //

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Conectado!");  //
  display.println("IP:");  //
  display.println(WiFi.localIP());  //
  display.display();
  delay(2000);  // Pausa para ver o IP

  // Configura as rotas do servidor
  server.on("/", handleRoot);  //
  server.on("/stats", handleStats);  // <-- Rota para status
  server.on("/wakeup", handleWakeup);  //
  server.on("/blink", handleBlink);  //
  server.on("/happy", handleHappy);  //
  server.on("/sleep", handleSleep);  //
  server.on("/left", handleMoveLeft);  //
  server.on("/right", handleMoveRight);  //
  server.onNotFound(handleNotFound);  //

  server.begin();  //
  Serial.println("Servidor HTTP iniciado");

  center_eyes(true);  //
}


// =========================================================================
// LOOP: Executado continuamente (MODIFICADO)
// =========================================================================
void loop() {
  server.handleClient();  //

  unsigned long currentMillis = millis();

  // --- Lógica de Decadência de Status ---
  if (currentMillis - previousMillisStats >= intervalStats) {
    previousMillisStats = currentMillis;
    if (!estaDormindo) {
      if (energia > 0) energia--;
      if (felicidade > 0) felicidade -= 2;  // Felicidade cai mais rápido
    }
  }

  // --- Lógica do Sensor de Toque ---
  int touchState = digitalRead(pinoTouch);
  if (touchState == HIGH && (currentMillis - lastTouchTime > touchCooldown)) {
    lastTouchTime = currentMillis;  // Reseta o timer do cooldown
    Serial.println("Toque (carinho) detectado!");

    if (estaDormindo) {
      requestedAnimation = 1;  // Acorda
    } else {
      felicidade = min(100, felicidade + 25);  // Aumenta 25
      requestedAnimation = 7;  // Animação "feliz"
    }
  }

  // --- Lógica de Animações Solicitadas ---
  if (requestedAnimation != 0) {  //
    switch (requestedAnimation) {  //
      case 1: wakeup(); break;  //
      case 2: blink(); break;  //
      case 3: happy_eye(); break;  //
      case 4: sleep_anim(); break;  //
      case 5: move_left_big_eye(); break;  //
      case 6: move_right_big_eye(); break;  //
      case 7: draw_happy_mouth(); break;
    }
    requestedAnimation = 0;  //
  }
  // --- Lógica de Comportamento Autônomo ---
  else if (!estaDormindo) {
    if (energia < 20) {
      // Se estiver com pouca energia, fica com os olhos quase fechados
      center_eyes(false);
      left_eye_height = 5; right_eye_height = 5;
      draw_eyes(true);
      delay(100);
    } else if (felicidade < 30) {
      // Se estiver infeliz, faz uma expressão triste e depois volta ao normal
      sad_eye();
    } else {
      // Comportamento padrão: olhos normais e pisca aleatoriamente
      center_eyes(true);
      if (random(100) < 2) {  // 2% de chance a cada loop de piscar
        blink();
        delay(2000);  // Pausa depois de piscar
      }
    }
  } else {
    // Se estiver dormindo, apenas redesenha os olhos fechados com a UI
    center_eyes(false);  // Atualiza coordenadas
    left_eye_height = 2; right_eye_height = 2;  // Mantém olhos fechados
    draw_eyes(true);
    delay(1000);
  }
}
