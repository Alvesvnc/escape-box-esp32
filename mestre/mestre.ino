/*
 * Escape Box - Firmware do ESP32 MESTRE
 * Projeto: Escape Box com ESP32 para o ensino introdutorio de
 *          sistemas embarcados (FUCAPI 2026)
 *
 * Bibliotecas (Gerenciador de Bibliotecas do Arduino IDE):
 *   - ESP32Servo (Kevin Harrington)
 *   - Adafruit SSD1306
 *   - Adafruit GFX
 *
 * Fluxo: INTRO -> D1 (saida digital) -> D2 (ADC + if) -> D3 (UART) -> VITORIA
 *        Tempo esgotado em qualquer etapa -> DERROTA
 *
 * Modo manutencao: segurar o botao de dica ao ligar abre todas as travas
 *                  (para colocar as pistas). Apertar de novo trava tudo.
 * Reiniciar partida: na tela final, segurar o botao por 5 s
 *                    (feche os compartimentos antes).
 *
 * Log da partida (CSV): LOG,<segundos desde o inicio>,<evento>,<detalhe>
 *   - Impresso no Monitor Serial (115200) e gravado na flash (LittleFS),
 *     em /partida.csv. A partida anterior fica em /anterior.csv.
 *   - Abrir o Monitor Serial reinicia o DevKit (sinais DTR/RTS). Por isso
 *     o log nao depende do monitor aberto: depois da partida, abra o
 *     monitor e envie L (log atual), A (partida anterior) ou C (apagar).
 *   - Erros sao registrados como episodios: um evento quando o erro
 *     comeca, nao a cada amostra. Ver o dicionario de eventos no README.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <LittleFS.h>

// ================= PINOS =================
const int PIN_SERVO[3] = {13, 14, 27};  // comp. 1, comp. 2, tampa final
const int PIN_BUZZER   = 25;
const int PIN_LED_OK   = 26;
const int PIN_LED_ERR  = 33;
const int PIN_BTN_DICA = 32;            // botao para GND
const int PIN_SINAL    = 18;            // <- GPIO 23 da placa do aluno (via 1k)
const int PIN_POT      = 36;            // wiper do potenciometro (VP)
const int PIN_RX2      = 16;            // <- TX (17) da placa do aluno (via 1k)
const int PIN_TX2      = 17;            // -> RX (16) da placa do aluno (via 1k)

// ================= JOGO =================
const uint32_t TEMPO_TOTAL_MS  = 45UL * 60UL * 1000UL;
const uint32_t PENALIDADE_DICA = 3UL * 60UL * 1000UL;
const int      MAX_DICAS       = 3;
const int      ANG_TRAVADO     = 0;     // ajuste conforme a montagem
const int      ANG_ABERTO      = 90;

// D1: piscar a 2 Hz (periodo 500 ms +-10%)
const uint32_t PERIODO_MIN_US = 450000;
const uint32_t PERIODO_MAX_US = 550000;
const int      CICLOS_OK_D1   = 6;      // ~3 s estavel

// D2: sinal HIGH so dentro da faixa
const int FAIXA_MIN      = 2000;
const int FAIXA_MAX      = 2800;
const int TOLERANCIA     = 150;         // zona ignorada perto das bordas
const int AMOSTRAS_OK_D2 = 20;          // 20 amostras de 50 ms = 1 s

// D3: cifra de Cesar pela UART
const int      DESLOCAMENTO      = 7;
const uint32_t INTERVALO_MSG_MS  = 5000;
const uint32_t PRAZO_RESPOSTA_MS = 3000;
const int      ACERTOS_D3        = 3;
const int      TAM_CODIGO        = 5;     // codigo aleatorio: 26^5 combinacoes

const char* OBJETIVO[3] = {
  "Pisque o sinal a 2 Hz",
  "HIGH so na faixa",
  "Decifre e responda"
};

const char* DICAS[3][3] = {
  {"Use pinMode e digitalWrite no pino 23.",
   "2 Hz = 2 piscadas por segundo.",
   "Periodo de 500 ms: 250 ligado, 250 desligado."},
  {"analogRead(34) vai de 0 a 4095.",
   "Compare a leitura com a faixa usando if.",
   "Fora da faixa o pino precisa ficar LOW."},
  {"So falta a funcao decifrar() do modelo.",
   "Volte cada letra o numero de posicoes da pista.",
   "(c - 'A' - N + 26) % 26 + 'A'"}
};

// ================= ESTADO =================
enum Estado { INTRO, D1, D2, D3, VITORIA, DERROTA };
Estado estado = INTRO;

Adafruit_SSD1306 tela(128, 64, &Wire, -1);
Servo travas[3];

uint32_t inicioJogo = 0;
uint32_t prazoFinal = 0;
int32_t  tempoFinal = 0;
int      etapaAlcancada = 0;
int      dicasUsadas = 0;
int      proxDica[3] = {0, 0, 0};
const char* dicaAtual = nullptr;
uint32_t dicaAte = 0;

String   statusTxt = "";
uint32_t erroAte = 0;
uint32_t ultimoErro = 0;

// Log por episodio de erro
String   ultimoCodErro = "";
uint32_t ultimoLogErro = 0;
int      errosEtapa[3] = {0, 0, 0};
uint32_t inicioEtapa = 0;
bool     fsOk = false;

// D1
volatile uint32_t ultimaSubida = 0;
volatile uint32_t periodoMedido = 0;
volatile bool     novoPeriodo = false;
int ciclosOk = 0;

// D2
int okDentro = 0, okFora = 0;
uint32_t ultimaAmostra = 0;

// D3
String   codigoAtual = "";
uint32_t enviadaEm = 0;
int      acertos = 0;
String   bufferRx = "";

// ================= UTILITARIOS =================
int32_t restante() { return (int32_t)(prazoFinal - millis()); }

void logEvento(const String& evento, const String& detalhe = "") {
  float s = inicioJogo ? (millis() - inicioJogo) / 1000.0f : 0;
  char linha[96];
  snprintf(linha, sizeof(linha), "LOG,%.1f,%s,%s", s, evento.c_str(), detalhe.c_str());
  Serial.println(linha);
  if (fsOk && inicioJogo) {
    File f = LittleFS.open("/partida.csv", FILE_APPEND);
    if (f) { f.println(linha); f.close(); }
  }
}

// Registra um erro quando um episodio comeca: codigo diferente do anterior
// ou episodio encerrado por acerto sustentado (encerrarEpisodio).
// Um erro que persiste volta ao log a cada 5 s com ";continua" e nao
// conta como episodio novo em episodios_erro.
void logErro(int etapa, const String& codigo, const String& detalhe) {
  bool novo = codigo != ultimoCodErro;
  if (!novo && millis() - ultimoLogErro < 5000) return;
  ultimoCodErro = codigo;
  ultimoLogErro = millis();
  if (novo) errosEtapa[etapa]++;
  logEvento(codigo, novo ? detalhe : detalhe + ";continua");
}

// Episodio encerrado quando o aluno volta a acertar
void encerrarEpisodio() { ultimoCodErro = ""; }

void imprimirArquivo(const char* caminho) {
  if (!fsOk) { Serial.println("Sistema de arquivos indisponivel"); return; }
  File f = LittleFS.open(caminho, FILE_READ);
  if (!f) { Serial.printf("%s vazio\n", caminho); return; }
  Serial.printf("---- %s ----\n", caminho);
  while (f.available()) Serial.write(f.read());
  Serial.println("---- fim ----");
  f.close();
}

void comandosSerial() {
  while (Serial.available()) {
    char c = toupper(Serial.read());
    if (c == 'L') imprimirArquivo("/partida.csv");
    else if (c == 'A') imprimirArquivo("/anterior.csv");
    else if (c == 'C' && fsOk) {
      LittleFS.remove("/partida.csv");
      LittleFS.remove("/anterior.csv");
      Serial.println("Logs apagados");
    }
  }
}

void setStatus(const String& s) { if (millis() >= erroAte) statusTxt = s; }
void setErro(const String& s)   { statusTxt = s; erroAte = millis() + 1500; }

void bip(int ms) { digitalWrite(PIN_BUZZER, HIGH); delay(ms); digitalWrite(PIN_BUZZER, LOW); }

void feedbackOk() {
  digitalWrite(PIN_LED_OK, HIGH);
  bip(80); delay(60); bip(80);
  digitalWrite(PIN_LED_OK, LOW);
}

void feedbackErro() {
  if (millis() - ultimoErro < 1500) return;  // evita bip continuo
  ultimoErro = millis();
  digitalWrite(PIN_LED_ERR, HIGH);
  bip(300);
  digitalWrite(PIN_LED_ERR, LOW);
}

void melodiaVitoria() {
  digitalWrite(PIN_LED_OK, HIGH);
  for (int i = 0; i < 4; i++) { bip(100); delay(80); }
  bip(600);
}

void moverTrava(int i, int ang) {
  travas[i].attach(PIN_SERVO[i], 500, 2400);
  travas[i].write(ang);
  delay(450);
  travas[i].detach();  // evita tremor e consumo parado
}

// 0 = nada, 1 = clique curto, 2 = segurou 5 s
int lerBotao() {
  static bool pressionado = false;
  static uint32_t inicio = 0;
  static bool longo = false;
  bool agora = digitalRead(PIN_BTN_DICA) == LOW;
  uint32_t t = millis();
  if (agora && !pressionado) { pressionado = true; inicio = t; longo = false; }
  else if (agora && pressionado && !longo && t - inicio >= 5000) { longo = true; return 2; }
  else if (!agora && pressionado) {
    pressionado = false;
    if (!longo && t - inicio >= 30) return 1;
  }
  return 0;
}

// ================= D1 =================
void IRAM_ATTR aoSubir() {
  uint32_t t = micros();
  uint32_t p = t - ultimaSubida;
  if (p < 20000) return;  // ignora ruido
  // Periodo acima de 2 s: o sinal estava parado; esta borda recomeca a medida
  if (ultimaSubida != 0 && p <= 2000000UL) { periodoMedido = p; novoPeriodo = true; }
  ultimaSubida = t;
}

void iniciarD1() {
  ultimaSubida = 0; novoPeriodo = false; ciclosOk = 0;
  attachInterrupt(digitalPinToInterrupt(PIN_SINAL), aoSubir, RISING);
}

void loopD1() {
  if (novoPeriodo) {
    noInterrupts();
    uint32_t p = periodoMedido;
    novoPeriodo = false;
    interrupts();
    float hz = 1000000.0f / p;
    if (p >= PERIODO_MIN_US && p <= PERIODO_MAX_US) {
      ciclosOk++;
      if (ciclosOk >= 2) encerrarEpisodio();  // acerto sustentado fecha o episodio
      setStatus("Freq " + String(hz, 1) + " Hz OK " + String(ciclosOk) + "/" + String(CICLOS_OK_D1));
      if (ciclosOk >= CICLOS_OK_D1) concluirEtapa(0);
    } else {
      ciclosOk = 0;
      setErro("Freq " + String(hz, 1) + " Hz, alvo 2");
      logErro(0, "erro_d1", String(hz, 2));
      feedbackErro();
    }
  }
  // Sinal parado por mais de 2 s depois de ja ter oscilado (pino fixo ou
  // programa travado): um registro no log, sem bip, porque no inicio da
  // etapa o sinal parado e normal e um alarme continuo atrapalharia.
  static bool semSinal = false;
  if (micros() - ultimaSubida > 2000000UL) {
    ciclosOk = 0;
    setStatus("Sem oscilacao");
    if (ultimaSubida != 0 && !semSinal) {
      semSinal = true;
      logErro(0, "erro_d1", "sem_oscilacao");
    }
  } else {
    semSinal = false;
  }
}

// ================= D2 =================
// Episodio de erro so fecha depois de 10 amostras boas seguidas (0,5 s):
// um pino alternando sem ler o potenciometro nao abre um episodio novo a cada troca
int boasSeguidasD2 = 0;
void amostraBoaD2() { if (++boasSeguidasD2 >= 10) encerrarEpisodio(); }

int lerPot() {
  long soma = 0;
  for (int i = 0; i < 8; i++) soma += analogRead(PIN_POT);
  return soma / 8;
}

void loopD2() {
  if (millis() - ultimaAmostra < 50) return;
  ultimaAmostra = millis();

  int v = lerPot();
  bool sinal = digitalRead(PIN_SINAL);
  bool dentro = v >= FAIXA_MIN + TOLERANCIA && v <= FAIXA_MAX - TOLERANCIA;
  bool fora   = v <= FAIXA_MIN - TOLERANCIA || v >= FAIXA_MAX + TOLERANCIA;

  if (dentro) {
    if (sinal) { if (okDentro < AMOSTRAS_OK_D2) okDentro++; amostraBoaD2(); }
    else {
      okDentro = okFora = 0;
      boasSeguidasD2 = 0;
      setErro("Na faixa e sinal LOW");
      logErro(1, "erro_d2", "dentro_low");
      feedbackErro();
      return;
    }
  } else if (fora) {
    if (!sinal) { if (okFora < AMOSTRAS_OK_D2) okFora++; amostraBoaD2(); }
    else {
      okDentro = okFora = 0;
      boasSeguidasD2 = 0;
      setErro("Fora da faixa e HIGH");
      logErro(1, "erro_d2", "fora_high");
      feedbackErro();
      return;
    }
  }

  setStatus("Pot " + String(v) + " D" + String(okDentro * 100 / AMOSTRAS_OK_D2) +
            "% F" + String(okFora * 100 / AMOSTRAS_OK_D2) + "%");
  if (okDentro >= AMOSTRAS_OK_D2 && okFora >= AMOSTRAS_OK_D2) concluirEtapa(1);
}

// ================= D3 =================
String cifrar(const String& s) {
  String r = "";
  for (unsigned int i = 0; i < s.length(); i++)
    r += char((s[i] - 'A' + DESLOCAMENTO) % 26 + 'A');
  return r;
}

// Codigo aleatorio de letras maiusculas: impede que a equipe monte uma
// tabela de respostas em vez de implementar a decifragem.
String gerarCodigo() {
  String c = "";
  for (int i = 0; i < TAM_CODIGO; i++) c += char('A' + random(26));
  return c;
}

void enviarNova() {
  codigoAtual = gerarCodigo();
  Serial2.print("MSG:");
  Serial2.println(cifrar(codigoAtual));
  enviadaEm = millis();
}

void processarResposta(String r) {
  r.trim();
  r.toUpperCase();
  if (r.length() == 0) return;

  if (millis() - enviadaEm > PRAZO_RESPOSTA_MS) {
    acertos = 0;
    setErro("Resposta atrasada");
    logErro(2, "erro_d3", "atraso");
    feedbackErro();
    return;
  }
  if (r == codigoAtual) {
    acertos++;
    encerrarEpisodio();
    setStatus("Correto! " + String(acertos) + "/" + String(ACERTOS_D3));
    bip(50);
    if (acertos >= ACERTOS_D3) concluirEtapa(2);
    else enviarNova();
  } else {
    acertos = 0;
    setErro("Recebi: " + r);
    logErro(2, "erro_d3", "esperado=" + codigoAtual + ";recebido=" + r);
    feedbackErro();
  }
}

void loopD3() {
  if (millis() - enviadaEm >= INTERVALO_MSG_MS) enviarNova();
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') { processarResposta(bufferRx); bufferRx = ""; }
    else if (c != '\r' && bufferRx.length() < 32) bufferRx += c;
  }
}

// ================= TRANSICOES =================
void concluirEtapa(int i) {
  if (i == 0) detachInterrupt(digitalPinToInterrupt(PIN_SINAL));
  logEvento("etapa_concluida", "etapa=" + String(i + 1) +
            ";duracao_s=" + String((millis() - inicioEtapa) / 1000) +
            ";episodios_erro=" + String(errosEtapa[i]));
  inicioEtapa = millis();
  encerrarEpisodio();
  feedbackOk();
  moverTrava(i, ANG_ABERTO);
  erroAte = 0;
  dicaAte = 0;

  if (i == 0) {
    estado = D2;
    okDentro = okFora = 0;
    setStatus("Gire o potenciometro");
  } else if (i == 1) {
    estado = D3;
    acertos = 0;
    bufferRx = "";
    while (Serial2.available()) Serial2.read();
    enviarNova();
    setStatus("Aguardando resposta");
  } else {
    estado = VITORIA;
    tempoFinal = restante();
    logEvento("vitoria", "restante_s=" + String(tempoFinal / 1000) + ";dicas=" + String(dicasUsadas));
    melodiaVitoria();
  }
}

void pedirDica() {
  int etapa = estado - D1;
  if (dicasUsadas >= MAX_DICAS)  { setErro("Sem dicas restantes"); return; }
  if (proxDica[etapa] >= 3)      { setErro("Sem dicas nesta etapa"); return; }
  dicaAtual = DICAS[etapa][proxDica[etapa]++];
  dicasUsadas++;
  prazoFinal -= PENALIDADE_DICA;
  dicaAte = millis() + 15000;
  logEvento("dica", String(etapa + 1) + "-" + String(proxDica[etapa]));
  bip(150);
}

void iniciarJogo() {
  inicioJogo = millis();
  prazoFinal = inicioJogo + TEMPO_TOTAL_MS;
  dicasUsadas = 0;
  for (int i = 0; i < 3; i++) proxDica[i] = 0;
  dicaAtual = nullptr;
  dicaAte = 0;
  erroAte = 0;
  for (int i = 0; i < 3; i++) errosEtapa[i] = 0;
  encerrarEpisodio();
  inicioEtapa = inicioJogo;
  if (fsOk) {  // guarda a partida anterior e comeca um log novo
    LittleFS.remove("/anterior.csv");
    if (LittleFS.exists("/partida.csv")) LittleFS.rename("/partida.csv", "/anterior.csv");
  }
  estado = D1;
  iniciarD1();
  setStatus("Aguardando sinal");
  logEvento("inicio");
}

void reiniciar() {
  digitalWrite(PIN_LED_OK, LOW);
  for (int i = 0; i < 3; i++) moverTrava(i, ANG_TRAVADO);
  inicioJogo = 0;
  estado = INTRO;
}

void modoManutencao() {
  for (int i = 0; i < 3; i++) moverTrava(i, ANG_ABERTO);
  tela.clearDisplay();
  tela.setCursor(0, 0);
  tela.println("MANUTENCAO");
  tela.println();
  tela.println("Coloque as pistas");
  tela.println("e feche as portas.");
  tela.println();
  tela.println("Aperte para travar.");
  tela.display();
  while (digitalRead(PIN_BTN_DICA) == LOW) delay(10);
  delay(50);
  while (digitalRead(PIN_BTN_DICA) == HIGH) delay(10);
  delay(50);
  while (digitalRead(PIN_BTN_DICA) == LOW) delay(10);
}

// ================= TELA =================
void desenhar() {
  static uint32_t ultima = 0;
  if (millis() - ultima < 200) return;
  ultima = millis();

  tela.clearDisplay();
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.setCursor(0, 0);

  switch (estado) {
    case INTRO:
      tela.println("ESTACAO DO RIO");
      tela.println();
      tela.println("Sistema bloqueado!");
      tela.println("Restaurem antes");
      tela.println("da cheia chegar.");
      tela.println();
      tela.println("Botao: iniciar");
      break;

    case D1: case D2: case D3: {
      int32_t r = max((int32_t)0, restante());
      tela.printf("Etapa %d/3     %02ld:%02ld\n", estado - D1 + 1,
                  (long)(r / 60000), (long)((r / 1000) % 60));
      tela.printf("Dicas: %d/%d\n", MAX_DICAS - dicasUsadas, MAX_DICAS);
      tela.drawLine(0, 18, 127, 18, SSD1306_WHITE);
      tela.setCursor(0, 22);
      if (dicaAtual && millis() < dicaAte) {
        tela.println("DICA:");
        tela.println(dicaAtual);
      } else {
        tela.println(OBJETIVO[estado - D1]);
        tela.println();
        tela.println(statusTxt);
      }
      break;
    }

    case VITORIA:
      tela.println("SISTEMA RESTAURADO!");
      tela.println();
      tela.printf("Tempo restante %02ld:%02ld\n", (long)(tempoFinal / 60000), (long)((tempoFinal / 1000) % 60));
      tela.printf("Dicas usadas: %d\n", dicasUsadas);
      tela.println();
      tela.println("Segure 5s: reiniciar");
      break;

    case DERROTA:
      tela.println("TEMPO ESGOTADO");
      tela.println();
      tela.println("A cheia chegou...");
      tela.printf("Etapa alcancada: %d/3\n", etapaAlcancada);
      tela.println();
      tela.println("Segure 5s: reiniciar");
      break;
  }
  tela.display();
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, PIN_RX2, PIN_TX2);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_OK, OUTPUT);
  pinMode(PIN_LED_ERR, OUTPUT);
  pinMode(PIN_BTN_DICA, INPUT_PULLUP);
  pinMode(PIN_SINAL, INPUT_PULLDOWN);
  analogReadResolution(12);
  randomSeed(esp_random());
  fsOk = LittleFS.begin(true);  // true: formata na primeira vez
  if (!fsOk) Serial.println("LittleFS indisponivel: log so no Monitor Serial");

  if (!tela.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("OLED nao encontrado");
  tela.setTextColor(SSD1306_WHITE);

  if (digitalRead(PIN_BTN_DICA) == LOW) modoManutencao();
  for (int i = 0; i < 3; i++) moverTrava(i, ANG_TRAVADO);

  estado = INTRO;
  Serial.println("LOG,0,boot,");
  Serial.println("Comandos: L = log atual, A = partida anterior, C = apagar");
}

void loop() {
  comandosSerial();
  int botao = lerBotao();

  switch (estado) {
    case INTRO:
      if (botao == 1) iniciarJogo();
      break;

    case D1: case D2: case D3:
      if (botao == 1) pedirDica();
      if (restante() <= 0) {
        etapaAlcancada = estado - D1 + 1;
        detachInterrupt(digitalPinToInterrupt(PIN_SINAL));
        estado = DERROTA;
        logEvento("derrota", "etapa=" + String(etapaAlcancada) + ";dicas=" + String(dicasUsadas));
        digitalWrite(PIN_LED_ERR, HIGH);
        bip(1500);
        digitalWrite(PIN_LED_ERR, LOW);
        break;
      }
      if (estado == D1) loopD1();
      else if (estado == D2) loopD2();
      else loopD3();
      break;

    case VITORIA:
    case DERROTA:
      if (botao == 2) reiniciar();
      break;
  }

  desenhar();
}
