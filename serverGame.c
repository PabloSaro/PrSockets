#include "serverGame.h"
#include <pthread.h>

void showError(const char *msg){
	perror(msg);
	exit(0);
}

void betByTurn(int currentPlayer, int socketPlayer1, int socketPlayer2, tSession* session) {

	unsigned int code;				/** Code for the active player */
	unsigned int bet;				/** Current bet */
	unsigned int stack;				/** Stack of active player */
	int socketCurrentPlayer;		/** Socket for active player */

	if (currentPlayer == player1) {
		stack = session->player1Stack;
		socketCurrentPlayer = socketPlayer1;
	}

	else {
		stack = session->player2Stack;
		socketCurrentPlayer = socketPlayer2;
	}

	// currentPlayer asked for bet
	code = TURN_BET;
	send(socketCurrentPlayer, &code, sizeof(code), 0);
	send(socketCurrentPlayer, &stack, sizeof(stack), 0);

	do {
		recv(socketCurrentPlayer, &bet, sizeof(bet), 0);
		if (bet > stack || bet > MAX_BET) {
			code = TURN_BET;
			send(socketCurrentPlayer, &code, sizeof(code), 0);
			send(socketCurrentPlayer, &stack, sizeof(stack), 0);
		}
		else {
			code = TURN_BET_OK;
			send(socketCurrentPlayer, &code, sizeof(code), 0);
		}
	} while (code == TURN_BET);

	// Update stacks after bet
	if (currentPlayer == player1) {
		session->player1Bet = bet;
		session->player1Stack -= bet;
	}

	else {
		session->player2Bet = bet;
		session->player2Stack -= bet;
	}
}

void playByTurn(int currentPlayer, int socketPlayer1, int socketPlayer2, tSession* session) {

	unsigned int card;				/** Current card */
	unsigned int code, codeRival;	/** Codes for the active player and the rival */
	unsigned int points;			/** Points of active player's Deck */
	int socketCurrentPlayer;		/** Socket for active player */
	int socketPassivePlayer;		/** Socket for the passive player */
	tDeck playerDeck;				/** Active player deck */


	if (currentPlayer == player1) {
		socketCurrentPlayer = socketPlayer1;
		socketPassivePlayer = socketPlayer2;
		playerDeck = session->player1Deck;
	}

	else {
		socketCurrentPlayer = socketPlayer2;
		socketPassivePlayer = socketPlayer1;
		playerDeck = session->player2Deck;
	}

	points = calculatePoints(&playerDeck);

	 do {
		printSession(session);
		code = TURN_PLAY;
		send(socketCurrentPlayer, &code, sizeof(code), 0);
		send(socketCurrentPlayer, &points, sizeof(points), 0);
		send(socketCurrentPlayer, &playerDeck, sizeof(playerDeck), 0);

		codeRival = TURN_PLAY_WAIT;
		send(socketPassivePlayer, &codeRival, sizeof(codeRival), 0);
		send(socketPassivePlayer, &points, sizeof(points), 0);
		send(socketPassivePlayer, &playerDeck, sizeof(playerDeck), 0);

		recv(socketCurrentPlayer, &code, sizeof(code), 0);

		if (code == TURN_PLAY_HIT) {
			card = getRandomCard(&session->gameDeck);

			// Update info for the server
			if (currentPlayer == player1)
				updateDeck(&session->player1Deck, card);

			else updateDeck(&session->player2Deck, card);

			updateDeck(&playerDeck, card);
			points = calculatePoints(&playerDeck);
		}

		// Player1 ends
		if (points > GOAL_GAME) {
			code = TURN_PLAY_OUT;
			send(socketCurrentPlayer, &code, sizeof(code), 0);
			send(socketCurrentPlayer, &points, sizeof(points), 0);
			send(socketCurrentPlayer, &playerDeck, sizeof(playerDeck), 0);
		}

	} while (code == TURN_PLAY_HIT && points <= GOAL_GAME);

	codeRival = TURN_PLAY_RIVAL_DONE;
	send(socketPassivePlayer, &codeRival, sizeof(codeRival), 0);
	send(socketPassivePlayer, &points, sizeof(points), 0);
	send(socketPassivePlayer, &playerDeck, sizeof(playerDeck), 0);
}

void *threadProcessing(void *threadArgs) {

	tSession session;				/** Session of this game */
	int socketPlayer1;				/** Socket descriptor for player 1 */
	int socketPlayer2;				/** Socket descriptor for player 2 */
	tPlayer currentPlayer;			/** Current player */
	int endOfGame;					/** Flag to control the end of the game*/
	unsigned int code, codeRival;	/** Codes for the active player and the rival */


		//Detach resources
		pthread_detach(pthread_self());

		// Get sockets for players
		socketPlayer1 = ((tThreadArgs *) threadArgs)->socketPlayer1;
		socketPlayer2 = ((tThreadArgs *) threadArgs)->socketPlayer2;
		session.gameID = ((tThreadArgs *) threadArgs)->gameID;

		// Receive player 1 info
		//bzero (session.player1Name, STRING_LENGTH);
		memset(session.player1Name, 0, STRING_LENGTH);
		recv(socketPlayer1, session.player1Name, STRING_LENGTH-1, 0);

		// Receive player 2 info
		memset(session.player2Name, 0, STRING_LENGTH);
		recv(socketPlayer2, session.player2Name, STRING_LENGTH-1, 0);

		// Init...
		endOfGame = FALSE;
		currentPlayer = player1;
		initSession(&session);

		while (!endOfGame){

			printSession(&session);

			//Bet
			betByTurn(currentPlayer, socketPlayer1, socketPlayer2, &session);
			currentPlayer = getNextPlayer(currentPlayer);
			betByTurn(currentPlayer, socketPlayer1, socketPlayer2, &session);
			currentPlayer = getNextPlayer(currentPlayer);

			//Play
			updateDeck(&session.player1Deck, getRandomCard(&session.gameDeck));
			updateDeck(&session.player1Deck, getRandomCard(&session.gameDeck));
			updateDeck(&session.player2Deck, getRandomCard(&session.gameDeck));
			updateDeck(&session.player2Deck, getRandomCard(&session.gameDeck));

			playByTurn(currentPlayer, socketPlayer1, socketPlayer2, &session);
			currentPlayer = getNextPlayer(currentPlayer);
			playByTurn(currentPlayer, socketPlayer1, socketPlayer2, &session);
			currentPlayer = getNextPlayer(currentPlayer);

			// Final step
			updateStacks(&session);

			if (session.player1Stack == 0) {
				code = TURN_GAME_LOSE;
				codeRival = TURN_GAME_WIN;
				send(socketPlayer1, &code, sizeof(code), 0);
				send(socketPlayer2, &codeRival, sizeof(codeRival), 0);
				endOfGame = TRUE;
			}

			else if (session.player2Stack == 0) {
			code = TURN_GAME_LOSE;
			codeRival = TURN_GAME_WIN;
			send(socketPlayer2, &code, sizeof(code), 0);
			send(socketPlayer1, &codeRival, sizeof(codeRival), 0);
				endOfGame = TRUE;
			}

			else {
				// Reset Decks
				initDeck(&session.gameDeck);
				clearDeck(&session.player1Deck);
				clearDeck(&session.player2Deck);
				//Reset info
				session.player1Bet = 0;
				session.player2Bet = 0;
				//Switch turn
				currentPlayer = getNextPlayer(currentPlayer);
			}

		} // Loop while

		// Close sockets
		close (socketPlayer1);
		close (socketPlayer2);

	return (NULL) ;
}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	struct sockaddr_in serverAddress;	/** Server address structure */
	unsigned int port;					/** Listening port */
	struct sockaddr_in player1Address;	/** Client address structure for player 1 */
	struct sockaddr_in player2Address;	/** Client address structure for player 2 */
	int socketPlayer1;					/** Socket descriptor for player 1 */
	int socketPlayer2;					/** Socket descriptor for player 2 */
	unsigned int clientLength;			/** Length of client structure */
	tThreadArgs *threadArgs; 			/** Thread parameters */
	pthread_t threadID;					/** Thread ID */
	unsigned int gameID;				/** Game ID */


		// Seed
		srand(time(0));

		// Check arguments
		if (argc != 2) {
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s port\n", argv[0]);
			exit(1);
		}

		// Create the socket
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		// Init server structure
		memset(&serverAddress, 0, sizeof(serverAddress));

		// Get listening port
		port = atoi(argv[1]);

		// Fill server structure
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(port);
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

		// Bind -> Asociar el socket creado con la información del struct
		if (bind(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
			showError("ERROR while binding");

		// Listen
		if (listen(socketfd, 10) < 0)
			showError("ERROR while listening");

		gameID = 0;

		// Infinite loop
		while (1) {
			clientLength = sizeof(player1Address);

			if ((socketPlayer1 = accept(socketfd, (struct sockaddr *) &player1Address, &clientLength)) < 0) //Espera player1
				showError("ERROR while accepting connection Player1");			

			if ((socketPlayer2 = accept(socketfd, (struct sockaddr *) &player2Address, &clientLength)) < 0) //Espera player2
				showError("ERROR while accepting connection Player2");

			if ((threadArgs = malloc(sizeof(tThreadArgs))) == NULL)
				showError("Error while allocating memory");

			// Asignar los id de los sockets al struct del proceso
			threadArgs->socketPlayer1 = socketPlayer1;
			threadArgs->socketPlayer2 = socketPlayer2;
			threadArgs->gameID = gameID;

			// Create a new thread, each thread is a new game of 2 players
			if (pthread_create(&threadID, NULL, threadProcessing, (void *) threadArgs) != 0)
				showError("pthread_create() failed");

			gameID++;
			//threadProcessing((void *) threadArgs);
		}
}
