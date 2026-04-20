#include "main.h"
#include <stdlib.h> // import for rand()

int updateWelcome(uint32_t, int*);
int updateGame(uint32_t , int*, int*, int*);
int updateGameOver(uint32_t, int*);
void resetVars();
void Delay(uint32_t);
int buttonPress(uint8_t);
void buzzerActivate();
void Shift_LCD(uint8_t);
void displayScore(uint32_t, uint8_t);
void feedLCD(char*, char*, uint8_t, uint8_t, int*);
void createGameMap(char**, char**, uint8_t);
void EXTI1_SW5_SW4_Init();
void EXTI9_5_IRQHandler();
void Init_GPIO_Ports();
void Write_SR_LCD(uint8_t);
void LCD_nibble_write(uint8_t, uint8_t);
void Write_Instr_LCD(uint8_t);
void Write_Char_LCD(uint8_t);
void Write_String_LCD(char*);
void Write_SR_7S(uint8_t, uint8_t);
void Write_7Seg(uint8_t, uint8_t);
void SystemClock_Config();

		// https://www.geeksforgeeks.org/c/understanding-volatile-qualifier-in-c/
volatile int inputEvent = 0; // must use the volatile variable type so it is correctly effected by the interrupt
		// Used to prevent initialization of static variables multiple times in 1 run of the game
int initVarsWelcome = 1, initVarsButton = 1, initVarsGame = 1, initVarsGameOver = 1, initVarsGameMap = 1, displayWelcomeText = 1, initVarsFeedLCD = 1;
	

//Main code 
int main(){
  	// initialization functions
    
  	HAL_Init();
  	SystemClock_Config();
  	Init_GPIO_Ports();
  	EXTI1_SW5_SW4_Init();

  	int difficulty; // int to say when to start the game and int for difficulty
  	int gameState = 0; // 0:welcome 1:gameplay 2:gameover
		int score = 0;
    int lastPressTime = 0; // used for debouncing
		int jump = 0;
		// Using integers for displaying the score on the 7 Segment Displays


	while (1) { // start the gameplay loop

	    uint32_t now = HAL_GetTick(); // create a timer variable to tell where we are in the game
	
	    switch(gameState) { // create a switch statement to look for and run which state we are in
	
	        case 0:
				score = 0; 
				gameState = updateWelcome(now, &difficulty);
				break;
	
	        case 1:
				if(inputEvent != 0){ // if there is an interrupt, do it first	
					if(inputEvent == 1) { // Displays Score on Seven Segment when the Game is Paused
							// since we cannot use delay with these functions (blocks main loop from running for a bit) we must use the clock again here to debounce
							if (now - lastPressTime >= 20)
								lastPressTime = now; // set the last press time every time it is pressed so the if statement works
							displayScore(score, 0);
					} else if(inputEvent == 2) { // Disables all of the Seven Segment Displays and gets back into the game
						Write_7Seg(0, 0);
						inputEvent = 0;
					} else if(inputEvent == 3) { // Sets the character to jump when we update the game in updateGame
						jump = 1;
						buzzerActivate(); // Activates the buzzer to indicate that the character is jumping
						inputEvent = 0;
					}
	        } else // no interrupt
						gameState = updateGame(now, &difficulty, &jump, &score); // every tick run the update game loop while we have not finished the game
	            break;
	
	        case 2:
	        	gameState = updateGameOver(now, &score); // Every tick run the update game over loop while getting ready to go back to welcome screen
	        	break;
	    }
	}
}

// **************************** Game States **************************** //
/*
	This function displays the welcome text to the LCD and shifts it left and right.
	This occurs until the user decides the difficulty of the game based on the given prompt.
	Once difficulty is decided, it change gameState to 1 for the game being in progress.
*/
int updateWelcome(uint32_t now, int* difficulty){ // take in which tick we are on
 	static uint32_t lastShift;
	static int direction;
	static int shiftChecker;
	char* welcome = "Push to Start   "; //Added spaces to get rid of extra characters
  	char* difficultys = " 0=E 1=M 2=H    "; //Added spaces to get rid of extra characters
	
	if (initVarsWelcome == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
		lastShift = 0; // have to use static with these variables so we don't lose their value in each function call
		direction = 0;
  		shiftChecker = 0; // two ints for direction of shifting and where on the LCD the Text is
		initVarsWelcome = 0;
	}

  	if (displayWelcomeText == 1) { // Write the welcome text to LCD if it is the first time entering the function in the current runthrough of the game
		Write_String_LCD(welcome); // place welcome text on screen
  		Write_Instr_LCD(0xC0); // go to the second line to display difficulty ratings
  		Write_String_LCD(difficultys);
		displayWelcomeText = 0;
	}

	/*
	  The below if statement is basically a delay, if the current tick - the last tick we shifted is greater than 500 ticks
	  we can go into the function and shift again, this is required since we are going through this function every tick.
	  This removes the requirement of a delay which would mess up our timing/tick rate since this logic is also used 
	  in other functions/ parts of the game
	*/
	
	if (now - lastShift >= 400) {
	    lastShift = now; // note you must update now each time the if statement becomes true so we only go through every 400 ticks
		if ((direction == 1)) { // If shifting right
			if (shiftChecker < 3) { // Occurs while shift is possible
				Shift_LCD(direction); // Shifts LCD Text Right 1
				shiftChecker++; // Increments the checker
			} else {
				direction = 0; // Changes direction to shifting left
				Shift_LCD(direction); // Shifts LCD Text Left 1
				shiftChecker--; // Decrements the checker
			}
		} else if ((direction == 0)) { // If Shifting left
	    	if (shiftChecker > 0) { // Occurs while shift is possible
				Shift_LCD(direction); // Shifts LCD Text Left 1
				shiftChecker--; // Decrements the checker
	      	} else {
				direction = 1; // Changes direction to shifting right
				Shift_LCD(direction); // Shifts LCD Text Right 1
				shiftChecker++; // Increments the checker
			}
		}
	}

  	if(buttonPress(11) !=0){ // If SW2 is pressed, change difficulty to 0 and Gamestate to 1
    	*(difficulty) = 0; //  passing difficulty by reference since we can not return more than 1 value
    	Write_Instr_LCD(0x01); // clear screen to prepare for next state
		return 1; // return gameplay state if we see a button press
  	} else if(buttonPress(10)!=0){ // If SW3 is pressed, change difficulty to 1 and Gamestate to 1
    	*(difficulty) = 1;
    	Write_Instr_LCD(0x01);
    	return 1;
  	} else if(buttonPress(9) !=0) { // If SW4 is pressed, change difficulty to 2 and Gamestate to 1
    	*(difficulty) = 2;
    	Write_Instr_LCD(0x01);
    	return 1;
  	}
	return 0; // return welcome game state if we havent changed
}

/*
   this game state will shift the screen to move objects toward the player jumping and playing/pausing
   will be implemeted in main since we don't want to clutter up this function any more than we have to
   too much in one function will slow down the game greatly (because it gets ran every tick)
*/

int updateGame(uint32_t now, int* difficulty, int *jump, int* score) {
	static int lastScreenShift = 0; // time variable to keep up with ticks
	static int shiftKey = 0;
	static int start = 0; // notice static modifier, this mean the variable will stay the same throughout function calls, the "= 0" is only valid for the first run through
	static int lost = 0;
	static char *line1; // lines for carrying row of 16 characters
	static char *line2;
	
	if (initVarsGame == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
		lastScreenShift = 0; // time variable to keep up with ticks
		shiftKey = 0; // need a place holder to tell when to shift indexer to tell where we are in the array, if we are past a certain point we need to loop back and make a new game map
		start = 0; // to start out, we need to make a map before we can run the program ( maps are created at the end of a tick )
		initVarsGame = 0;
		lost = 0; // variable to tell if we have lost the game
	}
	
	if (start == 0) { // make sure we have a map first
    createGameMap(&line1, &line2, *difficulty);
		EXTI->IMR1  |= (1 << 8); // Enabling Interrupts once the game has started
		EXTI->IMR1  |= (1 << 9);
		start = 1;
	}
	
	if (now - lastScreenShift >= 300) { // shifting the indexes onto the screen
		lastScreenShift = now;
		
		feedLCD(line1, line2, shiftKey, *jump, &lost); // shift screen
		*score += 1;
		*jump = 0;
		if(*difficulty == 0){ // fixing an issue with not clearing the left symbol in easy difficulty, also helps slow down the game to make it easier
			Write_Instr_LCD(0x80);
			Write_Char_LCD(' ');
			Write_Instr_LCD(0xC0);
			Write_Char_LCD(' ');
		}
		
		shiftKey++; // update the offset to move the characters to their new position
		if (shiftKey >= 48) { // create a new map after a certain number of frames have passed (limited array size)
			shiftKey = 0;
			createGameMap(&line1, &line2, *difficulty);
		}
		if (lost == 1) // If feedLCD updates that the player lost, go to updateGameOver
			return 2;
	}
}

/*
	This game state will indicate the game is over and show the score that was reached for that run through of the game.
	It will also reset the game to the beginning of the game at the Welcome Screen.
*/

int updateGameOver(uint32_t now, int* score) {
	static int displayFirstText; // Using two integers to prevent text from writing to LCD multiple times
	static int displaySecondText;
	static uint32_t waitTime; // Using integer to keep track of how long to wait to display next text
	char* gameOver = "GAME OVER!!"; // Using char* to write Game Over text to LCD
	char* scoreText = "  Score: ";
	char* startOver = " Try Again??     "; //Added spaces to get rid of extra characters
	
	if(initVarsGameOver == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
		//goToStart = 0;
		displayFirstText = 1;
		displaySecondText = 1;
		waitTime = now; // have to use static with these variables so we don't lose their value in each function call
		initVarsGameOver = 0;
	}
	
	if (displayFirstText == 1) { // If first time in the function, Disable interrupts and write Game Over to LCD
		EXTI->IMR1  &= ~(1 << 8);
		EXTI->IMR1  &= ~(1 << 9);
		Write_Instr_LCD(0x01);
		Write_String_LCD(gameOver);
		displayFirstText = 0;
	}
	if (now - waitTime >= 1000) { // Waiting for clock to increment by 1000 to display next text
		if (displaySecondText == 1) { // If first time in if statement, write score and prompt to play again
			Write_Instr_LCD(0x80);
			Write_String_LCD(scoreText);
			displayScore(*score, 1);
			Write_Instr_LCD(0xC0);
			Write_String_LCD(startOver);
			displaySecondText = 0;
		}
		if (now - waitTime >= 2000) { // Waiting for clock to increment by 2000 to go back to welcome screen
			*score = 0;
			resetVars();
			return 0; // Setting new game state to 0 and Updating all global variables to initial values
		}
	}
	return 2; // Staying in GameOver state
}

// **************************** Base Functions **************************** //
// This function puts all global variables that need to be reset to their starting values to be able to run the game again.
void resetVars() {
	initVarsWelcome = 1;
	initVarsButton = 1;
	initVarsGame = 1;
	initVarsGameOver = 1;
	initVarsGameMap = 1;
	initVarsFeedLCD = 1;
	displayWelcomeText = 1;
}

// Acts as a function to wait a specified amount of time before leaving to go back to where it was called from
void Delay(uint32_t n){
	int i;
	if(n!=0) {
	    for (; n > 0; n--)
	        for (i = 0; i < 300; i++);
	}
}

// This function makes button presses simpler with debouncing active
int buttonPress(uint8_t buttonNum){
  	static int lastPressTime;
	if (initVarsButton == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
		lastPressTime = 0;
		initVarsButton = 0;
	}
	// since we cannot use delay with these functions (blocks main loop from running for a bit) we must use the clock again here to debounce
	if ((GPIOB->IDR&(0x1<<buttonNum)) != 0){
		if (HAL_GetTick() - lastPressTime >= 20){
      		lastPressTime = HAL_GetTick(); // set the last press time every time it is pressed so the if statement works
			return 1;
	  	}
	}
	return 0; // if not pressed return 0
}

// Turn buzzer on, after 50 ms turn off, last line of code as safegaurd
void buzzerActivate(){
	for(int i = 0; i < 15000; i++) {
		GPIOC->ODR |= (1 << 9);
		GPIOC->ODR &= ~(1 << 9);
	}
}

/*
 Shifts LCD text based on the direction given to the function (could not use this for gameUpdate since
 there is a limit on the number of shifts you can do before you can no longer display material (it shifts the screen not your view)
*/
void Shift_LCD(uint8_t direction) {
	if (direction == 0) // Shifts the LCD text to the left if direction = 0
		Write_Instr_LCD(0x18);
	if (direction == 1) // Shifts the LCD text to the right if direction = 1
		Write_Instr_LCD(0x1C);
}
// This function displays the current score of the game in progress when the game is paused on the 7 segment or LCD
void displayScore(uint32_t score, uint8_t medium){
		// if LCD, medium = 1, if 7 segment, medium = 0
		int partialScore1, partialScore2, partialScore3, partialScore4; // Establishing integers for every digit of score when displaying it on the Seven Segment Displays
		
		if(medium == 0){
				if (score <= 9) // Displays the Score if there is only 1 digit
					Write_7Seg(4, score); // Displays ones digit on Segment 4
				else if (score >= 10 && score <= 99) { // Displays the score if there is 2 digits
					partialScore1 = score % 10;
					Write_7Seg(4, partialScore1); // Displays ones digit on Segment 4
					partialScore2 = (score / 10) % 10;
					Write_7Seg(3, partialScore2); // Displays tens digit on Segment 3
				} else if (score >= 100 && score <= 999) { // Displays the score if there is 3 digits
					partialScore1 = score % 10;
					Write_7Seg(4, partialScore1); // Displays ones digit on Segment 4
					partialScore2 = (score / 10) % 10;
					Write_7Seg(3, partialScore2); // Displays tens digit on Segment 3
					partialScore3 = (score / 100) % 10;
					Write_7Seg(2, partialScore3); // Displays hundreds digit on Segment 2
				} else if (score >= 1000 && score <= 9999) { // Displays the score if there is 4 digits
					partialScore1 = score % 10;
					Write_7Seg(4, partialScore1); // Displays ones digit on Segment 4
					partialScore2 = (score / 10) % 10;
					Write_7Seg(3, partialScore2); // Displays tens digit on Segment 3
					partialScore3 = (score / 100) % 10;
					Write_7Seg(2, partialScore3); // Displays hundreds digit on Segment 2
					partialScore4 = (score / 1000) % 10;
					Write_7Seg(1, partialScore4); // Displays thousands digit on Segment 1
				}
		} else {
				if (score <= 9) // Displays the Score if there is only 1 digit
					Write_Char_LCD('0' + score); // Displays ones digit
				else if (score >= 10 && score <= 99) { // Displays the score if there is 2 digits
					partialScore1 = score % 10;
					partialScore2 = (score / 10) % 10;
					Write_Char_LCD('0' + partialScore2); // Displays tens digit
					Write_Char_LCD('0' + partialScore1); // Displays ones digit
				} else if (score >= 100 && score <= 999) { // Displays the score if there is 3 digits
					partialScore1 = score % 10;
					partialScore2 = (score / 10) % 10;
					partialScore3 = (score / 100) % 10;
					Write_Char_LCD('0' + partialScore3); // Displays hundreds digit
					Write_Char_LCD('0' + partialScore2); // Displays tens digit					
					Write_Char_LCD('0' + partialScore1); // Displays ones digit
				} else if (score >= 1000 && score <= 9999) { // Displays the score if there is 4 digits
					partialScore1 = score % 10;
					partialScore2 = (score / 10) % 10;
					partialScore3 = (score / 100) % 10;
					partialScore4 = (score / 1000) % 10;
					Write_Char_LCD('0' + partialScore4); // Displays thousands digit
					Write_Char_LCD('0' + partialScore3); // Displays hundreds digit					
					Write_Char_LCD('0' + partialScore2); // Displays tens digit					
					Write_Char_LCD('0' + partialScore1); // Displays ones digit					
				}
		}
}

// **************************** Game State Helper Functions **************************** //
/*
	will write out a single frame of the lcd depending on how shifted we
	are as well as the majority of the game logic which is collision and jumping
 	position 1 = up position 0 = down; line1 is top and line 2 is bottom
*/
void feedLCD(char* line1, char* line2, uint8_t numShift, uint8_t yposition, int* loss){
	int obstPos1[4]; // four obsticales at most at any time
	int obstPos2[4];
	int posIndexer = 0;// int to check for obsticals in our current frame
	char l1; // storing the character to print (l1 and l2 to keep things organized and understandable)
	char l2;
	int upDown; // variable to store weather an obstical is on the top or on the bottom line
	static int prevJump = 0;
	
	if (initVarsFeedLCD == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
		prevJump = 0;
		initVarsFeedLCD = 0;
	}
	
	for(int i = 0; i < 4; i++){ // initialize obstical positions to 0
    obstPos1[i] = 0;
    obstPos2[i] = 0;
	}
	
	for(int i = 0; i<16; i++){ // detect where every obstacle is
		if (line1[i+numShift] == 'x'){
			obstPos1[posIndexer] = i+numShift;
			posIndexer++;
		}	 else if (line2[i+numShift] == 'x'){
			obstPos2[posIndexer] = i+numShift;
			posIndexer++;
		}
		if (i == 1){
			if((yposition == 1) && (line1[i+numShift] == 'x')){ // check to see if we collided with an obstacle
					*loss += 1;
			} else if((yposition == 0) && (line2[i+numShift] == 'x')){// if the position of the obstacle is where the character is we should set a lost variable
					*loss += 1;
			}
			if (prevJump != yposition){
				if (prevJump == 1 && (line1[i+numShift] != 'x')){ // clearing previous jumping position when we have just jumped (prevJump != yposition)
					Write_Instr_LCD(0x81);
					Write_Char_LCD(' ');
				} else if (prevJump == 0 && (line2[i+numShift] != 'x')){
					Write_Instr_LCD(0xC1);
					Write_Char_LCD(' ');
				}
			}
		}
	}
	
	for(int i = 0; i<4; i++){ // writing each obstacle position
		
		// notice all the if statements, if there are less than 4 on the screen we will look past them and do nothing
		
		l1 = ' '; // initialize l1 and l2 to blank in case we try to print them by accident
		l2 = ' ';
		if(line1[obstPos1[i]] == 'x'){ // check which line each obstacle is on
			l1 = line1[obstPos1[i]];
			upDown = 1;
		}
		else if(line2[obstPos2[i]] == 'x'){
			l2 = line2[obstPos2[i]];
			upDown = 0;
		} else
			break; // if there are no more obstacles to check, skip to the end
		
		
		if(line1[numShift] == 'x'){ // if the first character in line 1 is 'x' then delete it
			Write_Instr_LCD(0x80); // deletes previous character
			Write_Char_LCD(' ');
		} else if(line2[numShift] == 'x'){ // if the first character in line 2 is 'x' then delete it
			Write_Instr_LCD(0xC0); // deletes previous character
			Write_Char_LCD(' ');
		}
		
		if(upDown == 1){ // deleting obsticals from their previous position
			Write_Instr_LCD(0x80 | obstPos1[i]+1-numShift); // deletes previous character
			Write_Char_LCD(' ');
				
			Write_Instr_LCD(0x80 | obstPos1[i]-numShift); // writes to top line
			Write_Char_LCD(l1);
	  	} else {
			Write_Instr_LCD(0xC0 | obstPos2[i]+1-numShift); // deletes previous character
			Write_Char_LCD(' ');
				
			Write_Instr_LCD(0xC0 | obstPos2[i]-numShift); // writes to bottom line
			Write_Char_LCD(l2);
		}
	}
	
		if(yposition == 1 && (prevJump != yposition)){ // place the character in a position so we go over any obstacle that might be in its way
			Write_Instr_LCD(0x81);
			Write_Char_LCD('*');
		} else if(yposition == 0){
			Write_Instr_LCD(0xC1);
			Write_Char_LCD('*');
		}
		prevJump = yposition;
}

/*
	forms the position of the obstacle in the game. We use rand() to make this
	random and this will be called every time we run out of characters to display
*/
void createGameMap(char** line1, char** line2, uint8_t difficulty){
	static char line1buffer[64]; // creates a register to hold a large game map so we don't have to create a new map so often
	static char line2buffer[64];
	static int start;
	int obstPosition; // obst position 1 = up, 0 = down;
	int temp = 1;
	if (initVarsGameMap == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
		start = 0;
		initVarsGameMap = 0;
	}
	
	for(int i = 0; i < 64; i++){
		line1buffer[i] = ' '; // filling entire buffer with blanks to minimize later logic
		line2buffer[i] = ' ';
	}
	
	if(difficulty == 2) { // Creates game map with obstacles every 4 spaces if hard is chosen
		for(int i = 1; i <= 16; i++) {
			obstPosition = rand() % 2;
			temp = (i*4)-1; // place obstacles at indexes 3, 7, 11, 15, ... , 59, 63
			if(obstPosition == 0){
				line1buffer[temp] = 'x'; // place an obstacle in the top line
			} else {
				line2buffer[temp] = 'x'; // place an obstacle in the bottom line
			}
		}
	} else if(difficulty == 1){ // Ever 8 spaces if medium is chosen
		for(int i = 1; i <= 8; i++){
			obstPosition = rand() % 2;
			temp = (i*8)-1; // place obstacles at indexes 7, 15, ... , 55, 63
			if(obstPosition == 0){
				line1buffer[temp] = 'x';
			} else {
				line2buffer[temp] = 'x';
			}
		}
	} else if(difficulty == 0){ // And every 16 spaces if easy is chosen
		for(int i = 1; i <= 4; i++) {
			obstPosition = rand() % 2;
			temp = (i*16)-1; // place obstacle at indexes 15, 31, 47, 63
			if(obstPosition == 0)
				line1buffer[temp] = 'x';
			else
				line2buffer[temp] = 'x';
		}
	}
	if (start == 0) { // on the first run through, we don't want to spawn any obstacles on the player so we delete the first 8 obstacles
		for(int i = 0; i<8; i++) {
			line1buffer[i] = ' '; 
			line2buffer[i] = ' ';
		}
		start = 1; // make the first frame nothing
	}
	*line1 = line1buffer; // lines for carrying row of 16 characters
	*line2 = line2buffer;
}

// **************************** Initialization Functions **************************** //
//Interrupt initialization
void EXTI1_SW5_SW4_Init(void){
  	RCC->APB2ENR |= 0x00000001;
  	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // turn on clock
  	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI8;
  	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI8_PB; // chooses the pin that triggers the interrupt
	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9;
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI9_PB; // and another pin that triggers the same interrupt so we can jump
  	EXTI->RTSR1 |= (1 << 8); // is just EXTI not EXTI1; choosing trigger type (rising edge)
  	NVIC->ISER[0U] |= (1 << 23); // If we don't use |= the other interrupts will be removed; this enables the interrupts
	EXTI->RTSR1 |= (1 << 9); // initialize interrupt on pin 9
}

//Call to interrupt function (events: pause:1, play:2, jump:3) whenever interrupt occurs
void EXTI9_5_IRQHandler(void) {
    if ((EXTI->PR1 & (1<<8)) != 0) { // If SW5 is pressed, switch to the opposite (play vs pause) 1 is pause, 2 is play
    	EXTI->PR1 |= (1 << 8); // Clearing Pending Flag
	    if (inputEvent != 1)
	      	inputEvent = 1; // if not paused, pause
	    else
	      	inputEvent = 2; // if paused play
	}
	else if ((EXTI->PR1 & (1<<9)) != 0){ // If SW4 is pressed and is not paused, Jump
    	EXTI->PR1 |= (1 << 9); // Clearing Pending Flag
    	inputEvent = 3;
	}
}

// Initializes all ports for LCD, Seven Segment Displays, Buzzer, and Buttons
void Init_GPIO_Ports() {
  	uint32_t temp;
  	/* enable GPIOA clock */ 
  	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; 
  	/* enable GPIOB clock */ 
  	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  	/* enable GPIOC clock*/
  	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

	// Initializing PC9: Buzzer
	temp = GPIOC->MODER;
	temp &= ~(0x03<<(2*9));
	temp |= (0x01<<(2*9));
	GPIOC->MODER = temp;
	
	temp = GPIOC->OTYPER;
	temp &= ~(0x01<<9);
	GPIOC->OTYPER = temp;
	
	temp = GPIOC->PUPDR;
	temp &= ~(0x03<<(2*9));
	GPIOC->PUPDR = temp;
	
	// Initializing SW5: PB8; SW4: PB9; SW3: PB10; SW2: PB11
	temp = GPIOB->MODER;
	temp &= ~(0x03<<(2*8));
	temp &= ~(0x03<<(2*9));
	temp &= ~(0x03<<(2*10));
	temp &= ~(0x03<<(2*11));
	GPIOB->MODER = temp; 
	
	temp = GPIOB->OTYPER;
	temp &= ~(0x01<<8);
	temp &= ~(0x01<<9);
	temp &= ~(0x01<<10);
	temp &= ~(0x01<<11);
	GPIOB->OTYPER = temp;
	
	temp = GPIOB->PUPDR;
	temp &= ~(0x03<<(2*8));
	temp &= ~(0x03<<(2*9));
	temp &= ~(0x03<<(2*10));
	temp &= ~(0x03<<(2*11));
	GPIOB->PUPDR = temp;
		
	// Initializing PA5: CLK for Both LCD and Seven Segment & PA10: CLK for LCD set to outputs 
	temp = GPIOA->MODER;
	temp &= ~(0x03<<(2*5)); 
	temp |= (0x01<<(2*5)); 
	temp &= ~(0x03<<(2*10));
	temp |= (0x01<<(2*10)); 
	GPIOA->MODER = temp;
	  
	temp = GPIOA->OTYPER;
	temp &=~(0x01<<5);
	temp &=~(0x01<<10);
	GPIOA->OTYPER=temp;
	
	temp = GPIOA->PUPDR;
	temp &= ~(0x03<<(2*5));
	temp &= ~(0x03<<(2*10));
	GPIOA->PUPDR = temp;
	
	// Initializing PB5: Output Mode for LCD and Seven Segment
	temp = GPIOB->MODER;
	temp &= ~(0x03<<(2*5)); 
	temp |= (0x01<<(2*5)); 
	GPIOB->MODER = temp;
	  
	temp = GPIOB->OTYPER;
	temp &=~(0x01<<5); 
	GPIOB->OTYPER=temp;
	      
	temp = GPIOB->PUPDR;
	temp &= ~(0x03<<(2*5)); 
	GPIOB->PUPDR = temp;
	
	// Initializing LCD controller reset sequence
	Delay(20);
	LCD_nibble_write(0x30,0); 
	Delay(5); 
	LCD_nibble_write(0x30,0); 
	Delay(1); 
	LCD_nibble_write(0x30,0);
	Delay(1); 
	LCD_nibble_write(0x20,0); 
	Delay(1);
	
	Write_Instr_LCD(0x28); /* set 4 bit data LCD - two line display - 5x8 font*/ 
	Write_Instr_LCD(0x0E); /* turn on display, turn on cursor , turn off blinking */ 
	Write_Instr_LCD(0x01); /* clear display screen and return to home position*/ 
	Write_Instr_LCD(0x06); /* move cursor to right (entry mode set instruction)*/
	
	// Initializing PC10: latch for Seven Segment
	temp = GPIOC->MODER;
	temp &= ~(0x03<<(2*10));
	temp |= (0x01<<(2*10));
	GPIOC->MODER = temp; 
			 
	temp = GPIOC->OTYPER;
	temp &= ~(0x01<<10);
	GPIOC->OTYPER = temp;
	
	temp = GPIOC->PUPDR;
	temp &= ~(0x03<<(2*10));
	GPIOC->PUPDR = temp;	
}

// **************************** LCD Base Functions **************************** //
// This function acts as the processing function for the LCD
void Write_SR_LCD(uint8_t temp){
	int i;
	uint8_t mask=0b10000000; // Creating a mask to process each bit of the instruction given from LCD_nibble_write
	
	for(i=0; i<8; i++) { // Looping to process each bit to the LCD
		if((temp&mask)==0) // If the current bit is 0, send 0 to the shift register
			GPIOB->ODR&=~(1<<5);
		else // If current bit is 1, send 1 to the shift register
			GPIOB->ODR|=(1<<5);
		
		// Disabling and enabling the clock to shift to the next bit in the shift register
		GPIOA->ODR&=~(1<<5);
		GPIOA->ODR|=(1<<5);
		Delay(1);
		
		mask=mask>>1; // Moving to the next bit
	}
	
	// Enabling the latch to process all of the given values of the shift register into the LCD
	GPIOA->ODR|=(1<<10); 
	GPIOA->ODR&=~(1<<10);
}

// This function sends all of the bits needed for an the LCD to form a computation
void LCD_nibble_write(uint8_t temp, uint8_t s) {
	if (s==0) { // Occurs if performing a task other than writing to LCD
		temp=temp&0xF0;
		temp=temp|0x02; /*RS (bit 0) = 0 for Command EN (bit1)=high */ 
		Write_SR_LCD(temp);
	
	  	temp=temp&0xFD; /*RS (bit 0) = 0 for Command EN (bit1) = low*/ 
	  	Write_SR_LCD(temp);
	}
	
	else if (s==1) { // Occurs if writing characters to the LCD
		temp=temp&0xF0;
		temp=temp|0x03;	/*RS(bit 0)=1 for data EN (bit1) = high*/ 
	  	Write_SR_LCD(temp);
	
	  	temp=temp&0xFD; /*RS(bit 0)=1 for data EN(bit1) = low*/ 
	 	Write_SR_LCD(temp); 
	}
}

// This function processes all instructions for the LCD to function
void Write_Instr_LCD(uint8_t code) {
	LCD_nibble_write(code & 0xF0, 0); // Processing the four most significant bits of the instruction to the processing function of the LCD
	code = code << 4;
	LCD_nibble_write(code, 0); // Processing the last four bits of the instruction being written to the processing function of the LCD
	Delay(2);
}

// This function takes a given character and writes it to the LCD
void Write_Char_LCD(uint8_t code){
	LCD_nibble_write(code & 0xF0, 1); // Writing the four most significant bits of the character to the processing function of the LCD
	code = code << 4; // Shifting the least significant bits to the four most significant bits
	LCD_nibble_write(code, 1); // Writing the last four bits of the character being written to the processing function of the LCD
	Delay(1);
}

// This function takes a given string of characters and processes each character to the LCD
void Write_String_LCD(char* temp){
	int i = 0; 
	
	while(temp[i] != 0){ // while not at the end of the string
		Write_Char_LCD(temp[i]); // Write the current character to LCD
		i++; // Move to next character
	}
}

// **************************** 7 Segment Base Functions **************************** //
// This function is the processing to process each bit and enable the latch to use the Seven Segment Displays
void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit) {
  	int i;
  	uint8_t mask=0b10000000; // Creating a mask to process each bit provided to the function
  	for(i=0; i<8; i++) { // Looping through each bit of the given number to turn on each segment for each number
      	if((temp_Digit&mask)==0) // If current bit == 0, set output to shift register as 0
        	GPIOB->ODR&=~(1<<5);
    	else // If current bit == 1, set output to shift register as 1
      		GPIOB->ODR|=(1<<5);
    	// Disables and enables the clock of the shift register being used to process the next bit
    	GPIOA->ODR&=~(1<<5);
    	Delay(1);
    	GPIOA->ODR|=(1<<5);
    	Delay(1);
    	mask=mask>>1; // Moving to next bit
  	}

  	mask=0b10000000; // Changing the mask back to original state to start at the beginning of the Enable binary #
  	for(i=0; i<8; i++) { // Looping through each bit of the provided enable number to turn on the correct seven segment
	  	if((temp_Enable&mask)==0) // If current bit == 0, set output to shift register as 0
      		GPIOB->ODR&=~(1<<5);
	  	else // If current bit == 1, set output to shift register as 1
      		GPIOB->ODR|=(1<<5);
    	/*	Sclck */
		// Disables and enables the clock of the shift register being used to process the next bit
	  	GPIOA->ODR&=~(1<<5);
    	Delay(1);
	  	GPIOA->ODR|=(1<<5);
    	Delay(1);
	  	mask=mask>>1; // Moving to next bit
  	}
  	// Enabling the latch to display the number on the given seven segment display
  	GPIOC->ODR|=(1<<10); 
  	GPIOC->ODR&=~(1<<10);
}

// This function takes in the Seven Segment Display we want and displays the given number on it
void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit){
  	uint8_t Enable[5] = {0x00, 0x08, 0x04, 0x02, 0x01}; // Assigning an array to determine which display is being used

  	uint8_t Digit[10]= {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90}; // Assigning an array to determine the code necessary to display the desired number

  	Write_SR_7S(Enable[temp_Enable], Digit[temp_Digit]); // Pass the values of the arrays into the processing function for Seven Segment Displays
}

// **************************** CubeMX functions **************************** //
// This function initializes the clock for the Microcontroller to run the program
void SystemClock_Config(void){
  	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  	// Configure the main internal regulator output voltage
  	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    	Error_Handler();
  	}

  	// Initializes the RCC Oscillators according to the specified parameters
  	//  in the RCC_OscInitTypeDef structure.
  	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  	RCC_OscInitStruct.MSICalibrationValue = 0;
  	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    	Error_Handler();
  	}

  	// Initializes the CPU, AHB and APB buses clocks
  	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    	Error_Handler();
  	}
}

// This function occurs if there is an error setting up the clocks for the program to indicate there is a problem
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
	  {}
  /* USER CODE END Error_Handler_Debug */
}
