#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define MIDA_PAQUET 50

void print_board(char board[3][3]) {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int is_valid_move(char board[3][3], int move) {
    int row = (move - 1) / 3;
    int col = (move - 1) % 3;
    return board[row][col] != 'X' && board[row][col] != 'O';
}

void update_board(char board[3][3], int move, char player) {
    int row = (move - 1) / 3;
    int col = (move - 1) % 3;
    board[row][col] = player;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Ús correcte: %s <IP> <Port>\n", argv[0]);
        return 1;
    }

    int s;
    struct sockaddr_in contacte_servidor;
    char paquet[MIDA_PAQUET];
    char board[3][3] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}};
    int game_over = 0;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    contacte_servidor.sin_family = AF_INET;
    contacte_servidor.sin_addr.s_addr = inet_addr(argv[1]);
    contacte_servidor.sin_port = htons(atoi(argv[2]));

    while (!game_over) {
        print_board(board);
        int move;
        printf("Jugador (X), selecciona la casella (1-9): ");
        scanf("%d", &move);

        if (move < 1 || move > 9 || !is_valid_move(board, move)) {
            printf("Moviment no vàlid! Intenta-ho de nou.\n");
            continue;
        }

        // Enviar moviment al servidor
        sprintf(paquet, "MOVI %d", move);
        sendto(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_servidor, sizeof(contacte_servidor));

        // Rebre resposta del servidor amb estat del joc
        recvfrom(s, paquet, MIDA_PAQUET, 0, NULL, NULL);
        
        if (strncmp(paquet, "STATUS", 6) == 0) {
            int status;
            sscanf(paquet, "STATUS %d", &status);
            game_over = (status != 0);  // Si status és diferent de 0, la partida ha acabat.

            // Actualitzar el tauler segons la resposta del servidor
            char board_state[9];
            sscanf(paquet, "STATUS %d %c %c %c %c %c %c %c %c %c", &status,
                   &board[0][0], &board[0][1], &board[0][2],
                   &board[1][0], &board[1][1], &board[1][2],
                   &board[2][0], &board[2][1], &board[2][2]);

            print_board(board);

            if (status == 1) printf("Jugador 1 (X) guanya!\n");
            else if (status == 2) printf("Servidor (O) guanya!\n");
            else if (status == 3) printf("Empat!\n");
        } else {
            printf("Resposta inesperada del servidor: %s\n", paquet);
        }
    }

    close(s);
    return 0;
}

