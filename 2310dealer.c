#include "2310dealer.h"

/** Sets up pipes.
 *
 * @param sendPipe send fd's
 * @param receivePipe receive fd's
 * @param programName The child's programs' name
 * @param numPlayers the number of players
 * @param playerID the player's ID
 */
void child_setup(int sendPipe[2], int receivePipe[2], char *programName,
        char *numPlayers, char *playerID) {
    // write 2310A's stdout into pipe
    dup2(sendPipe[WRITE_END], STDOUT_FILENO);
    close(sendPipe[WRITE_END]);
    close(sendPipe[READ_END]);

    // 2310A reads from pipe2
    dup2(receivePipe[READ_END], STDIN_FILENO);
    close(receivePipe[READ_END]);
    close(receivePipe[WRITE_END]);

    // suppress stderr
    int sink = fileno(fopen("/dev/null", "w"));
    dup2(sink, STDERR_FILENO);

    execl(programName, programName, numPlayers, playerID, NULL);

    // if we get here, exec failed.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
//    exit(0);
    kill(getppid(), SIGABRT);
}

/** Closes pipes and creates reader/writers. **/
void create_reader_writers(Player *player) {
    close(player->sendPipes[WRITE_END]);
    close(player->receivePipes[READ_END]);

    // write to this to talk to player
    player->writer = fdopen(player->receivePipes[WRITE_END], "w");

    // read from this to read from player
    player->reader = fdopen(player->sendPipes[READ_END], "r");
}

/** Checks the stream for a carat. **/
int check_carat(Player *player, char *givenPath) {
    char carat = fgetc(player->reader);
    if (carat == '^') {
        fprintf(player->writer, "%s\n", givenPath);
        fflush(player->writer);
        return 0;
    }
    dealer_exit(ERROR_STARTING_PLAYER);
    return 1;
}

/** Checks if the deck file is valid. **/
DealerErrorCode check_deck_file(char *deckFileName) {
    char *line = read_line(fopen(deckFileName, "r"));

    // get number of cards
    char *trash;
    char buff[5];
    snprintf(buff, 5, "%s", &line[0]);
    int numberOfCards = (int) strtol(buff, &trash, 10);

    if (numberOfCards < 4) {
        dealer_exit(INVALID_DECK_FILE);
    }
    char *validChars = "ABCDE";

    char str[2] = "\0";
    for (int i = 1; i < strlen(line); ++i) {
        str[0] = line[i];
        if (strstr(validChars, str) == NULL) {
            dealer_exit(INVALID_DECK_FILE);
        }
        if (i > numberOfCards) {
            dealer_exit(INVALID_DECK_FILE);
        }
    }

    return NO_ERROR;
}

/** Checks the dealer's args.
 *
 * @param argc
 * @param argv
 * @return NO_ERROR on success
 */
DealerErrorCode check_dealer_args(int argc, char **argv) {
    char *deckFileName = argv[1];
    char *mapFileName = argv[2];

    // 9 players max + deck + path + program name
    if (argc > 3 + 9) {
        dealer_exit(INVALID_NUM_ARGS);
    }

    if (argc < 4) {
        dealer_exit(INVALID_NUM_ARGS);
    }
    FILE *file = fopen(mapFileName, "r");
    check_deck_file(deckFileName);
    check_map_file(read_line(file), DEALER);

    if (fgetc(file) == '\n') {
        dealer_exit(INVALID_PATH_FILE);
    }

    return NO_ERROR;
}

/** Sets up all the forks.
 *
 * @param numPlayers Number of players in game
 * @param path The game path
 * @param argv argv
 */
void make_forks(int numPlayers, Path *path, char **argv) {
    int cPID = fork();
    char childArgsNumPlayers[2];
    char childArgsID[2];

    snprintf(childArgsNumPlayers, 2, "%d", numPlayers);

    // if child
    if (cPID == 0) {
        for (int i = 1; i < numPlayers; ++i) {
            // we are another clone
            if (fork() == 0) {
                snprintf(childArgsID, 2, "%d", i);
                Player *player = path->players[i];
                child_setup(player->sendPipes, player->receivePipes,
                        argv[i + 3], childArgsNumPlayers, childArgsID);
            }
        }
        Player *player = path->players[0];
        child_setup(player->sendPipes, player->receivePipes, argv[3],
                childArgsNumPlayers, "0");
    }

    for (int i = 0; i < numPlayers; ++i) {
        create_reader_writers(path->players[i]);
    }
}

/** Initialises the dealer.
 *
 * @param givenPath The contents of the pathfile
 * @param numPlayers The number of players
 * @param deckFileName The deck file name
 * @return an initialised Path
 */
Path *initialise_dealer(char *givenPath, int numPlayers, char *deckFileName) {
    // do initialisation
    Path *path = allocate_path(givenPath, numPlayers);
    path->players = malloc(sizeof(Player *) * numPlayers);
    path->deck = allocate_deck(fopen(deckFileName, "r"));

    for (int i = 0; i < numPlayers; ++i) {
        path->players[i] = init_player(path, i);
        // make pipes

        pipe(path->players[i]->sendPipes);
        pipe(path->players[i]->receivePipes);
    }
    arrange_order_of_players(path);

    return path;
}


/** Dealer program. **/
int main(int argc, char **argv) {
    if ((intptr_t) signal(SIGABRT, sig_handler) == SIGABRT) {
        dealer_exit(ERROR_STARTING_PLAYER);
    }
    if ((intptr_t) signal(SIGHUP, sig_handler) == SIGHUP) {
        dealer_exit(COMMUNICATION_ERROR);
    }
    // arg checking
    check_dealer_args(argc, argv);

    char *deckFileName = argv[1];
    char *mapFileName = argv[2];

    // first three args are not players
    int numPlayers = argc - 3;
    char *givenPath = read_line(fopen(mapFileName, "r"));
    Path *path = initialise_dealer(givenPath, numPlayers, deckFileName);

    make_forks(numPlayers, path, argv);

    start_dealer(path, givenPath);

    return 0;
}

/** Sends HAPs to players and prints to dealer stdout. **/
void send_to_player(Path *path, Player *player, int oldPoints, int oldMoney,
        int cardIndexHap) {
    int playerId = player->id;
    int newPoints = player->points - oldPoints;
    int newMoney = player->money - oldMoney;
    print_player_details(path, playerId, stdout);
    fflush(stdout);

    // send haps to players
    for (int j = 0; j < path->playersInGame; ++j) {
        fprintf(path->players[j]->writer, "HAP%d,%d,%d,%d,%d\n",
                playerId, player->siteNumber, newPoints, newMoney,
                cardIndexHap);
        fflush(path->players[j]->writer);
    }
}

/** Checks if all player have sent their carats. **/
void check_players_carats(Path *path, char *givenPath) {
    for (int playerID = 0; playerID < path->playersInGame; ++playerID) {
        if (check_carat(path->players[playerID], givenPath) == 1) {
            exit(9);
        }
    }
}

/** Prints the endgame and sends endgame to players.
 *
 * @param path The game path
 */
void clean_up_game(Path *path) {
    game_over(path, stdout);

    // send DONE to players
    for (int j = 0; j < path->playersInGame; ++j) {
        fprintf(path->players[j]->writer, "DONE\n");
        fflush(path->players[j]->writer);
    }
}

/** Starts a dealer session.
 *
 * @param path The game path
 * @param givenPath The contents of the path file
 * @return 0 when game has ended
 */
int start_dealer(Path *path, char *givenPath) {
    check_players_carats(path, givenPath);
    print_path(path, stdout);

    // main loop
    while (check_game_over(path) != 0) {
        int nextMovePlayerID = next_player_to_move(path);
        fprintf(path->players[nextMovePlayerID]->writer, "YT\n");
        fflush(path->players[nextMovePlayerID]->writer);
        // wait for dealer input
        char *input = read_line(path->players[nextMovePlayerID]->reader);

        int playerID = next_player_to_move(path);

        // recieved DO
        char read[5];
        char *ptr;
        snprintf(read, 3, "%s", &input[0]);

        // find sitenumber
        if (strcmp("DO", read) == 0) {
            snprintf(read, 5, "%s", &input[2]);
            int siteNumber = (int) strtol(read, &ptr, 10);

            Player *player = path->players[playerID];

            // for HAP
            int oldPoints = player->points;
            int oldMoney = player->money;

            move_player(path, playerID, siteNumber - player->siteNumber,
                    DEALER);

            Site *site = &path->sites[player->siteNumber];

            // do Ri type
            int cardIndexHAP = 0;
            if (strcmp(site->type, RI) == 0) {
                cardIndexHAP = do_ri(path->deck, player);
            }
            send_to_player(path, player, oldPoints, oldMoney, cardIndexHAP);
        }
    }
    clean_up_game(path);
    return 0;
}
