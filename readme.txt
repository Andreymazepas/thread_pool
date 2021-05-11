Trabalho Final de Programação Concorrente
Andrey Emmanuel Matrosov Mazépas - 16/0112362

Para compilar utilizar as flags -lncurses e lpthread

$ gcc main.c -lncurses lpthread
$ ./a.out

Caso não esteja instalada, utilizar o instalador de pacotes para instalar a biblioteca libncurses5

Pool de Threads - Uma visualização de como threads podem realizar uma lista de tarefas sem
passarem na frente umas das outras, atualizando multiplas regiões críticas e desenhando na tela sem
problemas de concorrência.
É possível alterar o número de threads utilizadas e tarefas a serem executadas.
