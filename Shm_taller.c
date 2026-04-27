#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#define ROWS 3
#define COLS 4
void manejador(int sig);

//Tamaño total del segmento
unsigned int sizeof_dm(int rows, int cols, size_t sizeElement) {
    size_t size;
    size  = rows * sizeof(void *);       // indexSize: los punteros M[0]..M[filas-1]
    size += (cols * rows * sizeElement); // datos reales
    return size;
}

//Apuntar cada M[i] a su fila dentro del bloque
void create_index(void **m, int rows, int cols, size_t sizeElement) {
    int i;
    size_t sizeRow = cols * sizeElement;
    m[0] = m + rows;                          // fila 0 empieza justo después del índice
    for (i = 1; i < rows; i++) {
        m[i] = (char *)m[i-1] + sizeRow;     // cada fila = fila anterior + tamaño de una fila
    }
}

void manejador(int sig) {
    // no necesita hacer nada, solo interrumpir el pause()
}

int main() {
    double **matrix = NULL;
    int rows = ROWS, cols = COLS;
    pid_t pid;

    /* ── 1. Calcular tamaño y crear el segmento ── */
    size_t sizeMatrix = sizeof_dm(rows, cols, sizeof(double));

    int shmId = shmget(IPC_PRIVATE, sizeMatrix, IPC_CREAT | 0600);
    if (shmId == -1) { perror("shmget"); exit(1); }

    matrix = (double **) shmat(shmId, NULL, 0);
    if (matrix == (void *) -1) { perror("shmat"); exit(1); }

    create_index((void **)matrix, rows, cols, sizeof(double));

    /* ── 2. fork() ── */
    pid = fork();
    if (pid == -1) { perror("fork"); exit(1); }

    if (pid == 0) {
        /* ─────────────── PROCESO HIJO ─────────────── */
        printf("[HIJO]  PID=%d  |  PID padre=%d\n", getpid(), getppid());

        /* ── 1. Registrar el manejador de la señal ── */
        signal(SIGUSR1, manejador);

        /* ── 2. Esperar a que el padre avise ── */
        printf("[HIJO]  Esperando señal del padre...\n");
        pause();

        /* ── 3. Imprimir la matriz (llegamos aquí después de la señal) ── */
        printf("[HIJO]  Señal recibida! Imprimiendo matriz:\n\n");
        int i, j;
        for (i = 0; i < rows; i++) {
            for (j = 0; j < cols; j++) {
                printf("%8.2f ", matrix[i][j]);
            }
            printf("\n");
        }
    }else {
        /* ─────────────── PROCESO PADRE ─────────────── */
        printf("[PADRE] PID=%d  |  PID hijo=%d\n", getpid(), pid);

        // aquí vendrá: llenar matriz → kill(hijo) → wait()

    }

    /* ── 3. Liberar segmento ── */
    shmdt(matrix);
    shmctl(shmId, IPC_RMID, NULL);

    return 0;
}
