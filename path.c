#include "path.h"


/** Checks the player's args.
 *
 * @param argc argc
 * @param argv argv
 * @return 0 on success.
 */
int check_player_args(int argc, char **argv) {
    if (argc != 3) {
        player_exit(INVALID_NUM_ARGS_PLAYER);
    }

    char *trash;

    char *validIDs = "0123456789";

    // check number of sites is more than 2
    int playerCount = (int) strtol(argv[1], &trash, 10);
    int playerID = (int) strtol(argv[2], &trash, 10);

    if (strstr(validIDs, argv[2]) == NULL || strlen(argv[2]) == 0) {
        player_exit(INVALID_ID);
    }

    if (playerCount < 1) {
        player_exit(INVALID_PLAYER_COUNT);
    }
    if (playerID < 0 || playerID >= playerCount) {
        player_exit(INVALID_ID);
    }

    return 0;
}

/** Allocates the path from the given string.
 *
 * @param line The contents of the path file
 * @param numPlayers The number of players in the game
 * @return The game path
 */
Path *allocate_path(char *line, int numPlayers) {
    Path *path = malloc(sizeof(Path));

    // get number of stops
    // following is to find how many stops we have
    char temp[] = ";";
    char *trash;
    int i = (int) strcspn(line, temp);

    char numOfSites[5];
    snprintf(numOfSites, i + 1, "%s", &line[0]);

    int numberOfStops = (int) strtol(numOfSites, &trash, 10);
    path->size = numberOfStops;

    // make a list of sites with length of the number of stops
    Site *sites = malloc(sizeof(Site) * path->size);

    for (int siteNum = 0; siteNum < numberOfStops; ++siteNum) {
        // break up into each site

        // already checked first few chars
        char string[4];
        snprintf(string, 4, "%s", &line[strlen(numOfSites) + 1 + 3 * siteNum]);

        // get site type and its value

        // 4th is a number
        char type[3];
        snprintf(type, 3, "%s", string);
        sites[siteNum].type = malloc(sizeof(char) * 3);
        strcpy(sites[siteNum].type, type);

        if (strcmp(sites[siteNum].type, "::") == 0) {
            sites[siteNum].playerLimit = numPlayers;
        } else {
            sites[siteNum].playerLimit = string[2] - '0';
        }
    }

    path->sites = sites;
    path->playersInGame = 0;
    return path;
}

/** Prints the player details to stdout.
 *
 * @param path The game path
 * @param playerId The playerID
 * @param location The location to display to
 */
void print_player_details(Path *path, int playerId, FILE *location) {
//    Cards cards;
    Player *player = path->players[playerId];
    fprintf(location, "Player %d Money=%d V1=%d V2=%d Points=%d A=%d"
            " B=%d C=%d "
            "D=%d E=%d\n", playerId, player->money, player->countV1,
            player->countV2, player->points, player->cards[A],
            player->cards[B], player->cards[C], player->cards[D],
            player->cards[E]);
    print_path(path, location);
}

/** Exits with a path error for the given process.
 *
 * @param processType The process type; either player or dealer
 */
void path_exit(ProcessType processType) {
    if (processType == DEALER) {
        dealer_exit(INVALID_PATH_FILE);
    }
    player_exit(PATH_ERROR);
}

/** Checks if maps args are valid.
 *
 * @param numberOfStops The number of stops on the path
 * @param processType The process type, either player or dealer
 * @param line The contents of the map file
 * @param numOfSites The number of sites
 */
void check_map_args(int numberOfStops, ProcessType processType, char *line,
        char *numOfSites) {
    if (numberOfStops < 2) {
        path_exit(processType);
    }

    // check file length is correct
    if (strlen(line) != 3 * numberOfStops + 1 + strlen(numOfSites)) {
        path_exit(processType);
    }
}


/** Checks if the map file is valid.
 *
 * @param line The contents of the map file
 * @param processType The type of the process, either player or dealer
 * @return NO_ERROR on success
 */
int check_map_file(char *line, ProcessType processType) {
    // get number of stops
    char temp[] = ";";
    char *trash;
    int i = (int) strcspn(line, temp);
    char numOfSites[5];
    snprintf(numOfSites, i + 1, "%s", &line[0]);
    // check number of sites is more than 2
    int numberOfStops = (int) strtol(numOfSites, &trash, 10);
    check_map_args(numberOfStops, processType, line, numOfSites);
    char *validTypes = "Mo V1 V2 Do Ri ::";
    char *validCapacity = "123456789";
    // check each site
    for (int siteNum = 0; siteNum < numberOfStops; ++siteNum) {
        // get entire site
        char site[4];
        snprintf(site, 4, "%s", &line[strlen(numOfSites) + 1 + 3 * siteNum]);
        // get site characters
        char type[3];
        snprintf(type, 3, "%s", site);
        // if not valid type
        if (strstr(validTypes, type) == NULL) {
            path_exit(processType);
        }
        // checking capacities
        if (strcmp(type, "::") == 0) {
            // needs to be -
            char dash[2];
            snprintf(dash, 2, "%s", &site[2]);

            if (strcmp(dash, "-") != 0) {
                path_exit(processType);
            }
        } else {
            // is normal site
            char str[2] = "\0";
            str[0] = site[2];
            if (strstr(validCapacity, str) == NULL) {
                path_exit(processType);
            }
        }
        // first and last site need to be barriers
        if (siteNum == 0 || siteNum == numberOfStops - 1) {
            if (strcmp(type, "::") != 0) {
                path_exit(processType);
            }
        }
    }
    return NO_ERROR;
}

/** Matches a comma in line at the given location.
 *
 * @param line The HAP message
 * @param location The location to match the comma
 */
void match_comma(char *line, int location) {
    char read[3];
    snprintf(read, 2, "%s", &line[location]);
    if (strcmp(read, ",") != 0) {
        player_exit(PLAYER_COMMUNICATION_ERROR);
    }
}

/** Does associated things with a YT message.
 *
 * @param line The YT message
 * @param path The path
 * @param playerId The playerID
 * @param playerType The player Type, either A or B
 * @return 0 on success.
 */
int do_message_yt(char *line, Path *path, int playerId, char playerType) {
    int move = 0;
    if (playerType == 'A') {
        move = player_a_next_move(path, playerId);
    } else if (playerType == 'B') {
        move = player_b_next_move(path, playerId);
    }

    fprintf(stdout, "DO%d\n", path->players[playerId]->siteNumber + move);
    fflush(stdout);

    return 0;
}

/** Matches the playerID in a HAP message.
 *
 * @param path The path
 * @param line The HAP message
 * @return The player ID in the HAP message.
 */
int match_player_id(Path *path, char *line) {
    char *trash;
    char subString[5];
    char *validIDs = "0123456789";

    snprintf(subString, 2, "%s", &line[3]);
    if (strstr(validIDs, subString) == NULL) {
        player_exit(PLAYER_COMMUNICATION_ERROR);
    }
    int newMovePlayerID = (int) strtol(subString, &trash, 10);
    if (newMovePlayerID > path->playersInGame - 1) {
        player_exit(PLAYER_COMMUNICATION_ERROR);
    }

    return newMovePlayerID;
}

/** Matches the site in a HAP message.
 *
 * @param path The path
 * @param line The HAP message
 * @return The site in the HAP message
 */
int match_site(Path *path, char *line) {
    char *trash;
    char subString[5];
    // get the site number
    snprintf(subString, 5, "%s", &line[5]);
    int newMoveSiteID = (int) strtol(subString, &trash, 10);

    // received invalid move
    if (newMoveSiteID > path->size) {
        player_exit(PLAYER_COMMUNICATION_ERROR);
    }

    // returns the padding following functions need to add
    return newMoveSiteID;
}

/** Matches the points in a HAP message.
 *
 * @param path The game path
 * @param line The HAP message
 * @param padding The padding to be added to subsequent print calls
 * @return The points in the HAP message
 */
int match_points(Path *path, char *line, int padding) {
    char *trash;
    char subString[5];
    char *validIDs = "0123456789";

    snprintf(subString, 5, "%s", &line[7 + padding]);

    for (int i = 0; i < strlen(subString); ++i) {
        char p[2] = "\0";
        p[0] = subString[i];
        if (subString[i] == ',') {
            break;
        }
        if (strstr(validIDs, p) == NULL) {
            player_exit(PLAYER_COMMUNICATION_ERROR);
        }
    }

    // get additional points for player
    return (int) strtol(subString, &trash, 10);

}

/** Matches the money in a HAP message.
 *
 * @param line The message
 * @param padding The padding to be added to subsequent print calls
 * @return The money of the HAP message
 */
int match_money(char *line, int padding) {
    char *trash;
    char subString[5];
    // get change in money for player
    snprintf(subString, 5, "%s", &line[9 + padding]);
    int newMoveMoney = (int) strtol(subString, &trash, 10);

    return newMoveMoney;
}

/** Matches the card in a HAP message.
 *
 * @param line The message
 * @param padding The padding to be added to subsequent print calls
 * @return The card in the HAP message
 */
int match_card(char *line, int padding) {
    char *trash;
    char subString[5];
    // get the card drawn by player.
    snprintf(subString, 2, "%s", &line[11 + padding]);
    int newMoveCardDrawn = (int) strtol(subString, &trash, 10);

    if (newMoveCardDrawn > 5 || newMoveCardDrawn < 0) {
        player_exit(PLAYER_COMMUNICATION_ERROR);
    }
    // check if any more to the line
    if (line[12 + padding] != '\0') {
        player_exit(PLAYER_COMMUNICATION_ERROR);
    }
    return newMoveCardDrawn;
}

/** Do V1 and V2 actions.
 *
 * @param player The player to do the actions to
 * @param site The site which the player has landed on
 */
void site_v1_v2(Player *player, Site *site) {
    if (strcmp(site->type, V1) == 0) {
        player->countV1 += 1;
        return;
    }
    if (strcmp(site->type, V2) == 0) {
        player->countV2 += 1;
        return;
    }
}


/** Performs the associated HAP message on the player.
 *
 * @param line The line
 * @param path
 * @param playerID
 * @param playerType
 * @return 0 if successful.
 */
int do_hap(char *line, Path *path) {
    int padding = 0;

    // get details from HAP
    int newMovePlayerID = match_player_id(path, line);
    match_comma(line, 4);

    int newMoveSiteID = match_site(path, line);
    padding += count_chars(newMoveSiteID) - 1;
    match_comma(line, 6 + padding);

    int newMovePoints = match_points(path, line, padding);
    padding += count_chars(newMovePoints) - 1;
    match_comma(line, 8 + padding);

    int newMoveMoney = match_money(line, padding);
    padding += count_chars(newMoveMoney) - 1;
    match_comma(line, 10 + padding);

    int newMoveCardDrawn = match_card(line, padding);

    Player *player = path->players[newMovePlayerID];

    int steps = newMoveSiteID - player->siteNumber;
    move_player(path, newMovePlayerID, steps, PLAYER);

    Site *site = &path->sites[player->siteNumber];
    site_v1_v2(player, site);

    player->points += newMovePoints;
    player->money += newMoveMoney;

    // add card to player's cards
    if (newMoveCardDrawn > 0) {
        player->cards[newMoveCardDrawn - 1] += 1;
    }

    print_player_details(path, newMovePlayerID, stderr);

    return 0;
}

/** Checks the dealer response and acts accordingly.
 *
 * @param line The line of input the dealer sent through
 * @param path The game path
 * @param playerId The playerID
 * @param playerType The type of player, either A or B
 * @return 0 if successful, 1 otherwise
 */
int check_input(char *line, Path *path, int playerId, char playerType) {
    if (strcmp(line, "YT") == 0) {
        do_message_yt(line, path, playerId, playerType);
        return 0;
    }

    if (strcmp(line, "EARLY") == 0) {
        player_exit(EARLY_GAME_OVER);
        return 0;
    }

    if (strcmp(line, "DONE") == 0) {
        game_over(path, stderr);
        player_exit(NO_ERROR_PLAYER);
        return 0;
    }

    // need to check first 3 characters
    char read[4];
    snprintf(read, 4, "%s", &line[0]);

    if (strcmp(read, "HAP") == 0) {
        do_hap(line, path);
        return 0;
    }

    return 1;
}

/** Signal handler for SIGABRT.
 *
 * @param signal The signal received
 */
void sig_handler(int signal) {
    if (signal == SIGABRT) {
        return;
    }
}

/** Main loop for a player.
 *
 * @param numPlayers The number of players in the game
 * @param id The player's id
 * @param path The game path
 * @param playerType The type of the process - either Dealer or Player
 */
void start_player(int numPlayers, int id, Path *path, char playerType) {
    print_path(path, stderr);
    fflush(stderr);
    while (true) {
        // wait for dealer input
        char *input = read_line(stdin);
        if (strcmp(input, "\a") == 0) {
            player_exit(PLAYER_COMMUNICATION_ERROR);
        }
        if (check_input(input, path, id, playerType)) {
            player_exit(PLAYER_COMMUNICATION_ERROR);
        }
    }
}

/** Display each player's score to stdout.
 *
 * @param path The game path
 * @param location The location to display to
 */
void game_over(Path *path, FILE *location) {
    int numPlayers = path->playersInGame;
    fprintf(location, "Scores: ");
    for (int playerId = 0; playerId < numPlayers; ++playerId) {
        int score = calculate_total_score(path->players[playerId]);

        if (playerId == numPlayers - 1) {
            fprintf(location, "%d\n", score);
        } else {
            fprintf(location, "%d,", score);
        }
    }
}

/** Returns 0 if the game is over.
 *
 * @param path The game path
 * @return 0 if all players have reached the final barrier.
 */
int check_game_over(Path *path) {
    if (path->sites[path->size - 1].numPlayersCurrently ==
            path->playersInGame) {
        return 0;
    }

    return 1;
}


/** Returns the index of the next barrier.
 *
 * @param path The game path
 * @param currentSiteID The current site Id
 * @return The ID of the next barrier site.
 */
int find_next_barrier(Path *path, int currentSiteID) {
    for (int siteID = currentSiteID + 1; siteID < path->size; ++siteID) {
        if (strcmp(path->sites[siteID].type, BARRIER) == 0) {
            return siteID;
        }
    }

    // if we get here then there are no more barriers, eg we are at end
    return currentSiteID;
}

/** Returns 0 if all other players are on later sites.
 *
 * @param path The game path
 * @param player The current player
 * @return 0 if all other players are on later sites, 1 otherwise.
 */
int check_if_last_player(Path *path, Player *player) {
    // checks from left to right if player is at smallest site
    Site *sites = path->sites;
    for (int siteID = 0; siteID < path->size; ++siteID) {
        // if player is at site
        if (sites[siteID].numPlayersCurrently != 0) {
            if (player->siteNumber == siteID) {
                if (player->position == 1) {
                    return 0;
                }
                return 1;
            }
            return 1;
        }
    }
    return 1;
}

/** Check if the specified site exists before the next barrier.
 *  Returns -1 if doesnt exist, or returns number of steps if it exists.
 *  param path: The game path
 *  param player: The player
 *  param siteType: The type of site to be searched for
 *  **/
int check_before_barrier(Path *path, Player *player, char *siteType) {
    int nextBarrierID = find_next_barrier(path, player->siteNumber);
    Site *sites = path->sites;

    for (int siteID = player->siteNumber + 1; siteID < nextBarrierID;
            ++siteID) {
        if (strcmp(sites[siteID].type, siteType) == 0) {
            if (available(sites[siteID]) == 0) {
                return siteID - player->siteNumber;
            }
        }
    }
    return -1;
}

/** Returns a sum of all the player's cards.
 *
 * @param player The player
 * @return The number of cards the player has.
 */
int sum_cards(Player *player) {
    int *playerCards = player->cards;
    int total = 0;

    total += playerCards[A];
    total += playerCards[B];
    total += playerCards[C];
    total += playerCards[D];
    total += playerCards[E];

    return total;
}

/** Returns 0 if the given player has the most cards of all players.
 *
 * @param path The game path
 * @param player The player to be checked
 * @return 0 if the player has the most cards, otherwise, 1
 */
int check_if_most_cards(Path *path, Player *player) {
    for (int playerID = 0; playerID < path->playersInGame; ++playerID) {
        if (sum_cards(path->players[playerID]) >= sum_cards(player)) {
            if (playerID != player->id) {
                return 1;
            }
        }
    }
    return 0;
}

/** Returns 0 if everyone has zero cards.
 *
 * @param path The game path
 * @return 0 if all players have zero cards, or 1 otherwise.
 */
int check_empty_cards(Path *path) {
    Player **players = path->players;
    for (int playerID = 0; playerID < path->playersInGame; ++playerID) {
        if (sum_cards(players[playerID]) != 0) {
            return 1;
        }
    }
    return 0;
}

/** Finds the earliest site which is available.
 *
 * @param path The game path
 * @param currentSiteID The site the player is currently one
 * @return The ID of the next available site.
 */
int find_earliest(Path *path, int currentSiteID) {
    for (int siteID = currentSiteID + 1; siteID < path->size; ++siteID) {
        if (path->sites[siteID].numPlayersCurrently <
                path->sites[siteID].playerLimit) {
            return siteID;
        }
    }
    // shouldn't be able to get here
    return 1;
}

/** Returns the number of steps player B should make.
 *
 * @param path The game path
 * @param playerId The player ID
 * @return The number of steps a player of type B should make
 */
int player_b_next_move(Path *path, int playerId) {
    Player *player = path->players[playerId];
    Site *sites = path->sites;

    Site nextSite = sites[player->siteNumber + 1];

    // if the next site is not full
    if (available(nextSite) == 0) {
        // if all other players are on later sites than us
        if (check_if_last_player(path, player) == 0) {
            return 1;
        }
    }

    // if we have an odd amount of money
    if (player->money % 2 == 1) {
        // if there is a MO between us and the next barrier
        int steps = check_before_barrier(path, player, MO);
        if (steps != -1 && !available(path->sites[player->siteNumber +
                steps])) {
            return steps;
        }
    }

    // if we have the most cards or if everyone has zero cards
    if (!check_if_most_cards(path, player) || !check_empty_cards(path)) {
        // if there is a Ri between us and the next barrier
        int steps = check_before_barrier(path, player, RI);
        if (steps != -1) {
            return steps;
        }
    }

    // if there is a V2 between us and the next barrier, go there
    int steps = check_before_barrier(path, player, V2);
    if (steps != -1) {
        if (available(path->sites[player->siteNumber + steps]) == 0) {
            return steps;
        }
    }

    return find_earliest(path, player->siteNumber) - player->siteNumber;
}

/** Calculates the number of sets of cards the player has.
 * Removes cards from the player which have been counted.
 * param player: The player
 * param setSize: The size of the set to be matched
 * return: The number of sets of cards of the given size which the player has
 * **/
int calculate_sets(Player *player, int setSize) {
    int *cards = player->cards;
    int lowestDenom = INT_MAX;

    int seenSets = 0;
    // find how many sets
    for (int suit = A; suit <= E; ++suit) {
        if (cards[suit] == 0) {
            seenSets += 1;
        }
        if (cards[suit] < lowestDenom && cards[suit] != 0) {
            lowestDenom = cards[suit];
        }
    }

    if (seenSets != (NUM_CARDS - setSize)) {
        return 0;
    }


    // remove counted cards
    for (int suit = A; suit <= E; ++suit) {
        if (cards[suit] != 0) {
            cards[suit] -= lowestDenom;
        }
    }

    return lowestDenom;
}


/** Calculates a player's total store from items and those derived from other
 *  ways.
 *  param player: The player
 *  return: The player's total calculated score.
 *  **/
int calculate_total_score(Player *player) {
    int totalScore = player->points;

    // calculate sets of 5
    totalScore += calculate_sets(player, 5) * 10;

    // calculate sets of 4
    totalScore += calculate_sets(player, 4) * 7;

    // calculate sets of 3
    totalScore += calculate_sets(player, 3) * 5;

    // calculate sets of 2
    totalScore += calculate_sets(player, 2) * 3;

    // calculate sets of 1
    totalScore += calculate_sets(player, 1);

    // V1
    totalScore += player->countV1;
    // V2
    totalScore += player->countV2;

    return totalScore;
}

/** Returns 0 if a site is available to move to.
 *
 * @param site The site to be checked
 * @return 0 if the site is available. 1 if the site is not.
 */
int available(Site site) {
    if (site.numPlayersCurrently < site.playerLimit) {
        return 0;
    }
    return 1;
}

/** Returns the number of steps a type A player should make.
 *
 * @param path The game path
 * @param playerId The id of the player to move
 * @return The number of steps the player should take
 */
int player_a_next_move(Path *path, int playerId) {
    Player *player = path->players[playerId];
    Site *sites = path->sites;
    int nextBarrierID = find_next_barrier(path, player->siteNumber);
    // if player has money and there is Do in front, go there
    if (player->money > 0) {
        int steps = check_before_barrier(path, player, DO);
        if (steps != -1) {
            return steps;
        }
    }

    // if the next site is MO and there is room, go there
    Site nextSite = sites[player->siteNumber + 1];
    if (strcmp(nextSite.type, MO) == 0) {
        if (nextSite.numPlayersCurrently < nextSite.playerLimit) {
            // return 1 because next site is one space ahead
            return 1;
        }
    }

    // pick the closest V1, V2 or Barrier and go there
    for (int siteId = player->siteNumber + 1; siteId <= nextBarrierID;
            ++siteId) {
        if (available(sites[siteId]) == 0) {
            if (strcmp(sites[siteId].type, V1) == 0) {
                return siteId - player->siteNumber;
            }
            if (strcmp(sites[siteId].type, V2) == 0) {
                return siteId - player->siteNumber;
            }
            if (strcmp(sites[siteId].type, BARRIER) == 0) {
                return siteId - player->siteNumber;
            }
        }

    }
    // shouldnt be able to get here
    return -1;
}


/** Prints the sites and returns the number of lines required to
 *  display the board.
 *
 * @param path The game path
 * @param location The filestream to write to
 * @return The number of lines required for print_path()
 */
int print_path_find_lines(Path *path, FILE *location) {
    int numLines = 0;
    for (int sitenum = 0; sitenum < path->size; ++sitenum) {
        Site site = path->sites[sitenum];
        fprintf(location, "%s ", site.type);
        fflush(location);

        // find out how many lines we need to display
        if (site.numPlayersCurrently > numLines) {
            numLines = site.numPlayersCurrently;
        }
    }

    return numLines;
}


/** Displays the path to the given filestream.
 *
 * @param path The game path
 * @param location The filestream to print the path to.
 */
void print_path(Path *path, FILE *location) {
    Player **players = path->players;
    int numLines = print_path_find_lines(path, location);
    char **lines = make_empty_strings(numLines, path->size);

    // printing player positions below the path
    for (int siteNumber = 0; siteNumber < path->size; ++siteNumber) {
        Site site = path->sites[siteNumber];

        int printed = 0;
        int lastPrintedPos = 0;
        int playerWithLowest = -1;

        // need to loop through each player to find lowest position and print
        while (printed < site.numPlayersCurrently) {
            int lowestPosition = 9999;

            // find positioning
            for (int player = 0; player < path->playersInGame; ++player) {
                if (players[player]->siteNumber == siteNumber) {
                    if (players[player]->position < lowestPosition &&
                            players[player]->position > lastPrintedPos) {
                        lowestPosition = players[player]->position;
                        playerWithLowest = player;
                    }
                }
            }

            for (int j = 0; j < numLines; ++j) {
                // if the space hasn't been printed to
                if (lines[j][3 * siteNumber] == ' ') {
                    lines[j][3 * siteNumber] = playerWithLowest + '0';
                    printed += 1;
                    lastPrintedPos = lowestPosition;
                    break;
                }
            }
        }
    }

    // display lines
    for (int line = 0; line < numLines; ++line) {
        fprintf(location, "\n%s", lines[line]);
    }
    fprintf(location, "\n");
    fflush(stdout);
}


/** Initialises and returns a player.
 *
 * @param path The game path
 * @param id The player's id
 * @return an initialised player
 */
Player *init_player(Path *path, int id) {
    Player *player = malloc(sizeof(Player));
    player->id = id;
    player->money = 7;
    player->countV1 = 0;
    player->countV2 = 0;
    player->siteNumber = 0;
    player->points = 0;

    // init cards - there are 5 cards to a hand
    int *cards = malloc(sizeof(int) * NUM_CARDS);
    memset(cards, 0, sizeof(int) * 5);
    player->cards = cards;

    path->sites[0].numPlayersCurrently += 1;

    // dynamically increase playerIDs array for each new player
    for (int siteNum = 0; siteNum < path->size; ++siteNum) {
        Site *site = &path->sites[siteNum];
        if (path->playersInGame == 0) {
            site->playerIDs = malloc(sizeof(int));
        } else {
            site->playerIDs = realloc(site->playerIDs,
                    sizeof(int) * path->playersInGame + 1);
        }
    }

    path->playersInGame += 1;

    // ensure the player only exists at one site
    for (int siteNum = 0; siteNum < path->size; ++siteNum) {
        path->sites[siteNum].playerIDs[id] = 0;
    }
    path->sites[0].playerIDs[id] = 1;
    player->position = path->sites[0].numPlayersCurrently;

    return player;
}

/** Arranges the order of players initially in order of smallest player
 *  furthest from the path.
 *  param path: The game path
 *  **/
void arrange_order_of_players(Path *path) {
    Player **players = path->players;
    int countPlayers = path->sites[0].numPlayersCurrently;

    for (int player = 0; player < countPlayers; ++player) {
        players[player]->position = -1 * (player - countPlayers);
    }
}

/** Returns the ID of the player who has the highest score in the current
 * site.
 *
 * param path: The game path
 * param siteNum: The Site to be checked
 * Return: The Id of the player with the highest score on the current site.
 */
int get_highest_score(Path *path, int siteNum) {
    int highestScore = 0;
    int playerIdwithHS = -1;

    for (int i = 0; i < path->playersInGame; ++i) {
        // if the player is at the site
        if (path->players[i]->siteNumber == siteNum) {
            if (path->players[i]->position > highestScore) {
                highestScore = path->players[i]->position;
                playerIdwithHS = i;
            }
        }
    }

    return playerIdwithHS;
}

/** Finds the next play to make a move.
 *
 * @param path The game path
 * @return The ID of the player to move next.
 */
int next_player_to_move(Path *path) {
    Site *sites = path->sites;

    for (int i = 0; i < path->size; ++i) {
        if (sites[i].numPlayersCurrently != 0) {
            return get_highest_score(path, i);
        }
    }
    // shouldnt be able to get here
    return -1;
}

/** Does action associated with landing on a Ri site.
 *
 * @param deck The deck of cards to draw from.
 * @param player The player who landed on Ri site.
 * @return 1 for A, 2 for B, 3 for C, 4 for D, and 5 for E
 */
int do_ri(Deck *deck, Player *player) {
    Card *card = next_card(deck);
    int *playerCards = player->cards;

    if (card->value == 'A') {
        playerCards[A] += 1;
        return 1;
    }
    if (card->value == 'B') {
        playerCards[B] += 1;
        return 2;
    }
    if (card->value == 'C') {
        playerCards[C] += 1;
        return 3;
    }
    if (card->value == 'D') {
        playerCards[D] += 1;
        return 4;
    }
    if (card->value == 'E') {
        playerCards[E] += 1;
        return 5;
    }
    return 0;
}

/** Performs action for player.
 *
 * @param path The game path
 * @param playerId The player's id
 * @return 0 if successful
 */
int do_action(Path *path, int playerId) {
    Player *player = path->players[playerId];
    Site *site = &(path->sites[player->siteNumber]);

    // do MO type
    if (strcmp(site->type, MO) == 0) {
        player->money += 3;
        return 0;
    }
    // do V1 type
    if (strcmp(site->type, V1) == 0) {
        player->countV1 += 1;
        return 0;
    }
    // do V2 type
    if (strcmp(site->type, V2) == 0) {
        player->countV2 += 1;
        return 0;
    }
    // do Do type
    if (strcmp(site->type, DO) == 0) {
        // converts every 2 money to 1 point
        player->points += player->money / 2;
        player->money = 0;
        return 0;
    }
    // do Ri type
    if (strcmp(site->type, RI) == 0) {
//        do_ri(path->deck, player);
        return 0;
    }
    // barrier site, do nothing
    if (strcmp(site->type, BARRIER) == 0) {
        // do nothing
        return 0;
    }
    return 1;


}

/**
 * Moves the given player forward the given number of steps.
 * @param path The game path
 * @param playerId The player ID
 * @param steps How many steps to move
 * @param processType What type the process is
 */
void move_player(Path *path, int playerId, int steps,
        ProcessType processType) {
    //TODO check if you can move here
    Player *player = path->players[playerId];
    Site *site = &(path->sites[player->siteNumber]);
    // remove from old site
    site->numPlayersCurrently -= 1;
    site->playerIDs[player->id] = 0;
    player->siteNumber += steps;

    site = &(path->sites[player->siteNumber]);
    // move to new site
    site->numPlayersCurrently += 1;
    site->playerIDs[player->id] = 1;

    player->position = site->numPlayersCurrently;

    // do action
    if (processType == DEALER) {
        do_action(path, playerId);
    }
}
