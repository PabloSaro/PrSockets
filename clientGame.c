#include "clientGame.h"

void showError(const char *msg){
	perror(msg);
	exit(0);
}

void showReceivedCode (unsigned int code){

	tString string;

		if (DEBUG_CLIENT){

			// Reset
			memset (string, 0, STRING_LENGTH);

			switch(code){

				case TURN_BET:
					strcpy (string, "TURN_BET");
					break;

				case TURN_BET_OK:
					strcpy (string, "TURN_BET_OK");
					break;

				case TURN_PLAY:
					strcpy (string, "TURN_PLAY");
					break;

				case TURN_PLAY_HIT:
					strcpy (string, "TURN_PLAY_HIT");
					break;

				case TURN_PLAY_STAND:
					strcpy (string, "TURN_PLAY_STAND");
					break;

				case TURN_PLAY_OUT:
					strcpy (string, "TURN_PLAY_OUT");
					break;

				case TURN_PLAY_WAIT:
					strcpy (string, "TURN_PLAY_WAIT");
					break;

				case TURN_PLAY_RIVAL_DONE:
					strcpy (string, "TURN_PLAY_RIVAL_DONE");
					break;

				case TURN_GAME_WIN:
					strcpy (string, "TURN_GAME_WIN");
					break;

				case TURN_GAME_LOSE:
					strcpy (string, "TURN_GAME_LOSE");
					break;

				default:
					strcpy (string, "UNKNOWN CODE");
					break;
			}

			printf ("Received:%s\n", string);
		}
}

unsigned int readBet (){

	int i, isValid, bet;
	tString enteredMove;

		// Init...
		bet = 0;

		// While player does not enter a correct bet...
		do{

			// Init...
			bzero (enteredMove, STRING_LENGTH);
			isValid = TRUE;

			// Show input message
			printf ("Enter a bet:");

			// Read move
			fgets(enteredMove, STRING_LENGTH-1, stdin);

			// Remove new-line char
			enteredMove[strlen(enteredMove)-1] = 0;

			// Check if each character is a digit
			for (i=0; i<strlen(enteredMove) && isValid; i++){

				if (!isdigit(enteredMove[i]))
					isValid = FALSE;
			}

			// Entered move is not a number
			if (!isValid){
				printf ("Entered bet is not correct. It must be a number greater than 0\n");
			}

			// Entered move is a number...
			else{

				// Conver entered text to an int
				bet = atoi (enteredMove);
			}

		}while (!isValid);

		printf ("\n");

	return ((unsigned int) bet);
}

unsigned int readOption (){

	int i, isValid, option;
	tString enteredMove;

		// Init...
		option = 0;

		// While player does not enter a correct bet...
		do{

			// Init...
			bzero (enteredMove, STRING_LENGTH);
			isValid = TRUE;

			// Show input message
			printf ("Press %d to hit a card and %d to stand:", PLAYER_HIT_CARD, PLAYER_STAND);

			// Read move
			fgets(enteredMove, STRING_LENGTH-1, stdin);

			// Remove new-line char
			enteredMove[strlen(enteredMove)-1] = 0;

			// Check if each character is a digit
			for (i=0; i<strlen(enteredMove) && isValid; i++){

				if (!isdigit(enteredMove[i]))
					isValid = FALSE;
			}

			// Entered move is not a number
			if (!isValid){
				printf ("Wrong option!\n");
			}

			// Entered move is a number...
			else{

				// Conver entered text to an int
				option = atoi (enteredMove);

				if ((option != PLAYER_HIT_CARD) && (option != PLAYER_STAND)){
					printf ("Wrong option!\n");
					isValid = FALSE;
				}
			}

		}while (!isValid);

		printf ("\n");

	return ((unsigned int) option);
}


int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	unsigned int port;					/** Port number (server) */
	struct sockaddr_in server_address;	/** Server address structure */
	char* serverIP;						/** Server IP */
	unsigned int endOfGame;				/** Flag to control the end of the game */
	tString playerName;					/** Name of the player */
	unsigned int code, code2;			/** Code */
	tDeck deck;


		// Check arguments!
		if (argc != 3){
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s serverIP port\n", argv[0]);
			exit(0);
		}

		// Get the server address
		serverIP = argv[1];

		// Get the port
		port = atoi(argv[2]);

		// Create socket
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Fill server address structure
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(port);
		server_address.sin_addr.s_addr = inet_addr(serverIP);

		// Connect with server
		if (connect(socketfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
			showError("ERROR while establishing connection");
		
		printf ("Connection established with server!\n");

		// Init player's name
		do{
			memset(playerName, 0, STRING_LENGTH);
			printf ("Enter player name:");
			fgets(playerName, STRING_LENGTH-1, stdin);

			// Remove '\n'
			playerName[strlen(playerName)-1] = 0;

		} while (strlen(playerName) <= 2);

		send(socketfd, playerName, strlen(playerName), 0);

		// Init
		endOfGame = FALSE;

		// Game starts
		printf ("Game starts!\n\n");

		// While game continues...
		while (!endOfGame){
		
			recv(socketfd, &code, sizeof(code), 0); // Code play
			showReceivedCode(code);

			if (code == TURN_BET) {
				recv(socketfd, &code2, sizeof(code2), 0); // bet
				printf("Your stack is %d chips\n", code2);
				printf("The bet must be (0-%d)\n", code2);
				code2 = readBet();
				send(socketfd, &code2, sizeof(code2), 0); // Send bet

			}

			else if (code == TURN_PLAY) {
				recv(socketfd, &code2, sizeof(code2), 0); // Deck's points
				recv(socketfd, &deck, sizeof(deck), 0); // Deck
				printf("Player has %d points\n", code2);
				printf("Deck: ");
				printDeck(&deck);
				code = readOption();
				if (code == PLAYER_HIT_CARD) code = TURN_PLAY_HIT;
				else code = TURN_PLAY_STAND;
				send(socketfd, &code, sizeof(code), 0);

			}

			else if (code == TURN_PLAY_WAIT) {
				recv(socketfd, &code2, sizeof(code2), 0); // Deck's points
				recv(socketfd, &deck, sizeof(deck), 0); // Deck
				printf("Rival has %d points\n", code2);
				printf("Rival's deck: ");
				printDeck(&deck);
				printf("\n");
			}

			else if (code == TURN_PLAY_OUT) {
				recv(socketfd, &code2, sizeof(code2), 0); // Deck's points
				recv(socketfd, &deck, sizeof(deck), 0); // Deck
				printf("Player is OUT!\n");
				printf("Player has %d points\n", code2);
				printf("Deck: ");
				printDeck(&deck);
				printf("\n");
			}

			else if (code == TURN_PLAY_RIVAL_DONE) {
				printf("Rival is done, your turn...\n");
				recv(socketfd, &code2, sizeof(code2), 0); // Deck's points
				recv(socketfd, &deck, sizeof(deck), 0); // Deck
				printf("Rival has %d points\n", code2);
				printf("Rival's deck: ");
				printDeck(&deck);
				printf("\n");
			}

			else if (code == TURN_GAME_WIN) {
				printf("YOU WIN!!!\n");
				endOfGame = TRUE;
			}

			else if (code == TURN_GAME_LOSE) {
				printf("YOU LOSE...\n");
				endOfGame = TRUE;
			}

			else if (code == EXIT_SERVER) {
				printf("Server disconnected...\n");
				endOfGame =  TRUE;
			}

			code = EXIT_SERVER; // Reset the value
		}

	// Close socket
	close (socketfd);
}
