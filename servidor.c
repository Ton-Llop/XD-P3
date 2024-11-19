#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define MIDA_PAQUET 50

void print_board(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

int check_winner(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return board[i][0] == 'X' ? 1 : 2;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return board[0][i] == 'X' ? 1 : 2;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0] == 'X' ? 1 : 2;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2] == 'X' ? 1 : 2;
    
    // Comprovar empat
    int full = 1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] != 'X' && board[i][j] != 'O')
                full = 0;
    return full ? 3 : 0;
}

int is_valid_move(char board[3][3], int move) {
    int row = (move - 1) / 3;
    int col = (move - 1) % 3;
    return board[row][col] != 'X' && board[row][col] != 'O';
}

int get_random_move(char board[3][3]) {
    int move;
    do {
        move = rand() % 9 + 1;
    } while (!is_valid_move(board, move));
    return move;
}

void send_game_state(int socket, struct sockaddr_in *client_addr, socklen_t client_addr_size, char board[3][3], int status) {
    char paquet[MIDA_PAQUET];
    snprintf(paquet, MIDA_PAQUET, "STATUS %d %c %c %c %c %c %c %c %c %c",
             status,
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    sendto(socket, paquet, MIDA_PAQUET, 0, (struct sockaddr *)client_addr, client_addr_size);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Ús correcte: %s <Port>\n", argv[0]);
        return 1;
    }

    int s;
    struct sockaddr_in socket_servidor, contacte_client;
    socklen_t contacte_client_mida = sizeof(contacte_client);
    char paquet[MIDA_PAQUET];
    char board[3][3] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}};

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    socket_servidor.sin_family = AF_INET;
    socket_servidor.sin_addr.s_addr = INADDR_ANY;
    socket_servidor.sin_port = htons(atoi(argv[1]));

    if (bind(s, (struct sockaddr *)&socket_servidor, sizeof(socket_servidor)) < 0) {
        perror("Error al enllaçar el socket");
        return 1;
    }

    printf("Servidor operatiu al port %d!\n", atoi(argv[1]));
    srand(time(NULL));

    int game_over = 0;
    while (!game_over) {
        recvfrom(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_client, &contacte_client_mida);
        printf("Paquet rebut: %s\n", paquet);

        int move;
        sscanf(paquet, "MOVI %d", &move);

        if (move < 1 || move > 9 || !is_valid_move(board, move)) {
            sprintf(paquet, "Moviment no vàlid o casella ocupada!");
            sendto(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_client, contacte_client_mida);
            continue;
        }

        // Actualitzar tauler amb el moviment del client
        int row = (move - 1) / 3;
        int col = (move - 1) % 3;
        board[row][col] = 'X';

        int status = check_winner(board);
        if (status != 0) {
            send_game_state(s, &contacte_client, contacte_client_mida, board, status);
            game_over = 1;
            break;
        }

        // Moviment del servidor
        int server_move = get_random_move(board);
        row = (server_move - 1) / 3;
        col = (server_move - 1) % 3;
        board[row][col] = 'O';

        status = check_winner(board);
        send_game_state(s, &contacte_client, contacte_client_mida, board, status);
        if (status != 0) game_over = 1;
    }

    close(s);
    return 0;
}

