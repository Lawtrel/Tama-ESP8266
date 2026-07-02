#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// --- Configurações do Display ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pinos (Arduino Uno) ---
const int pinoBotaoNavegar = 2; // Botão 1: Navegar no Menu / Saltar
const int pinoBotaoSelect = 3;  // Botão 2: Selecionar Opção
const int pinoBuzzer = 4;       // Buzzer
const int pinoTouch = 5;        // Touch: Voltar / Carinho / Saltar

// --- Estrutura de Dados ---
struct TamagotchiState {
  int energia; int felicidade; int fome; int high_score; byte magicByte; 
};

// =========================================================================
// VARIÁVEIS DE ESTADO
// =========================================================================
#define MODE_TAMAGOTCHI 0
#define MODE_GAME       1
#define MODE_MENU       2
int currentMode = MODE_TAMAGOTCHI;

// --- Estado do Menu ---
int menuOpcao = 0;
const int totalOpcoes = 4; // Comer, Jogar, Dormir, Curar

// --- Estado do Tamagotchi ---
int energia = 100;
int felicidade = 100;
int fome = 100;
bool estaDormindo = false;

unsigned long previousMillisStats = 0;
const long intervalStats = 30000;
unsigned long lastInteractionTime = 0;
const long boredomInterval = 60000; 
unsigned long previousMillisSave = 0;
const long saveInterval = 300000;

// Input
unsigned long lastInputTime = 0;
const long inputCooldown = 250; 
unsigned long lastTouchTime = 0;
const long touchCooldown = 2000;

volatile int requestedAnimation = 0;
String currentEmotionString = "contente";

// Relógio Interno
unsigned long previousMillisClock = 0;
int gameHour = 8;
int gameMinute = 0;

// --- Jogo ---
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

// --- Constantes UI ---
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
// FUNÇÕES DE MEMÓRIA
// =========================================================================
void saveData() {
  TamagotchiState state = { energia, felicidade, fome, high_score, 0xAD };
  EEPROM.put(0, state);
}

void loadData() {
  TamagotchiState state;
  EEPROM.get(0, state);
  if (state.magicByte == 0xAD) {
    energia = state.energia; felicidade = state.felicidade; fome = state.fome; high_score = state.high_score;
  } else {
    energia = 100; felicidade = 100; fome = 100; high_score = 0;
    saveData();
  }
}

// =========================================================================
// FUNÇÕES DE SOM E UI
// =========================================================================
void play_tone(int freq, int duration) {
  tone(pinoBuzzer, freq, duration);
  delay(duration);
  noTone(pinoBuzzer);
}

void drawInterface() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int barWidth = 32; int barHeight = 7;
  
  display.setCursor(0, 0); display.print("E");
  display.drawRect(9, 0, barWidth + 2, barHeight, SSD1306_WHITE);
  display.fillRect(10, 1, map(energia, 0, 100, 0, barWidth), barHeight - 2, SSD1306_WHITE);

  display.setCursor(44, 0); display.print("H"); 
  display.drawRect(53, 0, barWidth + 2, barHeight, SSD1306_WHITE);
  display.fillRect(54, 1, map(felicidade, 0, 100, 0, barWidth), barHeight - 2, SSD1306_WHITE);

  display.setCursor(88, 0); display.print("F");
  display.drawRect(97, 0, barWidth + 2, barHeight, SSD1306_WHITE);
  display.fillRect(98, 1, map(fome, 0, 100, 0, barWidth), barHeight - 2, SSD1306_WHITE);
}

// =========================================================================
// DESENHO DO MENU
// =========================================================================
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Título
  display.setCursor(40, 0);
  display.print("MENU");
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  // Ícones (Simplificados)
  for(int i=0; i<totalOpcoes; i++) {
    int x = 15 + (i * 30);
    int y = 25;

    if(i == 0) { // Comer
      display.drawRect(x, y, 20, 20, SSD1306_WHITE);
      display.drawLine(x+10, y+2, x+10, y+18, SSD1306_WHITE);
      display.drawLine(x+2, y+10, x+18, y+10, SSD1306_WHITE);
    } 
    else if (i == 1) { // Jogar
      display.drawCircle(x+10, y+10, 9, SSD1306_WHITE);
    }
    else if (i == 2) { // Dormir
      display.setCursor(x+6, y+6);
      display.setTextSize(2);
      display.print("Z");
      display.setTextSize(1);
    }
    else if (i == 3) { // Curar
      display.fillRect(x+8, y+2, 4, 16, SSD1306_WHITE);
      display.fillRect(x+2, y+8, 16, 4, SSD1306_WHITE);
    }
  }

  // Seletor
  int selX = 11 + (menuOpcao * 30);
  display.drawRect(selX, 21, 28, 28, SSD1306_WHITE);

  // Legenda
  display.setCursor(0, 56);
  String legenda = "";
  if (menuOpcao == 0) legenda = "COMER";
  if (menuOpcao == 1) legenda = "JOGAR";
  if (menuOpcao == 2) legenda = "DORMIR";
  if (menuOpcao == 3) legenda = "CURAR";
  
  int centerText = (SCREEN_WIDTH - (legenda.length() * 6)) / 2;
  display.setCursor(centerText, 56);
  display.print(legenda);

  display.display();
}

// =========================================================================
// ANIMAÇÕES
// =========================================================================
void draw_eyes(bool update = true) {
  display.clearDisplay(); drawInterface();
  int x1 = left_eye_x - left_eye_width / 2; int y1 = left_eye_y - left_eye_height / 2;
  display.fillRoundRect(x1, y1, left_eye_width, left_eye_height, REF_CORNER_RADIUS, SSD1306_WHITE);
  int x2 = right_eye_x - right_eye_width / 2; int y2 = right_eye_y - right_eye_height / 2;
  display.fillRoundRect(x2, y2, right_eye_width, right_eye_height, REF_CORNER_RADIUS, SSD1306_WHITE);
  if (update) { display.display(); }
}

void center_eyes(bool update = true) {
  left_eye_width = REF_EYE_WIDTH; left_eye_height = REF_EYE_HEIGHT;
  right_eye_width = REF_EYE_WIDTH; right_eye_height = REF_EYE_HEIGHT;
  int centerY = (SCREEN_HEIGHT - 10) / 2;
  left_eye_x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  left_eye_y = centerY;
  right_eye_x = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
  right_eye_y = centerY;
  if (update) { draw_eyes(true); }
}

void draw_happy_eyes_replacement() {
  tone(pinoBuzzer, 1000, 100); display.clearDisplay(); drawInterface();
  int cy = (SCREEN_HEIGHT - 10) / 2;
  display.setTextSize(3); display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, cy - 10); display.print("^   ^");
  display.display(); delay(1500); center_eyes(true); noTone(pinoBuzzer);
}

void sad_eye() {
  tone(pinoBuzzer, 200, 500); center_eyes(false); draw_eyes(false);
  display.fillCircle(left_eye_x, left_eye_y + 25, 3, SSD1306_WHITE);
  display.fillCircle(right_eye_x, right_eye_y + 25, 3, SSD1306_WHITE);
  display.display(); delay(1500); center_eyes(true); noTone(pinoBuzzer);
}

void draw_sick_face() {
  tone(pinoBuzzer, 100, 500); display.clearDisplay(); drawInterface();
  int cy = (SCREEN_HEIGHT - 10) / 2;
  display.setTextSize(3); display.setCursor(20, cy - 10); display.print("X   X");
  display.display(); noTone(pinoBuzzer);
}

void blink() {
  center_eyes(false); draw_eyes();
  for (int h = REF_EYE_HEIGHT; h >= 2; h -= 8) { left_eye_height = h; right_eye_height = h; draw_eyes(); }
  delay(50);
  for (int h = 2; h <= REF_EYE_HEIGHT; h += 8) { left_eye_height = h; right_eye_height = h; draw_eyes(); }
  center_eyes(true);
}

// =========================================================================
// AÇÕES DO JOGO E MENU
// =========================================================================
void game_reset() {
  play_tone(600, 100); score = 0; game_speed = 3; player_y = GROUND_Y - PLAYER_SIZE;
  player_vel_y = 0; player_is_jumping = false; obstacle_x = SCREEN_WIDTH;
}

void game_over() {
  play_tone(200, 500);
  display.clearDisplay();
  display.setTextSize(2); display.setCursor(15, 10); display.print("GAME OVER");
  display.setTextSize(1); display.setCursor(30, 35); display.print("Score: " + String(score));
  if (score > high_score) { high_score = score; saveData(); display.setCursor(30, 45); display.print("NEW HIGH!"); }
  display.display(); delay(2000);
  currentMode = MODE_TAMAGOTCHI;
}

void game_loop() {
  if ((digitalRead(pinoBotaoNavegar) == LOW || digitalRead(pinoTouch) == HIGH) && !player_is_jumping) {
    player_vel_y = JUMP_FORCE; player_is_jumping = true; tone(pinoBuzzer, 800, 50);
  }
  player_y -= player_vel_y; player_vel_y += GRAVITY;
  if (player_y > GROUND_Y - PLAYER_SIZE) { player_y = GROUND_Y - PLAYER_SIZE; player_vel_y = 0; player_is_jumping = false; }
  
  obstacle_x -= game_speed;
  if (obstacle_x < 0) { obstacle_x = SCREEN_WIDTH; score++; if(score%5==0) game_speed++; }
  
  if (PLAYER_X + PLAYER_SIZE > obstacle_x && PLAYER_X < obstacle_x + 5 && player_y + PLAYER_SIZE > GROUND_Y - 10) {
    game_over(); return;
  }

  display.clearDisplay();
  display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);
  display.fillRect(PLAYER_X, player_y, PLAYER_SIZE, PLAYER_SIZE, SSD1306_WHITE);
  display.fillRect(obstacle_x, GROUND_Y - 10, 5, 10, SSD1306_WHITE);
  display.setCursor(0,0); display.print(score);
  display.display();
  delay(20);
}

void handleCurar() {
  if (energia < 50 || fome < 50 || felicidade < 50) {
    energia = 100; fome = 100; felicidade = 100;
    draw_happy_eyes_replacement();
    play_tone(1000, 100); play_tone(1500, 200);
  } else {
    play_tone(200, 100);
  }
}

void handleAlimentar() {
  fome = 100; draw_happy_eyes_replacement();
}

void handleDormir() {
  estaDormindo = !estaDormindo;
}

// =========================================================================
// LOOP DO MENU
// =========================================================================
void menu_loop() {
  drawMenu();
  unsigned long currentMillis = millis();
  
  if (digitalRead(pinoBotaoNavegar) == LOW && (currentMillis - lastInputTime > inputCooldown)) {
    lastInputTime = currentMillis;
    menuOpcao = (menuOpcao + 1) % totalOpcoes;
    play_tone(440, 50);
  }

  if (digitalRead(pinoBotaoSelect) == LOW && (currentMillis - lastInputTime > inputCooldown)) {
    lastInputTime = currentMillis;
    play_tone(880, 100);
    switch(menuOpcao) {
      case 0: handleAlimentar(); currentMode = MODE_TAMAGOTCHI; break;
      case 1: game_reset(); currentMode = MODE_GAME; break;
      case 2: handleDormir(); currentMode = MODE_TAMAGOTCHI; break;
      case 3: handleCurar(); currentMode = MODE_TAMAGOTCHI; break;
    }
    lastInteractionTime = currentMillis;
  }

  if (digitalRead(pinoTouch) == HIGH && (currentMillis - lastInputTime > inputCooldown)) {
    lastInputTime = currentMillis;
    play_tone(330, 50);
    currentMode = MODE_TAMAGOTCHI;
  }
}

// =========================================================================
// SETUP (ESTA É A FUNÇÃO QUE FALTAVA)
// =========================================================================
void setup() {
  // Configuração dos pinos
  pinMode(pinoBotaoNavegar, INPUT_PULLUP);
  pinMode(pinoBotaoSelect, INPUT_PULLUP);
  pinMode(pinoTouch, INPUT); // Assumindo sensor de toque 5V normal
  pinMode(pinoBuzzer, OUTPUT);

  // Inicializa Display
  Wire.begin(); // Padrão Arduino Uno
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for (;;); // Trava se falhar
  }
  
  // Carrega dados da memória
  loadData();
  
  // Splash Screen
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20); display.print("TAMAGOTCHI UNO");
  display.display();
  play_tone(500, 100); play_tone(800, 200);
  delay(1000);
  
  lastInteractionTime = millis();
  center_eyes(true);
}

// =========================================================================
// LOOP PRINCIPAL
// =========================================================================
void loop() {
  unsigned long currentMillis = millis();

  // Relógio
  if (currentMillis - previousMillisClock >= 60000) {
    previousMillisClock = currentMillis;
    gameMinute++; if (gameMinute >= 60) { gameMinute = 0; gameHour++; if (gameHour >= 24) gameHour = 0; }
  }
  // Decadência
  if (currentMillis - previousMillisStats >= intervalStats) {
    previousMillisStats = currentMillis;
    if (!estaDormindo) {
      if (energia > 0) energia--; if (felicidade > 0) felicidade--; if (fome > 0) fome--;
    }
  }
  // Salvar
  if (currentMillis - previousMillisSave >= saveInterval) {
    previousMillisSave = currentMillis;
    if (!estaDormindo) { saveData(); }
  }

  switch (currentMode) {
    case MODE_TAMAGOTCHI:
      // Botão 1 (Nav): Abre o Menu
      if (digitalRead(pinoBotaoNavegar) == LOW && (currentMillis - lastInputTime > inputCooldown)) {
        lastInputTime = currentMillis;
        currentMode = MODE_MENU;
        play_tone(400, 50);
      }
      // Touch: Faz Carinho
      if (digitalRead(pinoTouch) == HIGH && (currentMillis - lastTouchTime > touchCooldown)) {
        lastTouchTime = currentMillis;
        felicidade = min(100, felicidade + 15);
        draw_happy_eyes_replacement();
      }
      
      // Lógica de Emoções
      if (!estaDormindo) {
        if (fome < 20 || felicidade < 20 || energia < 10) draw_sick_face();
        else if (fome < 40) sad_eye(); 
        else {
          center_eyes(true);
          if (random(1000) < 10) blink();
        }
      } else {
        display.clearDisplay(); display.setCursor(50, 30); display.print("zZz...");
        display.display(); delay(500);
      }
      break;

    case MODE_GAME: game_loop(); break;
    case MODE_MENU: menu_loop(); break;
  }
}