/*
 * ESCAPE BOX - Placa do aluno
 *
 * Ligacoes com a caixa (ja estao conectadas):
 *   Pino de sinal ......... GPIO 23
 *   Potenciometro ......... GPIO 34
 *   Comunicacao (UART) .... Serial2 (RX 16, TX 17)
 *
 * Use Serial (Monitor Serial, 115200) para testar e depurar.
 * Use Serial2 somente para conversar com a caixa.
 *
 * Desafio 3: a leitura e o envio pela UART ja estao prontos neste modelo.
 * Voces so precisam completar a funcao decifrar() e chamar
 * responderMensagens() dentro do loop().
 */

const int PINO_SINAL = 23;
const int PINO_POT   = 34;

// ---------- Desafio 3: completem esta funcao ----------
// Recebe o texto cifrado (letras maiusculas) e devolve o texto original.
String decifrar(String cifrado) {
  String original = "";

  // Escrevam aqui a decifragem, letra por letra

  return original;
}

// ---------- Pronto: le a mensagem da caixa e responde ----------
// A caixa envia linhas no formato MSG:XXXXX
void responderMensagens() {
  if (Serial2.available()) {
    String linha = Serial2.readStringUntil('\n');
    linha.trim();
    if (linha.startsWith("MSG:")) {
      String cifrado = linha.substring(4);
      String resposta = decifrar(cifrado);
      Serial2.println(resposta);
      Serial.println("Recebi " + cifrado + ", respondi " + resposta);
    }
  }
}

void setup() {
  Serial.begin(115200);                      // Monitor Serial
  Serial2.begin(9600, SERIAL_8N1, 16, 17);   // Comunicacao com a caixa

  // Escrevam aqui a configuracao dos pinos

}

void loop() {
  // Escrevam aqui o codigo de cada desafio
  // No desafio 3, descomentem a linha abaixo:
  // responderMensagens();

}
