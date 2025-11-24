/**
 * ESP32 Turing Machine
 *
 * Implementação de uma Máquina de Turing em ESP32 com:
 * - Interface web para configuração e execução
 * - Display OLED para visualização
 * - Botões físicos para controle
 *
 * Autor: Márcio
 * Projeto: TCC - Ciência da Computação
 */

#include <LittleFS.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================================
// CONFIGURAÇÕES DE HARDWARE
// ============================================================================

// Display OLED (I2C)
const int OLED_WIDTH = 128;
const int OLED_HEIGHT = 64;
const int OLED_SDA = 5;
const int OLED_SCL = 4;
const int OLED_RESET = -1;
const int OLED_ADDRESS = 0x3C;

// Constantes para desenho da fita
const int CELL_SIZE = 18;
const int CELL_SPACING = 17;
const int TAPE_Y = 22;
const int VISIBLE_CELLS = 7;

// Botões (com pull-up interno)
const int BTN_PREV = 12;
const int BTN_SELECT = 14;
const int BTN_NEXT = 2;

// ============================================================================
// CONFIGURAÇÕES DE REDE
// ============================================================================

const char* WIFI_SSID = "MARCIO-WIFI";
const char* WIFI_PASSWORD = "102030405060";

const char* AP_SSID = "ESP32_TuringMachine";
const char* AP_PASSWORD = "123";

const int TIMEOUT_WIFI = 10000;

bool modoAP = false;

// ============================================================================
// OBJETOS GLOBAIS
// ============================================================================

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ============================================================================
// DECLARAÇÕES ANTECIPADAS
// ============================================================================

extern bool displayAtivo;

void drawTape(String tape, int headPos, String currentState, int stepCount, String statusMsg);
void animateTransition(String oldTape, String newTape, int oldPos, int newPos,
                       String oldState, String newState, int stepCount);
void displayMessage(String title, String message, int delayMs = 2000);

// ============================================================================
// ESTRUTURAS DE DADOS
// ============================================================================

struct TransitionInfo {
  bool found;
  String nextState;
  String newSymbol;
  String direction;
};

struct ExecutionResult {
  bool accepted;
  int steps;
  String finalTape;
  String message;
  String history;
};

// ============================================================================
// PROTÓTIPOS DE FUNÇÕES
// ============================================================================

TransitionInfo buscarTransicao(String currentState, char currentSymbol, JsonObject config);
TransitionInfo buscarTransicao(String currentState, char currentSymbol);
bool isEstadoFinal(String state);
bool isEstadoFinal(String state, JsonObject config);
int aplicarTransicao(TransitionInfo transition, String &tape, int headPosition, String &currentState);
int aplicarTransicao(TransitionInfo transition);
void mostrarResultadoFinal(String titulo, String motivo, int passos, bool waitForButton = true);

// ============================================================================
// CLASSE MÁQUINA DE TURING
// ============================================================================

class TuringMachine {
public:
  String currentState;
  String tape;
  int headPosition;
  int stepCount;

  static const int MAX_STEPS = 1000;
  static const int MAX_TAPE_SIZE = 500;

  ExecutionResult execute(String input, String configJSON, bool useDisplay = false, int stepDelay = 500) {
    ExecutionResult result;
    result.accepted = false;
    result.steps = 0;
    result.message = "";
    result.history = "[";

    StaticJsonDocument<8192> doc;
    DeserializationError error = deserializeJson(doc, configJSON);

    if (error) {
      result.message = "Erro ao parsear JSON: " + String(error.c_str());
      result.finalTape = "";
      result.history = "]";
      Serial.println("✗ " + result.message);
      return result;
    }

    String initialState = doc["initialState"] | "q0";

    tape = "^" + input;
    for (int i = 0; i < 20; i++) tape += "_";

    headPosition = 0;
    currentState = initialState;
    stepCount = 0;

    Serial.println("\n=== Executando Máquina de Turing ===");
    Serial.printf("Entrada: %s\n", input.c_str());
    Serial.printf("Fita inicial: %s\n", tape.c_str());
    Serial.printf("Estado inicial: %s\n", currentState.c_str());

    if (useDisplay && displayAtivo) {
      displayMessage("Executando", "Iniciando...", 1000);
      drawTape(tape, headPosition, currentState, stepCount, "Pronto");
      delay(1000);
    }

    bool transitionFound = true;
    while (stepCount < MAX_STEPS && transitionFound) {
      if (headPosition < 0 || headPosition >= tape.length()) {
        result.message = "Erro: Cabeça saiu da fita";
        result.finalTape = tape;
        result.steps = stepCount;
        result.history += "]";
        return result;
      }

      char currentSymbol = tape[headPosition];
      String symbolStr = String(currentSymbol);

      Serial.printf("Passo %d: Estado=%s, Posição=%d, Símbolo=%c\n",
                    stepCount, currentState.c_str(), headPosition, currentSymbol);

      if (stepCount > 0) result.history += ",";
      result.history += "{";
      result.history += "\"step\":" + String(stepCount) + ",";
      result.history += "\"state\":\"" + currentState + "\",";
      result.history += "\"position\":" + String(headPosition) + ",";
      result.history += "\"symbol\":\"" + symbolStr + "\",";
      result.history += "\"tape\":\"" + tape + "\"";
      result.history += "}";

      TransitionInfo transition = buscarTransicao(currentState, currentSymbol, doc.as<JsonObject>());

      if (!transition.found) {
        Serial.printf("✗ Sem transição para estado '%s' e símbolo '%s'\n",
                      currentState.c_str(), symbolStr.c_str());
        transitionFound = false;
        break;
      }

      String oldTape = tape;
      int oldPosition = headPosition;
      String oldState = currentState;

      Serial.printf("→ Transição: (%s, %s, %s)\n",
                    transition.nextState.c_str(), transition.newSymbol.c_str(),
                    transition.direction.c_str());

      int newPosition = aplicarTransicao(transition, tape, headPosition, currentState);

      if (useDisplay && displayAtivo) {
        animateTransition(oldTape, tape, oldPosition, newPosition,
                         oldState, currentState, stepCount);
      }

      headPosition = newPosition;
      stepCount++;

      if (stepCount >= MAX_STEPS) {
        result.message = "Timeout: Limite de passos excedido";
        result.finalTape = tape;
        result.steps = stepCount;
        result.history += "]";
        return result;
      }

      if (isEstadoFinal(currentState, doc.as<JsonObject>())) {
        result.history += "]";
        result.finalTape = tape;
        result.steps = stepCount;
        result.accepted = true;
        result.message = "ACEITO - Estado final";
        Serial.println("✓ ACEITO! Estado final: " + currentState);

        if (useDisplay && displayAtivo) {
          mostrarResultadoFinal("ACEITO", "Estado final", stepCount, false);
        }

        return result;
      }
    }

    result.history += "]";
    result.finalTape = tape;
    result.steps = stepCount;
    result.accepted = false;
    result.message = "REJEITADO - Sem transicao";
    Serial.println("✗ REJEITADO! Sem transição para estado: " + currentState);
    Serial.printf("Passos executados: %d\n", stepCount);
    Serial.printf("Fita final: %s\n", tape.c_str());

    if (useDisplay && displayAtivo) {
      mostrarResultadoFinal("REJEITADO", "Sem transicao", stepCount, false);
    }

    return result;
  }
};

TuringMachine tm;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

bool displayAtivo = false;
bool maquinaExecutando = false;
bool maquinaPausada = false;
bool executarPasso = false;
int scrollOffset = 0;

unsigned long ultimoDebounce = 0;
const int DEBOUNCE_DELAY_MS = 400;

// ============================================================================
// SISTEMA DE NAVEGAÇÃO
// ============================================================================

enum MenuState {
  MENU_PRINCIPAL,
  MENU_SELECIONAR_MT,
  EDITOR_FITA,
  EXECUTANDO_AUTO,
  EXECUTANDO_PASSO,
  RESULTADO_FINAL
};

MenuState estadoAtual = MENU_PRINCIPAL;
int opcaoSelecionada = 0;

String configAtual = "";
String nomeArquivoAtual = "";
JsonArray alfabetoFita;
String entradaUsuario = "";
int posicaoEditor = 0;

String arquivosDisponiveis[20];
int numArquivos = 0;
int arquivoSelecionado = 0;

int passoAtual = 0;
bool aguardandoProximoPasso = false;

StaticJsonDocument<8192> docGlobal;

// ============================================================================
// FUNÇÕES DE TRANSIÇÃO
// ============================================================================

TransitionInfo buscarTransicao(String currentState, char currentSymbol, JsonObject config) {
  TransitionInfo result;
  result.found = false;
  result.nextState = "?";
  result.newSymbol = "?";
  result.direction = "?";

  String symbolStr = String(currentSymbol);
  JsonObject transitions = config["transitions"];

  if (!transitions.containsKey(currentState)) {
    return result;
  }

  JsonObject stateTransitions = transitions[currentState];

  if (!stateTransitions.containsKey(symbolStr)) {
    return result;
  }

  JsonObject transition = stateTransitions[symbolStr];

  result.nextState = transition["nextState"] | "?";
  result.newSymbol = transition["newSymbol"] | "?";
  result.direction = transition["direction"] | "?";

  if (result.nextState == "" || result.nextState == "?" ||
      result.newSymbol == "" || result.newSymbol == "?" ||
      result.direction == "" || result.direction == "?") {
    result.found = false;
    result.nextState = "?";
    result.newSymbol = "?";
    result.direction = "?";
    return result;
  }

  result.found = true;
  return result;
}

TransitionInfo buscarTransicao(String currentState, char currentSymbol) {
  return buscarTransicao(currentState, currentSymbol, docGlobal.as<JsonObject>());
}

bool isEstadoFinal(String state) {
  JsonArray finalStates = docGlobal["finalStates"];
  for (JsonVariant finalState : finalStates) {
    if (finalState.as<String>() == state) {
      return true;
    }
  }
  return false;
}

bool isEstadoFinal(String state, JsonObject config) {
  JsonArray finalStates = config["finalStates"];
  for (JsonVariant finalState : finalStates) {
    if (finalState.as<String>() == state) {
      return true;
    }
  }
  return false;
}

int aplicarTransicao(TransitionInfo transition, String &tape, int headPosition, String &currentState) {
  if (!transition.found) {
    return headPosition;
  }

  tape[headPosition] = transition.newSymbol[0];
  currentState = transition.nextState;

  int newPosition = headPosition;
  if (transition.direction == "L") {
    newPosition--;
  } else if (transition.direction == "R") {
    newPosition++;
  }

  if (newPosition >= tape.length()) {
    tape += "_";
  }

  return newPosition;
}

int aplicarTransicao(TransitionInfo transition) {
  return aplicarTransicao(transition, tm.tape, tm.headPosition, tm.currentState);
}

// ============================================================================
// FUNÇÕES AUXILIARES DE ALFABETO
// ============================================================================

bool isSimboloValidoParaEntrada(String simbolo) {
  return simbolo != "^" && simbolo != "_" && simbolo != "x" && simbolo.length() > 0;
}

char obterPrimeiroSimboloValido() {
  JsonArray inputAlphabet = docGlobal["alphabet"];

  if (inputAlphabet.size() == 0) {
    inputAlphabet = alfabetoFita;
  }

  for (int i = 0; i < inputAlphabet.size(); i++) {
    String simbolo = inputAlphabet[i].as<String>();
    if (isSimboloValidoParaEntrada(simbolo)) {
      return simbolo[0];
    }
  }

  return '0';
}

String obterSimbolosValidosString() {
  JsonArray inputAlphabet = docGlobal["alphabet"];

  if (inputAlphabet.size() == 0) {
    inputAlphabet = alfabetoFita;
  }

  String simbolosValidos = "";
  for (int i = 0; i < inputAlphabet.size(); i++) {
    String simbolo = inputAlphabet[i].as<String>();
    if (isSimboloValidoParaEntrada(simbolo)) {
      simbolosValidos += simbolo[0];
    }
  }

  if (simbolosValidos.length() == 0) {
    simbolosValidos = "01";
  }

  return simbolosValidos;
}

// ============================================================================
// FUNÇÕES DE INICIALIZAÇÃO
// ============================================================================

void addCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

bool inicializarDisplay() {
  Serial.println("Inicializando display OLED...");

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false)) {
    Serial.println("✗ Falha ao inicializar display OLED!");
    return false;
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Turing");
  display.setCursor(10, 30);
  display.println("Machine");
  display.setTextSize(1);
  display.setCursor(20, 55);
  display.println("Iniciando...");
  display.display();

  delay(2000);

  displayAtivo = true;
  Serial.println("✓ Display OLED inicializado!");
  return true;
}

void inicializarBotoes() {
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);

  Serial.println("✓ Botões inicializados!");
}

bool inicializarLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("ERRO: Falha ao montar LittleFS");
    return false;
  }

  Serial.println("LittleFS montado com sucesso!");
  return true;
}

void listarArquivos() {
  Serial.println("=== Arquivos no LittleFS ===");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

bool conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Conectando a: %s", WIFI_SSID);

  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - inicio > TIMEOUT_WIFI) {
      Serial.println("\nTimeout na conexão WiFi");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✓ Conectado ao WiFi!");
  Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void iniciarAP() {
  Serial.println("\n=== Modo Access Point ===");

  modoAP = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();

  Serial.println("✓ Access Point ativo!");
  Serial.printf("  SSID: %s\n", AP_SSID);
  Serial.printf("  IP: %s\n", IP.toString().c_str());

  dnsServer.start(DNS_PORT, "*", IP);
  Serial.println("✓ DNS Captive Portal ativo");
}

// ============================================================================
// FUNÇÕES DO DISPLAY OLED
// ============================================================================

void drawTape(String tape, int headPos, String currentState, int stepCount, String statusMsg) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("q atual:");
  display.print(currentState);

  display.setCursor(70, 0);
  display.print("Passo:");
  display.print(stepCount);

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  int startCell = headPos - 3;
  scrollOffset = startCell;

  int x = 0;
  for (int i = 0; i < VISIBLE_CELLS; i++) {
    int cellIndex = startCell + i;

    if (cellIndex >= 0 && cellIndex < tape.length()) {
      char symbol = tape[cellIndex];

      if (cellIndex == headPos) {
        display.fillRect(x, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else {
        display.drawRect(x, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
      }

      display.setCursor(x + 6, TAPE_Y + 5);
      display.setTextSize(1);
      display.write(symbol);
    }

    x += CELL_SPACING;
  }

  int arrowX = (3 * CELL_SPACING) + 8;
  int arrowY = TAPE_Y + CELL_SIZE + 2;
  display.fillTriangle(
    arrowX, arrowY,
    arrowX - 5, arrowY + 7,
    arrowX + 5, arrowY + 7,
    SSD1306_WHITE
  );

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print(statusMsg);

  display.display();
}

void animateTransition(String oldTape, String newTape, int oldPos, int newPos,
                       String oldState, String newState, int stepCount) {

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  char symbolAtOldPos = newTape[oldPos];
  TransitionInfo trans = buscarTransicao(oldState, oldTape[oldPos]);

  String transInfo = "trans: ";
  if (trans.found) {
    transInfo += trans.nextState + " | " + trans.newSymbol + " | ";
    if (trans.direction == "L") transInfo += "E";
    else if (trans.direction == "R") transInfo += "D";
    else transInfo += "S";
  } else {
    transInfo += "-";
  }

  // Fase 1: Escrita do símbolo
  drawTape(oldTape, oldPos, oldState, stepCount, transInfo);
  delay(200);

  for (int fade = 0; fade < 3; fade++) {
    drawTape(oldTape, oldPos, oldState, stepCount, transInfo);
    delay(80);
    drawTape(newTape, oldPos, oldState, stepCount, transInfo);
    delay(80);
  }

  drawTape(newTape, oldPos, oldState, stepCount, transInfo);
  delay(150);

  // Fase 2: Movimento suave da fita
  if (oldPos != newPos) {
    int direction = (newPos > oldPos) ? 1 : -1;
    int framesPerCell = 12;

    for (int frame = 0; frame <= framesPerCell; frame++) {
      display.clearDisplay();

      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("q atual:");
      display.print(oldState);
      display.setCursor(70, 0);
      display.print("Passo:");
      display.print(stepCount);
      display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

      float t = (float)frame / framesPerCell;
      float easedProgress = t < 0.5 ?
                           2 * t * t :
                           1 - pow(-2 * t + 2, 2) / 2;

      int pixelOffset = (int)(easedProgress * CELL_SPACING);

      int oldStartCell = oldPos - 3;
      int newStartCell = newPos - 3;

      float startCellFloat = oldStartCell + (newStartCell - oldStartCell) * easedProgress;
      int startCell = (int)startCellFloat;

      for (int i = 0; i < VISIBLE_CELLS + 1; i++) {
        int cellIndex = startCell + i;

        if (cellIndex >= 0 && cellIndex < newTape.length()) {
          char symbol = newTape[cellIndex];

          int cellX = i * CELL_SPACING;

          float subCellOffset = (startCellFloat - startCell) * CELL_SPACING;
          cellX -= (int)subCellOffset;

          if (cellX >= -CELL_SIZE && cellX <= OLED_WIDTH) {
            bool shouldHighlight = (cellIndex == oldPos && easedProgress < 0.5) ||
                                  (cellIndex == newPos && easedProgress >= 0.5);

            if (shouldHighlight) {
              display.fillRect(cellX, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
              display.setTextColor(SSD1306_BLACK);
            } else {
              display.drawRect(cellX, TAPE_Y, CELL_SIZE, CELL_SIZE, SSD1306_WHITE);
              display.setTextColor(SSD1306_WHITE);
            }

            display.setCursor(cellX + 6, TAPE_Y + 5);
            display.write(symbol);
          }
        }
      }

      int arrowX = (3 * CELL_SPACING) + 8;
      int arrowY = TAPE_Y + CELL_SIZE + 2;

      display.setTextColor(SSD1306_WHITE);
      display.fillTriangle(
        arrowX, arrowY,
        arrowX - 5, arrowY + 7,
        arrowX + 5, arrowY + 7,
        SSD1306_WHITE
      );

      display.setCursor(0, 55);
      display.print(transInfo);

      display.display();
      delay(25);
    }
  }

  // Fase 3: Novo estado
  char nextSymbol = newTape[newPos];
  TransitionInfo nextTrans = buscarTransicao(newState, nextSymbol);

  String nextTransInfo = "trans: ";

  if (isEstadoFinal(newState)) {
    nextTransInfo += newState + " (final)";
  } else if (nextTrans.found && isEstadoFinal(nextTrans.nextState)) {
    nextTransInfo += nextTrans.nextState + " (final)";
  } else if (nextTrans.found) {
    nextTransInfo += nextTrans.nextState + " | " + nextTrans.newSymbol + " | ";
    if (nextTrans.direction == "L") nextTransInfo += "E";
    else if (nextTrans.direction == "R") nextTransInfo += "D";
    else nextTransInfo += "S";
  } else {
    nextTransInfo += "-";
  }

  drawTape(newTape, newPos, newState, stepCount + 1, nextTransInfo);
  delay(150);
}

void displayMessage(String title, String message, int delayMs) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 10);
  display.setTextSize(2);
  display.println(title);

  display.setCursor(0, 35);
  display.setTextSize(1);
  display.println(message);

  display.display();
  delay(delayMs);
}

void mostrarTelaInicial(String ip) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Pronto!");

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("Acesse pelo ip:");

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println(ip);

  display.setCursor(0, 56);
  display.println("Pressione botao...");

  display.display();

  while (true) {
    int botao = lerBotao();
    if (botao != 0) {
      delay(200);
      break;
    }
    delay(50);
  }
}

void mostrarResultadoFinal(String titulo, String motivo, int passos, bool waitForButton) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 5);
  display.println(titulo);

  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println(motivo);

  display.setCursor(0, 40);
  display.print("Passos: ");
  display.println(passos);

  if (waitForButton) {
    display.setCursor(0, 54);
    display.println("Pressione botao...");
  }

  display.display();

  if (waitForButton) {
    while (true) {
      int botao = lerBotao();
      if (botao != 0) {
        delay(200);
        break;
      }
      delay(50);
    }
  } else {
    delay(3000);
  }
}

// ============================================================================
// FUNÇÕES DOS BOTÕES
// ============================================================================

int lerBotao() {
  bool btnPrevPressed = (digitalRead(BTN_PREV) == LOW);
  bool btnSelectPressed = (digitalRead(BTN_SELECT) == LOW);
  bool btnNextPressed = (digitalRead(BTN_NEXT) == LOW);

  if (!btnPrevPressed && !btnSelectPressed && !btnNextPressed) {
    return 0;
  }

  unsigned long agora = millis();
  if (agora - ultimoDebounce < DEBOUNCE_DELAY_MS) {
    return 0;
  }

  ultimoDebounce = agora;

  if (btnPrevPressed) return 1;
  if (btnSelectPressed) return 2;
  if (btnNextPressed) return 3;

  return 0;
}

// ============================================================================
// FUNÇÕES DE NAVEGAÇÃO E MENUS
// ============================================================================

void carregarListaArquivos() {
  numArquivos = 0;

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file && numArquivos < 20) {
    String fileName = String(file.name());

    if (!fileName.startsWith("/")) {
      fileName = "/" + fileName;
    }

    if (fileName.endsWith(".json")) {

      arquivosDisponiveis[numArquivos] = fileName;
      Serial.printf("  [%d] %s\n", numArquivos, fileName.c_str());
      numArquivos++;
    }

    file = root.openNextFile();
  }

  Serial.printf("Total: %d arquivos .json encontrados\n", numArquivos);
}

bool carregarConfiguracao(String nomeArquivo) {
  Serial.printf("\n=== Carregando MT ===\n");
  Serial.printf("Arquivo solicitado: '%s'\n", nomeArquivo.c_str());

  if (!LittleFS.exists(nomeArquivo)) {
    Serial.printf("✗ Arquivo '%s' não existe no LittleFS!\n", nomeArquivo.c_str());

    Serial.println("\nArquivos disponíveis:");
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    while (f) {
      Serial.printf("  - %s\n", f.name());
      f = root.openNextFile();
    }

    if (displayAtivo) {
      displayMessage("Erro", "Arquivo nao existe", 2000);
    }
    return false;
  }

  File file = LittleFS.open(nomeArquivo, "r");
  if (!file) {
    Serial.printf("✗ Erro ao abrir arquivo '%s'\n", nomeArquivo.c_str());
    if (displayAtivo) {
      displayMessage("Erro", "Falha ao abrir", 2000);
    }
    return false;
  }

  configAtual = file.readString();
  file.close();

  Serial.printf("Tamanho do arquivo: %d bytes\n", configAtual.length());

  if (configAtual.length() == 0) {
    Serial.println("✗ Arquivo vazio!");
    if (displayAtivo) {
      displayMessage("Erro", "Arquivo vazio", 2000);
    }
    return false;
  }

  docGlobal.clear();
  DeserializationError error = deserializeJson(docGlobal, configAtual);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    if (displayAtivo) {
      displayMessage("Erro", "JSON invalido", 2000);
    }
    return false;
  }

  alfabetoFita = docGlobal["tapeAlphabet"];
  nomeArquivoAtual = nomeArquivo;

  String nomeMT = docGlobal["nome"] | "Sem nome";
  String nomeArquivoLimpo = nomeArquivo;
  nomeArquivoLimpo.replace("/", "");
  nomeArquivoLimpo.replace(".json", "");

  Serial.printf("✓ MT carregada: %s (arquivo: %s)\n", nomeMT.c_str(), nomeArquivoLimpo.c_str());
  Serial.printf("  Estados: %d\n", docGlobal["states"].size());
  Serial.printf("  Alfabeto: %d símbolos\n", alfabetoFita.size());

  return true;
}

void desenharMenuPrincipal() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== MENU PRINCIPAL ===");

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  const char* opcoes[] = {
    "1. Modo Automatico",
    "2. Modo Passo-a-Passo",
    "3. Selecionar MT"
  };

  for (int i = 0; i < 3; i++) {
    int y = 18 + (i * 12);

    if (i == opcaoSelecionada) {
      display.fillRect(0, y - 2, OLED_WIDTH, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(2, y);
    display.print(opcoes[i]);
  }

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print("PREV/NEXT SELECT=OK");

  display.display();
}

void desenharMenuSelecaoMT() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== SELECIONAR MT ===");

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  if (numArquivos == 0) {
    display.setCursor(10, 25);
    display.println("Nenhuma MT salva");
  } else {
    int inicio = max(0, arquivoSelecionado - 1);
    int fim = min(numArquivos, inicio + 4);

    for (int i = inicio; i < fim; i++) {
      int y = 14 + ((i - inicio) * 10);

      if (i == arquivoSelecionado) {
        display.fillRect(0, y - 2, OLED_WIDTH, 10, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }

      display.setCursor(2, y);

      String nomeDisplay = arquivosDisponiveis[i];
      nomeDisplay.replace("/", "");
      nomeDisplay.replace(".json", "");

      if (nomeDisplay.length() > 18) {
        nomeDisplay = nomeDisplay.substring(0, 15) + "...";
      }

      display.print(nomeDisplay);
    }
  }

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print("PREV SELEC=OK NEXT");

  display.display();
}

void desenharEditorFita() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== EDITAR FITA ===");

  display.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

  display.setCursor(0, 14);
  String nomeDisplay = nomeArquivoAtual;
  nomeDisplay.replace("/", "");
  nomeDisplay.replace(".json", "");
  if (nomeDisplay.length() > 21) {
    nomeDisplay = nomeDisplay.substring(0, 18) + "...";
  }
  display.print("MT: ");
  display.println(nomeDisplay);

  int y = 28;
  int cellWidth = 16;
  int startX = 4;

  int inicioCelula = posicaoEditor - 3;

  for (int i = 0; i < 7; i++) {
    int posicaoAtual = inicioCelula + i;
    int x = startX + (i * cellWidth);

    bool highlighted = (posicaoAtual == posicaoEditor);

    if (highlighted) {
      display.fillRect(x, y, cellWidth - 1, 16, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.drawRect(x, y, cellWidth - 1, 16, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(x + 4, y + 4);

    if (posicaoAtual < -2) {
      // Célula vazia
    } else if (posicaoAtual == -2) {
      display.print("<<");
    } else if (posicaoAtual == -1) {
      display.print("X");
    } else if (posicaoAtual == 0) {
      display.print(">");
    } else if (posicaoAtual > 0 && posicaoAtual <= entradaUsuario.length()) {
      display.write(entradaUsuario[posicaoAtual - 1]);
    } else if (posicaoAtual > entradaUsuario.length()) {
      display.print("+");
    }
  }

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50);

  if (posicaoEditor == -2) {
    display.println("SELECT=Voltar");
  } else if (posicaoEditor == -1) {
    display.println("SELECT=Resetar");
  } else if (posicaoEditor == 0) {
    display.println("SELECT=Iniciar MT");
  } else if (posicaoEditor > entradaUsuario.length()) {
    display.println("SELECT=Add simbolo");
  } else {
    display.println("SELECT=Trocar");
  }

  display.display();
}

void alternarSimboloEditor() {
  if (posicaoEditor < 1 || posicaoEditor > entradaUsuario.length()) {
    Serial.println("Posição inválida para alternar símbolo");
    return;
  }

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  String simbolosValidos = obterSimbolosValidosString();

  int indiceString = posicaoEditor - 1;
  char simboloAtual = entradaUsuario[indiceString];
  int indiceAtual = simbolosValidos.indexOf(simboloAtual);

  if (indiceAtual == -1) {
    indiceAtual = 0;
  }

  int proximoIndice = (indiceAtual + 1) % simbolosValidos.length();
  entradaUsuario[indiceString] = simbolosValidos[proximoIndice];

  Serial.printf("Símbolo alterado na pos %d: %c -> %c\n",
                posicaoEditor, simboloAtual, simbolosValidos[proximoIndice]);
}

// ============================================================================
// HANDLERS DO SERVIDOR WEB
// ============================================================================

void servirArquivo(const char* caminho, const char* contentType) {
  File file = LittleFS.open(caminho, "r");

  if (!file) {
    String msg = "Arquivo " + String(caminho) + " não encontrado";
    server.send(404, "text/plain", msg);
    return;
  }

  server.streamFile(file, contentType);
  file.close();
}

void handleRoot() {
  servirArquivo("/index.html", "text/html");
}

void handleCSS() {
  servirArquivo("/styles.css", "text/css");
}

void handleJS() {
  servirArquivo("/script.js", "application/javascript");
}

void handleStatus() {
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"modo\":\"" + String(modoAP ? "AP" : "STA") + "\",";

  if (modoAP) {
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  } else {
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  }

  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  server.send(200, "application/json", json);
}

void handleNotFound() {
  String message = "Erro 404 - Página não encontrada\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Método: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";

  server.send(404, "text/plain", message);
}

void handleSave() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  String body = server.arg("plain");
  String fileName = "/machine.json";

  if (server.hasArg("filename")) {
    fileName = "/" + server.arg("filename");
    if (!fileName.endsWith(".json")) {
      fileName += ".json";
    }
  }

  Serial.printf("Salvando configuração em: %s\n", fileName.c_str());

  File file = LittleFS.open(fileName, "w");
  if (!file) {
    server.send(500, "application/json", "{\"error\":\"Erro ao criar arquivo\"}");
    return;
  }

  file.print(body);
  file.close();

  String response = "{\"success\":true,\"message\":\"Configuração salva\",\"file\":\"" + fileName + "\"}";
  server.send(200, "application/json", response);

  Serial.println("✓ Configuração salva com sucesso!");
}

void handleLoad() {
  addCORSHeaders();

  String fileName = "/machine.json";

  if (server.hasArg("filename")) {
    fileName = "/" + server.arg("filename");
    if (!fileName.endsWith(".json")) {
      fileName += ".json";
    }
  }

  Serial.printf("Carregando configuração de: %s\n", fileName.c_str());

  File file = LittleFS.open(fileName, "r");
  if (!file) {
    Serial.println("✗ Arquivo não encontrado!");
    server.send(404, "application/json", "{\"error\":\"Arquivo não encontrado\"}");
    return;
  }

  String content = file.readString();
  file.close();

  Serial.println("✓ Configuração carregada");

  server.send(200, "application/json", content);
}

void handleListFiles() {
  addCORSHeaders();

  String json = "{\"files\":[";

  File root = LittleFS.open("/");
  File file = root.openNextFile();
  bool first = true;

  while (file) {
    String fileName = String(file.name());

    if (fileName.endsWith(".json")) {
      if (!first) json += ",";
      json += "{\"name\":\"" + fileName + "\",\"size\":" + String(file.size()) + "}";
      first = false;
    }

    file = root.openNextFile();
  }

  json += "]}";

  server.send(200, "application/json", json);
}

void handleDelete() {
  addCORSHeaders();

  if (!server.hasArg("filename")) {
    server.send(400, "application/json", "{\"error\":\"Nome do arquivo não fornecido\"}");
    return;
  }

  String fileName = "/" + server.arg("filename");
  if (!fileName.endsWith(".json")) {
    fileName += ".json";
  }

  if (LittleFS.remove(fileName)) {
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Arquivo deletado\"}");
    Serial.printf("✓ Arquivo %s deletado\n", fileName.c_str());
  } else {
    server.send(404, "application/json", "{\"error\":\"Arquivo não encontrado\"}");
  }
}

void handleExecute() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  String body = server.arg("plain");

  Serial.println("\n=== API: Executar MT ===");

  StaticJsonDocument<8192> requestDoc;
  DeserializationError error = deserializeJson(requestDoc, body);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    String errorMsg = "{\"error\":\"Erro ao parsear JSON: " + String(error.c_str()) + "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }

  String input = requestDoc["input"] | "";
  JsonObject config = requestDoc["config"];

  if (input == "") {
    server.send(400, "application/json", "{\"error\":\"Campo 'input' não encontrado\"}");
    return;
  }

  if (config.isNull()) {
    server.send(400, "application/json", "{\"error\":\"Campo 'config' não encontrado\"}");
    return;
  }

  Serial.printf("Entrada: %s\n", input.c_str());

  String configJSON;
  serializeJson(config, configJSON);

  ExecutionResult result = tm.execute(input, configJSON);

  String response = "{";
  response += "\"accepted\":" + String(result.accepted ? "true" : "false") + ",";
  response += "\"message\":\"" + result.message + "\",";
  response += "\"steps\":" + String(result.steps) + ",";
  response += "\"finalTape\":\"" + result.finalTape + "\",";
  response += "\"history\":" + result.history;
  response += "}";

  server.send(200, "application/json", response);
}

void handleExecuteDisplay() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  if (!displayAtivo) {
    server.send(503, "application/json", "{\"error\":\"Display OLED não está ativo\"}");
    return;
  }

  String body = server.arg("plain");

  Serial.println("\n=== API: Executar MT no Display ===");

  StaticJsonDocument<8192> requestDoc;
  DeserializationError error = deserializeJson(requestDoc, body);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    String errorMsg = "{\"error\":\"Erro ao parsear JSON: " + String(error.c_str()) + "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }

  String input = requestDoc["input"] | "";
  JsonObject config = requestDoc["config"];
  int stepDelay = requestDoc["delay"] | 500;

  if (input == "") {
    server.send(400, "application/json", "{\"error\":\"Campo 'input' não encontrado\"}");
    return;
  }

  if (config.isNull()) {
    server.send(400, "application/json", "{\"error\":\"Campo 'config' não encontrado\"}");
    return;
  }

  String configJSON;
  serializeJson(config, configJSON);

  server.send(202, "application/json", "{\"status\":\"executing\",\"message\":\"Executando no display\"}");

  ExecutionResult result = tm.execute(input, configJSON, true, stepDelay);

  Serial.printf("Execução no display finalizada: %s\n", result.message.c_str());
}

void handleStartStepMode() {
  addCORSHeaders();

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
    return;
  }

  if (!displayAtivo) {
    server.send(503, "application/json", "{\"error\":\"Display OLED não está ativo\"}");
    return;
  }

  String body = server.arg("plain");

  Serial.println("\n=== API: Iniciar Modo Passo a Passo ===");

  StaticJsonDocument<8192> requestDoc;
  DeserializationError error = deserializeJson(requestDoc, body);

  if (error) {
    Serial.printf("✗ Erro ao parsear JSON: %s\n", error.c_str());
    String errorMsg = "{\"error\":\"Erro ao parsear JSON: " + String(error.c_str()) + "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }

  String input = requestDoc["input"] | "";
  JsonObject config = requestDoc["config"];

  if (input == "") {
    server.send(400, "application/json", "{\"error\":\"Campo 'input' não encontrado\"}");
    return;
  }

  if (config.isNull()) {
    server.send(400, "application/json", "{\"error\":\"Campo 'config' não encontrado\"}");
    return;
  }

  entradaUsuario = input;

  configAtual = "";
  serializeJson(config, configAtual);

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  Serial.printf("Entrada: %s\n", entradaUsuario.c_str());

  server.send(200, "application/json", "{\"status\":\"step_mode_started\",\"message\":\"Modo passo a passo iniciado. Use os botões físicos do ESP32.\"}");

  iniciarExecucaoPasso();
}

void handleOptions() {
  addCORSHeaders();
  server.send(204);
}

void configurarServidor() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/styles.css", HTTP_GET, handleCSS);
  server.on("/script.js", HTTP_GET, handleJS);

  server.on("/status", HTTP_GET, handleStatus);

  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/save", HTTP_OPTIONS, handleOptions);

  server.on("/api/load", HTTP_GET, handleLoad);
  server.on("/api/load", HTTP_OPTIONS, handleOptions);

  server.on("/api/files", HTTP_GET, handleListFiles);
  server.on("/api/files", HTTP_OPTIONS, handleOptions);

  server.on("/api/delete", HTTP_DELETE, handleDelete);
  server.on("/api/delete", HTTP_OPTIONS, handleOptions);

  server.on("/api/execute", HTTP_POST, handleExecute);
  server.on("/api/execute", HTTP_OPTIONS, handleOptions);

  server.on("/api/execute-display", HTTP_POST, handleExecuteDisplay);
  server.on("/api/execute-display", HTTP_OPTIONS, handleOptions);

  server.on("/api/start-step-mode", HTTP_POST, handleStartStepMode);
  server.on("/api/start-step-mode", HTTP_OPTIONS, handleOptions);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("✓ Servidor web iniciado!");
}

// ============================================================================
// PROCESSAMENTO DE BOTÕES
// ============================================================================

void processarBotoes() {
  int botao = lerBotao();

  if (botao == 0) {
    return;
  }

  switch (estadoAtual) {
    case MENU_PRINCIPAL:
      processarMenuPrincipal(botao);
      break;

    case MENU_SELECIONAR_MT:
      processarMenuSelecaoMT(botao);
      break;

    case EDITOR_FITA:
      processarEditorFita(botao);
      break;

    case EXECUTANDO_PASSO:
      processarExecucaoPasso(botao);
      break;

    case RESULTADO_FINAL:
      processarResultadoFinal(botao);
      break;

    default:
      break;
  }
}

void processarMenuPrincipal(int botao) {
  if (botao == 1) {
    opcaoSelecionada = (opcaoSelecionada - 1 + 3) % 3;
    desenharMenuPrincipal();

  } else if (botao == 3) {
    opcaoSelecionada = (opcaoSelecionada + 1) % 3;
    desenharMenuPrincipal();

  } else if (botao == 2) {
    if (opcaoSelecionada == 0) {
      if (configAtual == "") {
        displayMessage("Aviso", "Selecione uma MT", 2000);
        estadoAtual = MENU_SELECIONAR_MT;
        arquivoSelecionado = 0;
        carregarListaArquivos();
        desenharMenuSelecaoMT();
      } else {
        estadoAtual = EDITOR_FITA;
        posicaoEditor = 0;
        inicializarEditor();
        desenharEditorFita();
      }

    } else if (opcaoSelecionada == 1) {
      if (configAtual == "") {
        displayMessage("Aviso", "Selecione uma MT", 2000);
        estadoAtual = MENU_SELECIONAR_MT;
        arquivoSelecionado = 0;
        carregarListaArquivos();
        desenharMenuSelecaoMT();
      } else {
        estadoAtual = EDITOR_FITA;
        posicaoEditor = 0;
        inicializarEditor();
        desenharEditorFita();
      }

    } else if (opcaoSelecionada == 2) {
      estadoAtual = MENU_SELECIONAR_MT;
      arquivoSelecionado = 0;
      carregarListaArquivos();
      desenharMenuSelecaoMT();
    }
  }
}

void processarMenuSelecaoMT(int botao) {
  if (numArquivos == 0) {
    if (botao == 2) {
      estadoAtual = MENU_PRINCIPAL;
      opcaoSelecionada = 0;
      desenharMenuPrincipal();
    }
    return;
  }

  if (botao == 1) {
    arquivoSelecionado = (arquivoSelecionado - 1 + numArquivos) % numArquivos;
    desenharMenuSelecaoMT();

  } else if (botao == 3) {
    arquivoSelecionado = (arquivoSelecionado + 1) % numArquivos;
    desenharMenuSelecaoMT();

  } else if (botao == 2) {
    if (carregarConfiguracao(arquivosDisponiveis[arquivoSelecionado])) {
      displayMessage("Sucesso", "MT carregada!", 1500);
      estadoAtual = MENU_PRINCIPAL;
      opcaoSelecionada = 0;
      desenharMenuPrincipal();
    }
  }
}

void inicializarEditor() {
  if (entradaUsuario.length() == 0) {
    docGlobal.clear();
    deserializeJson(docGlobal, configAtual);
    entradaUsuario = String(obterPrimeiroSimboloValido());
  }

  posicaoEditor = 0;

  Serial.printf("Editor inicializado: entrada='%s', pos=%d\n", entradaUsuario.c_str(), posicaoEditor);
}

void resetarFita() {
  entradaUsuario = "";
  inicializarEditor();
  Serial.println("Fita resetada");
}

void adicionarSimboloEditor() {
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  char novoSimbolo = obterPrimeiroSimboloValido();
  entradaUsuario += novoSimbolo;
  Serial.printf("Símbolo adicionado: %c (nova entrada: %s)\n", novoSimbolo, entradaUsuario.c_str());
}

void processarEditorFita(int botao) {
  if (botao == 1) {
    posicaoEditor--;

    if (posicaoEditor < -2) {
      posicaoEditor = -2;
    }

    desenharEditorFita();

  } else if (botao == 3) {
    if (posicaoEditor < 0 || posicaoEditor <= entradaUsuario.length()) {
      posicaoEditor++;
    }

    desenharEditorFita();

  } else if (botao == 2) {
    if (posicaoEditor == -2) {
      Serial.println("Voltando ao menu principal");
      estadoAtual = MENU_PRINCIPAL;
      opcaoSelecionada = 0;
      desenharMenuPrincipal();

    } else if (posicaoEditor == -1) {
      resetarFita();
      desenharEditorFita();

    } else if (posicaoEditor == 0) {
      if (entradaUsuario.length() == 0) {
        displayMessage("Aviso", "Fita vazia!", 1500);
        desenharEditorFita();
        return;
      }

      if (opcaoSelecionada == 0) {
        iniciarExecucaoAutomatica();
      } else {
        iniciarExecucaoPasso();
      }

    } else if (posicaoEditor > 0 && posicaoEditor <= entradaUsuario.length()) {
      alternarSimboloEditor();
      desenharEditorFita();

    } else if (posicaoEditor > entradaUsuario.length()) {
      adicionarSimboloEditor();
      desenharEditorFita();
    }
  }
}

// ============================================================================
// EXECUÇÃO DA MÁQUINA DE TURING
// ============================================================================

void iniciarExecucaoAutomatica() {
  Serial.println("=== Iniciando Execução Automática ===");
  Serial.printf("Entrada: %s\n", entradaUsuario.c_str());

  tm.tape = "^" + entradaUsuario;
  for (int i = 0; i < 20; i++) tm.tape += "_";

  tm.headPosition = 0;
  tm.stepCount = 0;

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  tm.currentState = docGlobal["initialState"] | "q0";

  Serial.printf("Estado inicial: %s\n", tm.currentState.c_str());
  Serial.printf("Fita inicial: %s\n", tm.tape.c_str());

  displayMessage("Executando", "Iniciando...", 1000);

  char firstSymbol = tm.tape[tm.headPosition];
  TransitionInfo firstTrans = buscarTransicao(tm.currentState, firstSymbol);
  String firstTransInfo = "trans: ";

  if (isEstadoFinal(tm.currentState)) {
    firstTransInfo += tm.currentState + " (final)";
  } else if (firstTrans.found && isEstadoFinal(firstTrans.nextState)) {
    firstTransInfo += firstTrans.nextState + " (final)";
  } else if (firstTrans.found) {
    firstTransInfo += firstTrans.nextState + " | " + firstTrans.newSymbol + " | ";
    if (firstTrans.direction == "L") firstTransInfo += "E";
    else if (firstTrans.direction == "R") firstTransInfo += "D";
    else firstTransInfo += "S";
  } else {
    firstTransInfo += "-";
  }

  drawTape(tm.tape, tm.headPosition, tm.currentState, tm.stepCount, firstTransInfo);
  delay(1000);

  bool continuar = true;
  int maxSteps = 1000;

  while (continuar && tm.stepCount < maxSteps) {
    continuar = executarPassoAutomatico();

    if (continuar) {
      delay(500);
    }
  }

  bool accepted = isEstadoFinal(tm.currentState);
  String titulo = accepted ? "ACEITO" : "REJEITADO";
  String motivo = accepted ? "Estado final" : "Sem transicao";

  Serial.printf("\n=== Resultado: %s ===\n", titulo.c_str());
  Serial.printf("Motivo: %s\n", motivo.c_str());
  Serial.printf("Passos executados: %d\n", tm.stepCount);
  Serial.printf("Estado final: %s\n", tm.currentState.c_str());
  Serial.printf("Fita final: %s\n", tm.tape.c_str());

  mostrarResultadoFinal(titulo, motivo, tm.stepCount);

  estadoAtual = MENU_PRINCIPAL;
  opcaoSelecionada = 0;
  desenharMenuPrincipal();
}

bool executarPassoAutomatico() {
  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  if (isEstadoFinal(tm.currentState)) {
    Serial.println("Estado final atingido");
    return false;
  }

  if (tm.headPosition < 0 || tm.headPosition >= tm.tape.length()) {
    Serial.println("Erro: Cabeça fora da fita");
    displayMessage("ERRO", "Cabeca fora da fita", 2000);
    return false;
  }

  char currentSymbol = tm.tape[tm.headPosition];
  Serial.printf("Passo %d: estado=%s, pos=%d, simbolo=%c\n",
                tm.stepCount, tm.currentState.c_str(), tm.headPosition, currentSymbol);

  TransitionInfo transition = buscarTransicao(tm.currentState, currentSymbol);

  if (!transition.found) {
    Serial.println("Sem transição disponível");
    return false;
  }

  Serial.printf("Transição: (%s, %s, %s)\n",
                transition.nextState.c_str(), transition.newSymbol.c_str(), transition.direction.c_str());

  String oldTape = tm.tape;
  int oldPosition = tm.headPosition;
  String oldState = tm.currentState;

  int newPosition = aplicarTransicao(transition);
  animateTransition(oldTape, tm.tape, oldPosition, newPosition,
                     oldState, tm.currentState, tm.stepCount);

  tm.headPosition = newPosition;
  tm.stepCount++;

  if (isEstadoFinal(tm.currentState)) {
    return false;
  }

  return true;
}

void iniciarExecucaoPasso() {
  Serial.println("=== Iniciando Execução Passo-a-Passo ===");
  Serial.printf("Entrada: %s\n", entradaUsuario.c_str());

  tm.tape = "^" + entradaUsuario;
  for (int i = 0; i < 20; i++) tm.tape += "_";

  tm.headPosition = 0;
  tm.stepCount = 0;

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);
  tm.currentState = docGlobal["initialState"] | "q0";

  estadoAtual = EXECUTANDO_PASSO;
  aguardandoProximoPasso = true;

  char currentSymbol = tm.tape[tm.headPosition];
  TransitionInfo transition = buscarTransicao(tm.currentState, currentSymbol);

  String transInfo = "trans: ";

  if (isEstadoFinal(tm.currentState)) {
    transInfo += tm.currentState + " (final)";
  } else if (transition.found && isEstadoFinal(transition.nextState)) {
    transInfo += transition.nextState + " (final)";
  } else if (transition.found) {
    transInfo += transition.nextState + " | " + transition.newSymbol + " | ";
    if (transition.direction == "L") transInfo += "E";
    else if (transition.direction == "R") transInfo += "D";
    else transInfo += "S";
  } else {
    transInfo += "-";
  }

  drawTape(tm.tape, tm.headPosition, tm.currentState, tm.stepCount, transInfo);
}

void processarExecucaoPasso(int botao) {
  if (botao == 2) {
    executarProximoPasso();
  }
}

void executarProximoPasso() {
  Serial.printf("Executando passo %d\n", tm.stepCount);

  docGlobal.clear();
  deserializeJson(docGlobal, configAtual);

  if (tm.headPosition < 0 || tm.headPosition >= tm.tape.length()) {
    displayMessage("ERRO", "Cabeca fora da fita", 2000);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  char currentSymbol = tm.tape[tm.headPosition];
  TransitionInfo transition = buscarTransicao(tm.currentState, currentSymbol);

  if (isEstadoFinal(tm.currentState) && !transition.found) {
    mostrarResultadoFinal("ACEITO", "Estado final", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  if (!transition.found) {
    mostrarResultadoFinal("REJEITADO", "Sem transicao", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  String oldTape = tm.tape;
  int oldPosition = tm.headPosition;
  String oldState = tm.currentState;

  int newPosition = aplicarTransicao(transition);
  animateTransition(oldTape, tm.tape, oldPosition, newPosition,
                     oldState, tm.currentState, tm.stepCount);

  tm.headPosition = newPosition;
  tm.stepCount++;

  if (isEstadoFinal(tm.currentState)) {
    mostrarResultadoFinal("ACEITO", "Estado final", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }

  char nextSymbol = tm.tape[tm.headPosition];
  TransitionInfo nextTrans = buscarTransicao(tm.currentState, nextSymbol);

  if (!nextTrans.found) {
    mostrarResultadoFinal("REJEITADO", "Sem transicao", tm.stepCount);
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
    return;
  }
}

void processarResultadoFinal(int botao) {
  if (botao == 2) {
    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
  }
}

// ============================================================================
// SETUP E LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=================================");
  Serial.println("  ESP32 Turing Machine");
  Serial.println("  Versão: 2.0");
  Serial.println("=================================\n");

  if (!inicializarLittleFS()) {
    Serial.println("ERRO: Não foi possível montar LittleFS");
    return;
  }

  if (!conectarWiFi()) {
    iniciarAP();
  }

  configurarServidor();

  inicializarDisplay();

  inicializarBotoes();

  if (modoAP) {
    Serial.printf("\nAcesse: http://%s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("\nAcesse: http://%s\n", WiFi.localIP().toString().c_str());
  }

  if (displayAtivo) {
    String ip = modoAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    mostrarTelaInicial(ip);

    carregarListaArquivos();

    if (numArquivos > 0) {
      if (carregarConfiguracao(arquivosDisponiveis[0])) {
        Serial.println("✓ MT padrão carregada");
      }
    }

    estadoAtual = MENU_PRINCIPAL;
    opcaoSelecionada = 0;
    desenharMenuPrincipal();
  }

  Serial.println("\n=== Sistema pronto! ===\n");
}

void loop() {
  server.handleClient();

  if (modoAP) {
    dnsServer.processNextRequest();
  }

  if (!modoAP && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado! Reconectando...");
    conectarWiFi();
  }

  if (displayAtivo) {
    processarBotoes();
  }

  delay(10);
}
