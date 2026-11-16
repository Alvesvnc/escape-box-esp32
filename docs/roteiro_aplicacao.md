# Roteiro de aplicação

Este roteiro é para o professor que vai aplicar a Escape Box em sala. Ele cobre a preparação da caixa, a conversa inicial com a turma, o que fazer durante a partida, a conversa final e o reinício para a próxima equipe. A aula toda cabe em 60 a 90 minutos: uns 10 minutos de preparação e conversa inicial, 45 minutos de partida e 15 a 20 minutos de conversa final.

## Antes da aula

1. Grave o `mestre/mestre.ino` no ESP32 da caixa e o `aluno_modelo/aluno_modelo.ino` na placa da equipe. O modelo já vem com a comunicação do desafio 3 pronta; a equipe só completa o que falta.
2. Teste a caixa com o gabarito. Grave o `gabarito/gabarito.ino` na placa da equipe com `DESAFIO 1`, jogue uma partida até o fim, depois troque para `DESAFIO 2` e `DESAFIO 3`. Se as três etapas abrirem, a caixa está pronta. Depois grave de novo o modelo do aluno.
3. Prepare o notebook da equipe com o Arduino IDE, o suporte ao ESP32 instalado e o modelo do aluno aberto. Deixe o cabo USB ligado na placa da equipe e confira se a placa aparece na porta certa.
4. Imprima os cartões e coloque cada um no seu lugar, usando o modo de manutenção (veja "Reinício entre equipes").

## Cartões

O cartão inicial fica fora da caixa, junto do notebook. Ele traz a ligação da placa da equipe com a caixa: sinal no pino 23, potenciômetro no pino 34 e comunicação pela Serial2, com recepção no pino 16 e envio no 17. Também lembra que o Monitor Serial a 115200 serve para testar.

O cartão do compartimento 1 traz a faixa de alerta do sensor: a leitura do potenciômetro precisa ficar entre 2000 e 2800 para o sinal ficar ligado, e fora dela o sinal precisa ficar desligado.

O cartão do compartimento 2 traz a chave da cifra: cada letra da mensagem foi avançada 7 posições no alfabeto, e depois do Z volta para o A.

A tampa final guarda o que a turma combinou como recompensa. Pode ser só uma mensagem de "estação restaurada".

## Conversa inicial (5 minutos)

Conte a história: a estação que monitora o nível do rio parou, e a equipe tem 45 minutos para restaurá-la antes da cheia. Explique as regras em poucas frases.

1. O objetivo de cada etapa aparece no display da caixa.
2. A equipe só mexe na placa de fora. A caixa não deve ser aberta nem desmontada.
3. O botão azul começa a partida e depois serve para pedir dica. São até três dicas, e cada uma tira 3 minutos do relógio.
4. Quando o código está certo, a caixa abre um compartimento sozinha. O LED vermelho e o bip avisam quando a tentativa não passou.

Pergunte se alguém tem dúvida e só então aperte o botão para começar.

## Durante a partida

Seu papel é observar. Evite dar a resposta, mesmo quando a equipe travar; para isso existe o botão de dica, e o custo em tempo faz parte do jogo. Se a equipe ficar parada por um problema que não é do conteúdo, como a porta USB errada ou o Arduino IDE travado, ajude sem descontar tempo.

Vale anotar em que ponto cada equipe travou e o que disseram na hora. Essas anotações ajudam muito na conversa final. A caixa também registra a partida sozinha, com o tempo de cada etapa, os erros e as dicas.

Se o tempo acabar, a caixa mostra a tela de derrota e para de aceitar tentativas.

## Conversa final (15 a 20 minutos)

Comece pelo resultado: tempo restante ou etapa alcançada, e quantas dicas foram usadas. Depois passe pelas três etapas, ligando cada uma ao conceito.

1. Desafio 1: o que significa piscar a 2 Hz? Como chegaram em 250 ms ligado e 250 ms desligado? O que aconteceria com um pino sempre ligado?
2. Desafio 2: o que o `analogRead` devolve? Por que a caixa testa a leitura dentro e fora da faixa, e não só dentro?
3. Desafio 3: como a mensagem chega pela UART? Como desfizeram a cifra letra por letra? Por que a caixa sorteia um código novo a cada mensagem?

Termine perguntando o que foi mais difícil e o que mudariam no jogo. Se a turma for jogar de novo outro dia, isso ajuda a ajustar as dicas.

## Recuperar o log da partida

Abrir o Monitor Serial reinicia o ESP32 da caixa, por isso o log fica guardado na memória dela. Depois da partida, ligue a caixa no notebook, abra o Monitor Serial a 115200 e envie uma letra:

1. `L` mostra a partida atual.
2. `A` mostra a partida anterior.
3. `C` apaga as duas.

O formato de cada linha e o significado dos eventos estão no README, na seção sobre o log.

## Reinício entre equipes

Use esta lista toda vez que a caixa voltar de uma partida.

1. Na tela final, segure o botão por 5 segundos. As travas fecham e a caixa volta para a introdução.
2. Para recolocar os cartões, desligue a caixa, ligue de novo com o botão pressionado e solte. As três travas abrem e o display mostra "MANUTENCAO".
3. Coloque o cartão da faixa no compartimento 1, o da cifra no compartimento 2 e a recompensa na tampa final.
4. Feche as portas e aperte o botão uma vez. As travas fecham.
5. Grave de novo o `aluno_modelo.ino` na placa da equipe, para a próxima equipe não começar com o código da anterior.
6. Se quiser guardar o log da equipe que acabou de jogar, recupere antes da próxima partida. Ao começar uma partida nova, a anterior passa para `A` e a mais antiga é apagada.
