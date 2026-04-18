#include "main.h"
#include <stdlib.h> // import for rand()
#include <string.h>

void Delay(unsigned int);
void Init_GPIO_Ports(void);
void Write_SR_LCD(uint8_t);
void LCD_nibble_write(uint8_t, uint8_t);
void Write_Instr_LCD(uint8_t);
void Write_Char_LCD(uint8_t);
void Write_String_LCD(char*);
void Write_SR_7S(uint8_t, uint8_t);
void Write_7Seg(uint8_t, uint8_t);
void SystemClock_Config(void);
int buttonPress(uint8_t);
void Shift_LCD(int);
int updateWelcome(uint32_t, int*);
int updateGame(uint32_t , int*, int*, int*);
int updateGameOver(uint32_t, int*);
int resetVars(void);
void feed_LCD(char*, char*, int, int, int*);
void createGameMap(char**, char**, int);
//New interrupt initialization
void EXTI1_SW5_SW4_Init();
void EXTI9_5_IRQHandler();
void jump_Buzz(int);
void displayScore(int);

volatile int inputEvent = 0; // must use this global variable so it is effected by the interrupt
// Used to prevent initialization of certain variables multiple times in 1 run of the game
int initVarsWelcome = 1, initVarsButton = 1, initVarsGame = 1, initVarsGameOver = 1, initVarsGameMap = 1, displayWelcomeText = 1;

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

  	// https://www.geeksforgeeks.org/c/understanding-volatile-qualifier-in-c/

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
                if (now - lastPressTime >= 20){
                  lastPressTime = now; // set the last press time every time it is pressed so the if statement works
                }
                
							displayScore(score);
					} else if(inputEvent == 2) { // Disables all of the Seven Segment Displays and gets back into the game
						Write_7Seg(0, 0);
						inputEvent = 0;
					} else if(inputEvent == 3) {
						//3 is jump
						jump = 1;
						jump_Buzz(now);
						inputEvent = 0;
					}
	        	} else
					gameState = updateGame(now, &difficulty, &jump, &score); // every tick run the update game loop while we have not finished the game
	            break;
	
	        case 2:
	        	gameState = updateGameOver(now, &score);
	        	break;
	    }
	}
}

// Turn buzzer on, after 500 ms turn off, last line of code as safegaurd
void jump_Buzz(int now){
	for(int i = 0; i < 150000; i++){
		GPIOC->ODR |= (1 << 9);
		GPIOC->ODR &= ~(1 << 9);
	}
}

void Delay(unsigned int n){
	int i;
	if(n!=0) {
	    for (; n > 0; n--)
	        for (i = 0; i < 300; i++) ;
	}
}

//Interrupt initialization
void EXTI1_SW5_SW4_Init(void){
  	RCC->APB2ENR |= 0x00000001;
  	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // turn on clock
  	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI8;
  	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI8_PB; // chooses the pin that triggers the interupt
	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9;
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI9_PB; // and another pin that triggers the same interupt so we can jump
  	EXTI->RTSR1 |= (1 << 8); // is just EXTI not EXTI1; choosing trigger type (rising edge)
  	NVIC->ISER[0U] |= (1 << 23); // If we don't use |= the other interupts will be removed; this enables the interrupts
	EXTI->RTSR1 |= (1 << 9); // initialize interupt on pin 9
}

//Call to interrupt function (events: pause:1, play:2, jump:3)
void EXTI9_5_IRQHandler(void) {
    if ((EXTI->PR1 & (1<<8)) != 0) {	// If SW5 is pressed, switch to the opposite (play vs pause) 1 is pause, 2 is play
    	EXTI->PR1 |= (1 << 8); // clear flag
    if (inputEvent != 1)
      	inputEvent = 1; // if not paused, pause
    else
      	inputEvent = 2; // if paused play
	}
	else if ((EXTI->PR1 & (1<<9)) != 0){ // If SW4 is pressed and is not paused, Jump
    //Clearing a flag
    	EXTI->PR1 |= (1 << 9);
    	inputEvent = 3;
	}
}

// game states:
int updateWelcome(uint32_t now, int* difficulty){ // take in which tick we are on
 	static uint32_t lastShift;
	static int direction;
	static int shiftChecker;
	char* welcome = "Push to Start";
  	char* difficultys = " 0=E 1=M 2=H ";
	
	if (initVarsWelcome == 1) {
		lastShift = 0; // have to use static with these variables so we don't lose their value in each function call
		direction = 0;
  		shiftChecker = 0; // two ints for direction of shifting and where on the LCD the Text is
		initVarsWelcome = 0;
	}
	
	/*The below if statement is basically a delay, if the current tick - the last tick we shifted is greater than 500 ticks
	  we can go into the function and shift again, this is required since we are going through this function every tick.
	  This removes the requirement of a delay which would mess up our timing/tick rate since this logic is also used 
	  in other functions/ parts of the game*/
  	if (displayWelcomeText == 1) {
		Write_String_LCD(welcome); // place welcome text on screen
  		Write_Instr_LCD(0xC0); // go to the second line to display difficulty ratings
  		Write_String_LCD(difficultys);
		displayWelcomeText = 0;
	}
	if (now - lastShift >= 400) {
	    lastShift = now; // note you must update now each time the if statement becomes true so we only go through every 400 ticks
		if ((direction == 1)) { // If shifting right
			if (shiftChecker < 3) { // Occurs while shift is possible
				Shift_LCD(direction); // Shifts Top Line 1
				shiftChecker++; // Increments the checker
			} else {
				direction = 0; // Changes direction to shifting left
				Shift_LCD(direction); // Shifts Top Line Left 1
				shiftChecker--; // Decrements the checker
			}
		} else if ((direction == 0)) {
	    	if (shiftChecker > 0) {
				Shift_LCD(direction);
				shiftChecker--;
	      	} else {
				direction = 1;
				Shift_LCD(direction);
				shiftChecker++;
			}
		}
	}

  	if(buttonPress(11) !=0){ // if there is a button press we need to go to the next state
    	*(difficulty) = 0; //  passing difficulty by reference since we can not return more than 1 value
    	Write_Instr_LCD(0x01); // clear screen to prepare for next state
		return 1; // return gameplay state if we see a button press
  	} else if(buttonPress(10)!=0){
    	*(difficulty) = 1;
    	Write_Instr_LCD(0x01);
    	return 1;
  	} else if(buttonPress(9) !=0){
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
	static int shiftKey = 0; // need a place holder to tell when to shift indexer to tell where we are in the state
	static int start = 0; // notice static modifier, this mean the variable will stay the same throughout function calls, the "= 0" is only valid for the first run through
	static int lost = 0;
	static char *line1; // lines for carrying row of 16 characters
	static char *line2;
	
	if (initVarsGame == 1) {
		lastScreenShift = 0; // time variable to keep up with ticks
		shiftKey = 0; // need a place holder to tell when to shift indexer to tell where we are in the state
		start = 0; // notice static modifier, this mean the variable will stay the same throughout function calls, the "= 0" is only valid for the first run through
		initVarsGame = 0;
		lost = 0;
	}
	
	if (start == 0) { // make sure we have a map first
    	createGameMap(&line1, &line2, *difficulty);
		EXTI->IMR1  |= (1 << 8); // Enabling Interrupts once the game has started
		EXTI->IMR1  |= (1 << 9);
		start = 1;
	}
	
	if (now - lastScreenShift >= 300) { // shifting the indexes onto the screen
		lastScreenShift = now;
		
		feed_LCD(line1, line2, shiftKey, *jump, &lost); // shift screen
		*score += 1;
		*jump = 0;
		
		shiftKey++; // update the offset to move the characters
		if (shiftKey >= 48) {
			shiftKey = 0;
			createGameMap(&line1, &line2, *difficulty);
		}
		if (lost == 1)
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
	char* displayScore = "   Score: ";
	char* startOver = "   Try Again??";
	
	if (initVarsGameOver == 1) { // Initialize all variables if it is the first time entering the function in the current runthrough of the game
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
			Write_String_LCD(displayScore);
			Write_Char_LCD(*score + 0x30);
			Write_Instr_LCD(0xC0);
			Write_String_LCD(startOver);
			displaySecondText = 0;
		}
		if (now - waitTime >= 2000) { // Waiting for clock to increment by 2000 to go back to welcome screen
			score = 0;
			return resetVars(); // Setting new game state to 0 and Updating all global variables to initial values
		}
	}
	return 2; // Staying in GameOver state
}

// This function puts all global variables that need to be reset to their starting values to be able to run the game again.
int resetVars() {
	initVarsWelcome = 1;
	initVarsButton = 1;
	initVarsGame = 1;
	initVarsGameOver = 1;
	initVarsGameMap = 1;
	displayWelcomeText = 1;
	return 0;
}

void displayScore(int score){

		int partialScore1, partialScore2, partialScore3, partialScore4;
	
		if (score <= 9) // Displays the Score if there is only 1 digit
			Write_7Seg(4, score);
		else if (score >= 10 && score <= 99) { // Displays the score if there is 2 digits
		partialScore1 = score % 10;
		Write_7Seg(4, partialScore1);
		partialScore2 = (score / 10) % 10;
		Write_7Seg(3, partialScore2);
		} else if (score >= 100 && score <= 999) { // Displays the score if there is 3 digits
		partialScore1 = score % 10;
		Write_7Seg(4, partialScore1);
		partialScore2 = (score / 10) % 10;
		Write_7Seg(3, partialScore2);
		partialScore3 = (score / 100) % 10;
		Write_7Seg(2, partialScore3);
		} else if (score >= 1000 && score <= 9999) { // Displays the score if there is 4 digits
		partialScore1 = score % 10;
		Write_7Seg(4, partialScore1);
		partialScore2 = (score / 10) % 10;
		Write_7Seg(3, partialScore2);
		partialScore3 = (score / 100) % 10;
		Write_7Seg(2, partialScore3);
		partialScore4 = (score / 1000) % 10;
		Write_7Seg(1, partialScore4);
	}

}

// function to make button presses simpler with debouncing active
int buttonPress(uint8_t buttonNum){
  	static int lastPressTime;
	if (initVarsButton == 1) {
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

// will write out a single frame of the lcd depending on how shifted we are
// position 1 = up position 0 = down; line1 is top and line 2 is bottom
void feed_LCD(char* line1, char* line2, int numShift, int position, int* loss){
  	for(int i = 0; i<16; i++){
		char l1 = line1[i+numShift]; // places all characters in the current frame depending on which offset we are on
		char l2 = line2[i+numShift];
		if(position == 1 && i == 1){ // jump character and see if we are on top of a x to see if we have lost.
			if(line1[i+numShift] == 'x')
				*loss += 1;
			l1 = '*';
		} else if (position == 0 && i == 1){
			if(line2[i+numShift] == 'x')
				*loss += 1;
			l2 = '*';
		}
				
		Write_Instr_LCD(0x80 | i); // writes to top line
		Write_Char_LCD(l1);
	
		Write_Instr_LCD(0xC0 | i); // writes to bottom line
		Write_Char_LCD(l2);
	}
}

// this function might be wrong, keeps writing almost random letters for some reason
void createGameMap(char** line1, char** line2, int difficulty){
	static char line1buffer[64]; // creates a register to hold a large game map so we don't have to create a new map so often
	static char line2buffer[64];
	static int start;
	int obstPosition; // obst position 1 = up, 0 = down;
	int temp = 1;
	if (initVarsGameMap == 1) {
		start = 0;
		initVarsGameMap = 0;
	}
	
	for(int i = 0; i < 64; i++){
		line1buffer[i] = ' '; // filling entire buffer with blanks
		line2buffer[i] = ' ';
	}
	
	if(difficulty == 0) {
		for(int i = 1; i <= 16; i++) {
			obstPosition = rand() % 2;
			temp = (i*4)-1; // place obsticles at indexes 3, 7, 11, 15, ... , 59, 63
			if(obstPosition == 0){
				line1buffer[temp] = 'x'; // place an obsticle in the top line
			} else {
				line2buffer[temp] = 'x'; // place an obsticle in the bottom line
			}
		}
	} else if(difficulty == 1){
		for(int i = 1; i <= 8; i++){
			obstPosition = rand() % 2;
			temp = (i*8)-1; // place obsticles at indexes 7, 15, ... , 55, 63
			if(obstPosition == 0){
				line1buffer[temp] = 'x';
			} else {
				line2buffer[temp] = 'x';
			}
		}
	} else if(difficulty == 2){
		for(int i = 1; i <= 4; i++){
			obstPosition = rand() % 2;
			temp = (i*16)-1; // place obsticle at indexe 15, 31, 47, 63
			if(obstPosition == 0){
				line1buffer[temp] = 'x';
			} else {
				line2buffer[temp] = 'x';
			}
		}
	}
	if (start == 0){ // on the first run through, we don't want to spawn any obsticles on the player so we delete the first 8 obsticles
		for(int i = 0; i<8; i++){
			line1buffer[i] = ' '; 
			line2buffer[i] = ' ';
		}
		start = 1; // make the first frame nothing
	}
	*line1 = line1buffer; // lines for carrying row of 16 characters
	*line2 = line2buffer;
}

void Shift_LCD(int direction) {
	if (direction == 0)
		Write_Instr_LCD(0x18); // left
	if (direction == 1)
		Write_Instr_LCD(0x1C); // right
}

void Init_GPIO_Ports(){
  	uint32_t temp;
  	/* enable GPIOA clock */ 
  	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; 
  	/* enable GPIOB clock */ 
  	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  	/* enable GPIOC clock*/
  	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

	// PC9: Buzzer
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
	
	// SW5: PB8; SW4: PB9; SW3: PB10; SW2: PB11
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
		
	/*PA5: CLK for Both LCD and Seven Segment & PA10: CLK for LCD set to outputs*/ 
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
	
	  /*PB5: Output Mode for LCD and Seven Segment*/
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
	
	/* LCD controller reset sequence */ 
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
	
	// PC10: latch for Seven Segment
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

void Write_SR_LCD(uint8_t temp){
	int i;
	uint8_t mask=0b10000000;
	
	for(i=0; i<8; i++) {
		if((temp&mask)==0)
			GPIOB->ODR&=~(1<<5);
		else
			GPIOB->ODR|=(1<<5);
		
		/*	Sclck */
		GPIOA->ODR&=~(1<<5); GPIOA->ODR|=(1<<5);
		Delay(1);
		
		mask=mask>>1;
	}
	
	/*Latch*/
	GPIOA->ODR|=(1<<10); 
	GPIOA->ODR&=~(1<<10);
}

void LCD_nibble_write(uint8_t temp, uint8_t s){
/*writing instruction*/ 
	if (s==0){ 
		temp=temp&0xF0;
		temp=temp|0x02; /*RS (bit 0) = 0 for Command EN (bit1)=high */ 
		Write_SR_LCD(temp);
	
	  	temp=temp&0xFD; /*RS (bit 0) = 0 for Command EN (bit1) = low*/ 
	  	Write_SR_LCD(temp);
	}
	
	/*writing data*/ 
	else if (s==1) {
		temp=temp&0xF0;
		temp=temp|0x03;	/*RS(bit 0)=1 for data EN (bit1) = high*/ 
	  	Write_SR_LCD(temp);
	
	  	temp=temp&0xFD; /*RS(bit 0)=1 for data EN(bit1) = low*/ 
	 	Write_SR_LCD(temp); 
	}
}

void Write_Instr_LCD(uint8_t code){
	LCD_nibble_write(code & 0xF0, 0);
	code = code << 4;
	LCD_nibble_write(code, 0);
	Delay(2);
}

void Write_Char_LCD(uint8_t code){
	LCD_nibble_write(code & 0xF0, 1);
	code = code << 4;
	LCD_nibble_write(code, 1);
	Delay(1);
}

void Write_String_LCD(char* temp){
	int i = 0; 
	
	while(temp[i] != 0){ // while not at the end of the string
		Write_Char_LCD(temp[i]);
		i++;
	}
}

void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit){
  	int i;
  	uint8_t mask=0b10000000;
  	for(i=0; i<8; i++) {
      	if((temp_Digit&mask)==0) 
        	GPIOB->ODR&=~(1<<5);
    	else
      		GPIOB->ODR|=(1<<5);
    	/*	Sclck */
    	GPIOA->ODR&=~(1<<5);
    	Delay(1);
    	GPIOA->ODR|=(1<<5);
    	Delay(1);
    	mask=mask>>1;
  	}

  	mask=0b10000000;
  	for(i=0; i<8; i++) {
	  	if((temp_Enable&mask)==0) 
      		GPIOB->ODR&=~(1<<5);
	  	else
      		GPIOB->ODR|=(1<<5);
    	/*	Sclck */
	  	GPIOA->ODR&=~(1<<5);
    	/*Delay(1);*/ 
	  	GPIOA->ODR|=(1<<5);
    	/*Delay(1);	*/ 
	  	mask=mask>>1;
  	}
  	/*Latch*/ // Needs to be tampered with to run LCD as well
  	GPIOC->ODR|=(1<<10); 
  	GPIOC->ODR&=~(1<<10);
}

void Write_7Seg(uint8_t temp_Enable, uint8_t temp_Digit){
  	uint8_t Enable[5] = {0x00, 0x08, 0x04, 0x02, 0x01};
  	/* Enable[i] can enable display i by writing one to DIGIT i and zeros to the other Digits */

  	uint8_t Digit[10]= {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};

  	Write_SR_7S(Enable[temp_Enable], Digit[temp_Digit]);
}

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

void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
	  {}
  /* USER CODE END Error_Handler_Debug */
}
 
