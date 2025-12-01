# Robomotion

Clone inspirado no Tamagotchi (Bandai) – um bichinho virtual autônomo com olhos animados em OLED, motor de emoções, mini‑jogo e controle via interface web + voz.

> "Robomotion" é um pet eletrônico feito para experimentar animação, estados emocionais e interação humano–dispositivo usando um ESP8266.
> Tamagotchi é marca registrada da Bandai; este projeto é apenas educacional e não oficial.

## 🎯 Objetivo
Criar um mascote virtual sempre ativo, que:
- Mostra emoções dinâmicas (contente, faminto, triste, doente, sonolento, sonolento_feliz, entediado, dormindo)
- Reage a horário real (NTP) – dorme à noite (22h–08h)
- Decai atributos com o tempo (energia, felicidade, fome) e exige cuidados
- Persiste progresso (EEPROM) incluindo recorde do mini‑jogo
- Permite jogar um runner simples (pular obstáculos) para aumentar engajamento
- Aceita comandos por botões físicos, HTTP e fala (SpeechRecognition/SpeechSynthesis no navegador)

## 🧩 Funcionalidades Principais
- Display OLED SSD1306 (128x64) – animação de olhos, piscadas, expressões especiais
- Motor de Emoções Autônomo (prioridades de estado, eventos aleatórios como piscar/assobiar/roncar)
- Mini‑jogo: corredor com salto, obstáculos, score e high score persistente
- Servidor Web embutido (ESP8266WebServer) com UI responsiva em português
- Comandos de voz (no navegador) – alimentar, brincar, dormir, acordar
- Animações acionáveis: piscar, mover olhos, feliz, dormir, acordar, etc.
- Sincronização de tempo via NTP (offset configurável)
- Persistência em EEPROM (magic byte 0xAD para validação)

## 🛠️ Hardware Necessário
| Componente | Observação |
|------------|-----------|
| ESP8266 (NodeMCU / Wemos D1 Mini) | Wi‑Fi integrado |
| OLED SSD1306 I2C 128x64 | Endereço padrão `0x3C` |
| Buzzer passivo | Notas/efeitos sonoros |
| 2 Botões tácteis | Alimentar/Saltar (D7), Jogar (D3) |
| Jumpers + Protoboard | Montagem |
| Fonte 5V / USB | Alimentação |

### Mapeamento de Pinos (código atual)
- Buzzer: D6 (GPIO12)
- Botão Alimentar / Saltar: D7 (GPIO13) – `pinoBotaoFeed`
- Botão Jogar: D3 (GPIO0) – `pinoBotaoGame`
- I2C: `Wire.begin(4,5)` → SDA=GPIO4 (D2), SCL=GPIO5 (D1)

## 📦 Bibliotecas Utilizadas
Instale via Gerenciador de Bibliotecas da Arduino IDE:
- `ESP8266WiFi`
- `ESP8266WebServer`
- `Wire`
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `WiFiUdp`
- `NTPClient`
- `EEPROM`

## ⚙️ Configuração Inicial
1. Instale Arduino IDE
2. Adicione placa ESP8266: Ferramentas > Placa > Gerenciador de Placas > "esp8266"
3. Selecione a placa correspondente (ex: NodeMCU 1.0)
4. Instale bibliotecas listadas acima
5. Abra `eyes4.ino`
6. Ajuste:
```cpp
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";
const long utcOffsetInSeconds = -10800; // Ajuste de fuso
```
7. Compile e faça upload
8. Abra o Serial Monitor (115200) para ver IP
9. Acesse `http://<IP_DO_ESP>/` no navegador

## 🌐 Endpoints HTTP
| Endpoint | Método | Função |
|----------|--------|--------|
| `/` | GET | Interface Web principal |
| `/stats` | GET | JSON de estado atual |
| `/alimentar` | GET | Zera fome e animação feliz |
| `/happy` | GET | Reabastece felicidade |
| `/sleep` | GET | Entra em modo dormir (energia recuperada) |
| `/wakeup` | GET | Acorda se dormindo |
| `/blink` | GET | Pisca os olhos |
| `/left` | GET | Animação movimento esquerdo |
| `/right` | GET | Animação movimento direito |
| `/jogar` | GET | Entra no mini‑jogo |

### Exemplo Resposta `/stats`
```json
{
  "energia": 87,
  "felicidade": 72,
  "fome": 54,
  "emocao": "contente",
  "hora": "14:23:08"
}
```

## 🧠 Lógica de Emoções (Resumo)
Prioridades (da mais crítica para a menos):
1. Doente (tudo <10)
2. Faminto (fome <30)
3. Sonolento / Sonolento Feliz (energia <20 + felicidade alta)
4. Triste (felicidade <30)
5. Entediado (sem interação >60s)
6. Contente (normal) + eventos aleatórios (piscar, assobiar)
7. Dormindo (22h–08h ou induzido)

## 🎮 Mini‑Jogo
- Inicia via botão Jogar (D3) ou `/jogar`
- Personagem salta com botão Alimentar/Saltar (D7)
- Obstáculos avançam; velocidade aumenta a cada 5 pontos
- Colisão → Game Over, volta ao modo Tamagotchi
- `high_score` salvo em EEPROM

## 💾 Persistência
Estrutura gravada:
```cpp
struct TamagotchiState {
  int energia;
  int felicidade;
  int fome;
  int high_score;
  byte magicByte; // 0xAD
};
```
Verificação de integridade por `magicByte` para evitar lixo antigo.

## 🔊 Áudio
- Diferentes sequências de tons para: feliz, acordar, dormir, jogo, assobio, ronco, erro
- Buzzer passivo (usa `tone()`)

## 🗺️ Roadmap / Ideias Futuras
- Sensor de toque ou acelerômetro para interação física
- Múltiplos mini‑jogos (memória, reflexo, ritmo)
- Sistema de evolução / fases de crescimento
- Integração MQTT ou WebSocket para dashboard em tempo real
- Modo economia de energia (reduzir brilho / atualizar menos)
- Internacionalização (i18n) PT/EN automático por navegador
- Persistência avançada em SPIFFS/LittleFS (logs, histórico)
- App móvel (Flutter ou PWA) para notificações push
- Ajuste dinâmico de dificuldade/decadência

## 🧪 Testes / Diagnóstico
- Use Serial Monitor para verificar: conexão Wi‑Fi, IP, carregamento de EEPROM
- Endpoint `/stats` para inspecionar evolução dos atributos
- Verifique se display inicializa sem erro (SSD1306)

## 🔐 Segurança / Observações
- Credenciais Wi‑Fi estão em texto plano (recomenda-se mover para arquivo de configuração separado)
- Sem autenticação nos endpoints (pode-se proteger via rede isolada ou adicionar token simples)
- Voz: requer navegador com suporte (Chrome desktop / Android). Em conexões locais pode haver limitações de permissão.

## ♻️ Boas Práticas Recomendadas
- Trocar `delay()` por lógica não bloqueante (millis) em animações longas
- Extrair motor de emoções para classe separada para facilitar expansão
- Abstrair camadas: DisplayService, EmotionEngine, GameEngine, WebInterface
- Adicionar watchdog / reconexão Wi‑Fi automática

## 🤝 Contribuição
1. Faça um fork
2. Crie branch: `feat/nova-funcionalidade`
3. Commit claro (Português ou Inglês consistente)
4. Pull Request com descrição e screenshots/gifs das animações

## 📄 Licença
Defina aqui uma licença (ex: MIT, Apache 2.0). Se desejar, aviso de uso não comercial também pode ser incluído.
> Me avise qual licença prefere e atualizo esta secção.

## 🙏 Créditos
- Inspiração: Tamagotchi (Bandai)
- Bibliotecas: Adafruit SSD1306 / GFX, NTPClient
- Comunidade Arduino & ESP8266

## 🗨️ FAQ Rápido
- "Ele não responde à voz": Verifique suporte `SpeechRecognition` e permissões de microfone.
- "Assobio não toca": Confirme ligação do buzzer e polaridade.
- "Não salva estado": Verifique espaço EEPROM e reinicialização correta.
- "Display não inicia": Cheque endereço I2C e soldas.

## 🚀 Próximos Passos
- Ajustar fuso horário conforme região
- Escolher licença
- Expandir mini‑jogo / adicionar score online

Bom cuidado com seu Robomotion! 🐾
