/*
 * ESCAPE BOX - Gabarito (uso do professor e para testar a caixa)
 * Mude DESAFIO para 1, 2 ou 3 e grave na placa do aluno.
 */

#define DESAFIO 1

const int PINO_SINAL = 23;
const int PINO_POT   = 34;
const int FAIXA_MIN  = 2000;
const int FAIXA_MAX  = 2800;
const int DESLOC     = 7;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(PINO_SINAL, OUTPUT);
}

void loop() {
#if DESAFIO == 1
  // 2 Hz: periodo de 500 ms
  digitalWrite(PINO_SINAL, HIGH);
  delay(250);
  digitalWrite(PINO_SINAL, LOW);
  delay(250);

#elif DESAFIO == 2
  int v = analogRead(PINO_POT);
  if (v >= FAIXA_MIN && v <= FAIXA_MAX) digitalWrite(PINO_SINAL, HIGH);
  else digitalWrite(PINO_SINAL, LOW);
  Serial.println(v);   // util para calibrar contra a leitura do mestre
  delay(10);

#elif DESAFIO == 3
  if (Serial2.available()) {
    String linha = Serial2.readStringUntil('\n');
    linha.trim();
    if (linha.startsWith("MSG:")) {
      String cifra = linha.substring(4);
      String resposta = "";
      for (unsigned int i = 0; i < cifra.length(); i++) {
        resposta += char((cifra[i] - 'A' - DESLOC + 26) % 26 + 'A');
      }
      Serial2.println(resposta);
      Serial.println(cifra + " -> " + resposta);
    }
  }
#endif
}
