# Casos de teste na simulação

Execução no projeto do Wokwi (https://wokwi.com/projects/480636863090165761) com a versão final do firmware, em 15 nov. 2026. Cada caso rodou uma vez nesta versão. Os casos foram feitos em três etapas: uma partida vencida (log em `log_partida_vitoria.csv`), uma partida perdida com o tempo reduzido (`log_partida_derrota.csv`) e um reinício com o botão pressionado, para o TF01.

Os cenários 1, 4 e 6 da placa da equipe rodam o código do `gabarito.ino`. Os outros cenários simulam erros de propósito: pino como entrada (0), 1 Hz (2), pino fixo em nível alto (3), pino alternando sem ler o potenciômetro (5), resposta fixa (7) e resposta atrasada em 3,5 s (8).

| ID | Caso | Esperado | Obtido |
|---|---|---|---|
| TF01 | Ligar com o botão pressionado | Abre as três travas; novo clique trava | Conforme: as três travas abriram (87°) e o display mostrou MANUTENCAO; o clique seguinte as fechou (0°) |
| TF02 | Gabarito do desafio 1 | Etapa 1 concluída após cerca de 3 s | Conforme: etapa concluída 3,5 s após o início do sinal |
| TF03 | Piscar a 1 Hz ou manter o pino fixo em nível alto | Etapa 1 não conclui; feedback de erro; um episódio no log | Conforme: o pino fixo gerou `erro_d1,sem_oscilacao` e a tela indicou "Sem oscilacao", sem bip; o 1 Hz acendeu o LED vermelho e foi registrado como continuação do mesmo episódio (`episodios_erro=1`) |
| TF04 | Gabarito do desafio 2, girando o potenciômetro | Etapa 2 concluída | Conforme: etapa concluída com 20 amostras corretas fora da faixa e 20 dentro |
| TF05 | Pino fixo em nível alto no desafio 2 | Progresso zerado; feedback de erro | Conforme: `erro_d2,fora_high`; progresso zerado |
| TF06 | Gabarito do desafio 3 | Três acertos seguidos; tampa abre | Conforme: três respostas decifradas em sequência (por exemplo JURLT, respondida CNKEM); tampa aberta e vitória registrada |
| TF07 | Resposta errada ou após 3 s | Contador de acertos zerado; erro com código esperado e recebido no log | Conforme: resposta errada registrada com os códigos esperado e recebido; resposta após 3,5 s registrada como atraso |
| TF08 | Pedir dica | Exibe a dica e desconta 3 minutos | Conforme: evento `dica,1-1`; ao final, `restante_s=2455`, que corresponde a 2700 s menos 64,9 s de partida e 180 s de penalidade |
| TF09 | Esgotar o tempo, com tempo reduzido para o teste | Tela de derrota e evento no log | Conforme: evento `derrota` 10 s após o comando T |
| TF10 | Segurar o botão 5 s na tela final | Travas fecham; volta à introdução | Conforme: travas em 0° e tela de introdução |
| TA01 | Pino de sinal como entrada, sem código | Etapa 1 não avança | Conforme: etapa 1 não avançou e nada foi registrado no log |
| TA03 | Desafio 2 com o pino alternando rápido, sem ler o potenciômetro | Progresso zera; um episódio de erro por vez no log | Conforme: nenhum episódio novo; só registros de continuação a cada 5 s |
| TA04 | Desafio 3 respondendo sempre o mesmo texto | Nenhum acerto registrado | Conforme: nenhum acerto em 11 s |
| TA05 | Após a partida, enviar o comando de leitura do log | Log completo da partida impresso | Conforme: `L` imprimiu a partida atual e `A`, a anterior, com as 16 linhas da partida vencida |

O caso TA02 depende do hardware real e fica para o protótipo físico.
