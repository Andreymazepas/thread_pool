/*
Trabalho Final de Programação Concorrente
Andrey Emmanuel Matrosov Mazépas - 16/0112362
Pool de Threads - Uma visualização de como threads podem realizar uma lista de tarefas sem
passarem na frente umas das outras, atualizando multiplas regiões críticas e desenhando na tela sem
problemas de concorrência.
É possível alterar o número de threads utilizadas e tarefas a serem executadas.
*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a, b) ((a) < (b) ? a : b)

// definições do número de threads, tasks e tamanho das janelas
#define THREADS 8
#define SECOND 1000000
#define NUM_TASKS 100
#define TASKWINDOW_STARTX 0
#define TASKWINDOW_STARTY 0
#define TASKWINDOW_WIDTH 16
#define TASKWINDOW_HEIGHT 26
#define THREADWINDOW_STARTX 18
#define THREADWINDOW_STARTY 0
#define THREADWINDOW_WIDTH 15
#define THREADWINDOW_HEIGHT 13


// estrutura da tarefa com tempo para ser concluida e id
typedef struct Task
{
    int id;
    int timeToComplete;
} Task;

Task taskQueue[256];            // fila de tarefas a serem realizadas
Task completeTasksQueue[256];   // fila de tarefas concluídas
int completeTaskCount = 0;      // contador de tarefas concluidas
int taskCount = 0;              // contador de tarefas a serem concluidas
int screenReady = 0;            // variável não crítica, apenas indica a inicialização da tela

pthread_mutex_t mutexQueue;     // lock da fila de tarefas
pthread_mutex_t mutexCompleteQueue; // lock da fila de tarefas concluidas
pthread_mutex_t mutexScreen;        // lock para alterar o estado da tela

WINDOW *create_newwin(int height, int width, int starty, int startx, const char *title);
void executeTask(Task *task);
void *worker(void *args);
void *screenThread(void *args);

// Função de execução da tarefa, apenas dorme a thread pelo tempo informado
void executeTask(Task *task)
{
    usleep(task->timeToComplete * 1000);
}


// Função das threads que irão executar as tarefas
// cada thread pega uma única tarefa da lista, desenha e mantém uma janela com informação
// ao realizar a tarefa, insere na fila de tarefas concluídas
void *worker(void *args)
{
    WINDOW *threadWindow;               // Janela da thread
    int id = (intptr_t)args;            // Cast do id através dos args
    int totalTasks = 0;                 // Total de tasks realizadas por essa thread
    char title[100];
    snprintf(title, 100, "Thread %d", id);  // Criando uma string com o id da thread
    while (!screenReady)                    // Evitando que as threads tentem desenhar na tela antes da inicialização.
        ;                                   // como as threads apenas leem, e apenas a thread da tela pode alterar uma vez, não há necessidade de um mutex
    while (taskCount)                       // O loop principal deve rodar apenas enquanto há tarefas a serem realizadas
    {
        pthread_mutex_lock(&mutexScreen);   // Lock para alterar o estado da tela
        wclear(threadWindow);               // Limpa apenas a janela da thread
        threadWindow = create_newwin(       // Redesenha a tela
            THREADWINDOW_HEIGHT,
            THREADWINDOW_WIDTH,
            THREADWINDOW_STARTY + ((id / 4) * THREADWINDOW_HEIGHT),
            (1 + id - (id / 4) * 4) * THREADWINDOW_STARTX,
            title);
        mvwprintw(threadWindow, 9, 1, "Total: %d", totalTasks);
        pthread_mutex_unlock(&mutexScreen);

        Task task;
        pthread_mutex_lock(&mutexQueue);    // Lock para alterar a fila de tarefas
        if(!taskCount){
            break;                          // Caso outra thread tenha finalizado as tarefas antes de conseguir o lock
        }                                   // sair do loop e finalizar a thread
        task = taskQueue[0];                // Pega a primeira tarefa da fila
        int i;
        for (i = 0; i < taskCount - 1; i++)
        {
            taskQueue[i] = taskQueue[i + 1];    // Shift das tarefas
        }
        taskCount--;                            // Reduz a contagem de tasks e devolve o lock
        pthread_mutex_unlock(&mutexQueue);

        pthread_mutex_lock(&mutexScreen);       // Novamente pega o lock da tela para atualizar o status
        mvwprintw(threadWindow, 2, 1, "Got task %d", task.id);
        mvwprintw(threadWindow, 4, 1, "Executing...");
        wrefresh(threadWindow);
        pthread_mutex_unlock(&mutexScreen);

        executeTask(&task);                      // Executa a tarefa

        pthread_mutex_lock(&mutexScreen);        // Atualiza novamente a tela após concluir a tarefa
        mvwprintw(threadWindow, 6, 1, "Done!");
        mvwprintw(threadWindow, 7, 1, "Took %dms", task.timeToComplete);
        totalTasks++;
        mvwprintw(threadWindow, 9, 1, "Total: %d", totalTasks);
        wrefresh(threadWindow);
        pthread_mutex_unlock(&mutexScreen);

        usleep(1 * SECOND);

        pthread_mutex_lock(&mutexCompleteQueue);    // Pega o lock da fila de tarefas completas
        completeTasksQueue[completeTaskCount] = task;   // e insere na primeira posição disponivel
        completeTaskCount++;
        pthread_mutex_unlock(&mutexCompleteQueue);
    }
    pthread_exit(NULL);
}

// função auxiliadora para criar janelas com borda e titulo
WINDOW *create_newwin(int height, int width, int starty, int startx, const char *title)
{
    WINDOW *local_win;

    local_win = newwin(height - 1, width, starty + 1, startx);
    box(local_win, 0, 0);   // borda da janela
    mvwprintw(local_win, 0, 2, title);  // titulo da janela na borda superior
    wrefresh(local_win);

    return local_win;
}


// Função da Thread responsável por inicializar a tela e desenhar as duas filas
void *screenThread(void *args)
{
    initscr();                              // Inicializa a tela
    clear();                                // Limpa o conteúdo do terminal
    refresh();                              // refresh inicial

    screenReady = 1;                        // Seta a variável permitindo que outras threads possam utilizar a tela
    WINDOW *taskWindow, *completeWindow;    // Janelas das filas

    while (completeTaskCount != NUM_TASKS)  // Continuar desenhando até que todas as tarefas estejam concluidas
    {
        pthread_mutex_lock(&mutexScreen);   // Pega o lock para alterar o estado da tela
        wclear(taskWindow);                 // Limpa as janelas no inicio do loop
        wclear(completeWindow);

        taskWindow = create_newwin(TASKWINDOW_HEIGHT, TASKWINDOW_WIDTH, TASKWINDOW_STARTY, TASKWINDOW_STARTX, "Tasks");
        completeWindow = create_newwin(TASKWINDOW_HEIGHT, TASKWINDOW_WIDTH, TASKWINDOW_STARTY, MIN((1 + THREADS), 5) * THREADWINDOW_STARTX, "Done");
        
        // Loop para desenhar as tarefas a serem completadas
        // O lock é utilizado para evitar que outra thread possa pegar uma tarefa
        // enquanto esta thread está lendo e desenhando
        pthread_mutex_lock(&mutexQueue);
            for (int i = 0; i < MIN(TASKWINDOW_HEIGHT - 3, taskCount); i++)
            {
                mvwprintw(taskWindow, 1 + i, 1, "Task %d", taskQueue[i].id); 
            }
        pthread_mutex_unlock(&mutexQueue);

        // O mesmo que o anterior mas para as tarefas concluídas
        // Evitando que tarefas sejam inseridas durante a renderização
        // O loop garante que irá renderizar apenas as ultimas inseridas que couberem na janela
        pthread_mutex_lock(&mutexCompleteQueue);
            for (
                int i = MAX(0, completeTaskCount - TASKWINDOW_HEIGHT - 2);
                i < MIN(MAX(0, completeTaskCount - TASKWINDOW_HEIGHT - 2) + TASKWINDOW_HEIGHT - 3, completeTaskCount);
                i++)
            {
                mvwprintw(
                    completeWindow,
                    1 + i - MAX(0, completeTaskCount - TASKWINDOW_HEIGHT - 2),
                    1,
                    "Task %d", completeTasksQueue[i].id);
            }
        pthread_mutex_unlock(&mutexCompleteQueue);

        // atualiza as janelas
        wrefresh(taskWindow);
        wrefresh(completeWindow);
        pthread_mutex_unlock(&mutexScreen); // Devolve o lock da tela
        usleep(160000);                     // Sleep até renderizar a tela novamente
    }
    // Ao sair do while (quando todas as outras threads finalizarem), aguardar uma tecla ser pressionada e fechar
    mvprintw(-4 + 30 + MAX((THREADS- 8+3 ) /4, 0)*THREADWINDOW_HEIGHT, 1,"Press anything to close..." );
    getch();
    endwin();
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // Inicialização das variáveis
    pthread_t th[THREADS + 1];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_mutex_init(&mutexCompleteQueue, NULL);
    pthread_mutex_init(&mutexScreen, NULL);

    srand(time(NULL));
    int i;
    int *id;

    // Gerando as tarefas com tempos aleatórios para serem concluídas
    pthread_mutex_lock(&mutexQueue);
        for (i = 0; i < NUM_TASKS; i++)
        {
            Task t = {
                .id = i,
                .timeToComplete = rand() % 5000
            };
            taskQueue[taskCount] = t;       // Inserindo na fila de tarefas
            taskCount++;                    // Incrementando o contador
        }
    pthread_mutex_unlock(&mutexQueue);

    // Criação das threads que irão realizar as tarefas
    for (i = 0; i < THREADS; i++)
    {
        id = (int *)malloc(sizeof(int));
        *id = i;
        if (pthread_create(&th[i], NULL, &worker, (void *)(intptr_t)(i)) != 0)
        {
            perror("Failed to create the thread");
        }
    }
    pthread_create(&th[THREADS], NULL, &screenThread, NULL);       // Thread responsável pela renderização das janelas de filas 

    for (i = 0; i < THREADS + 1; i++)
    {
        if (pthread_join(th[i], NULL) != 0)
        {
            perror("Failed to join the thread");
        }
    }

    return 0;
}