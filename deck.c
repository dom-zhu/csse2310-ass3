#include "deck.h"

/** Returns the next card in the deck.
 *
 * @param deck The deck
 * @return The next card to be drawn
 */
Card *next_card(Deck *deck) {
    Card *topCard = deck->topCard;
    Card *nextCard = topCard->nextCard;
    add_card(deck, topCard);
    deck->topCard = nextCard;

    return topCard;
}

/** Makes a card.
 *
 * @param value The card's value
 * @return The card
 */
Card *make_card(char value) {
    Card *card = malloc(sizeof(Card));
    card->value = value;

    return card;
}

/** Adds a card to the end of the deck.
 *
 * @param deck The deck
 * @param card The card to be added
 */
void add_card(Deck *deck, Card *card) {
    deck->lastCard->nextCard = card;
    deck->lastCard = card;
    deck->lastCard->nextCard = deck->topCard;
}

/** Makes the deck from the given file.
 *
 * @param file The file to read the deck from
 * @return A Deck
 */
Deck *allocate_deck(FILE *file) {
    Deck *deck = malloc(sizeof(Deck));
    char *line = read_line(file);

    // get number of cards
    char *trash;
    char buff[5];
    snprintf(buff, 5, "%s", &line[0]);
    int numberOfCards = (int) strtol(buff, &trash, 10);

    deck->count = numberOfCards;

    // add first card
    deck->topCard = make_card(line[1]);
    deck->lastCard = deck->topCard;

    // add rest of cards
    for (int cardindex = 1; cardindex < deck->count; ++cardindex) {
        add_card(deck, make_card(line[cardindex + 1]));
    }

    return deck;
}