#ifndef DEALER_H
#define DEALER_H

#define READ_END 0
#define WRITE_END 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include "util.h"
#include "path.h"
#include "deck.h"

int start_dealer(Path *path, char *givenPath);

#endif

