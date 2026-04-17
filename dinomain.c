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
void updateGame(uint32_t , int*);
void feed_LCD(char , char, int);
void createGameMap(char**, char**, int);
//New interrupt initialization
void EXTI1_SW5_Init();
void EXTI9_5_IRQHandler();

  volatile int inputEvent = 0; // must use this global variable so it is effected by the interrupt

//Main code 
int main(){
  // initialization functions
    
  HAL_Init();
  SystemClock_Config();
  Init_GPIO_Ports();
  EXTI1_SW5_Init();

  char line1Chars[16] = {0}, line2Chars[16] = {0};
  char dino = '*', obstacle = '|';
  char* gameOver = "Sorry, you lost";
  char* welcome = "Push to Start";
  char* difficultys = " 0=E 1=M 2=H ";
  int difficulty; // int to say when to start the game and int for difficulty
  int gameState = 0; // 0:welcome 1:gameplay 2:gameover


  // https://www.geeksforgeeks.org/c/understanding-volatile-qualifier-in-c/

  Write_String_LCD(welcome); // place welcome text on screen
  Write_Instr_LCD(0xC0); // go to the second line to display difficulty ratings
  Write_String_LCD(difficultys);

while (1) { // start the gameplay loop

    uint32_t now = HAL_GetTick(); // create a timer variable to tell where we are in the game

    switch(gameState) { // create a switch statement to look for and run which state we are in

        case 0:
            gameState = updateWelcome(now, &difficulty);
            break;

        case 1:
          if(inputEvent != 0){ // if there is an interupt, do it first
						if(inputEvent == 1){
						// 1 is pause
						}	
						else if(inputEvent == 2){
						//2 is play
						}	
						else if(inputEvent == 3){
						//3 is jump
						}
          }
            updateGame(now, &difficulty); // every tick run the update game loop while we have not finished the game
            break;

        // case 2:
        //     updateGameOver(now);
        //     break;
    }
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
void EXTI1_SW5_Init(void){
  RCC->APB2ENR |= 0x00000001;
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // turn on clock
  SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI8;
  SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI8_PB; // chooses the pin that triggers the interupt
	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9;
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI9_PB; // and another pin that triggers the same interupt so we can jump
  EXTI->RTSR1 |= (1 << 8); // is just EXTI not EXTI1; choosing trigger type (rising edge)
  NVIC->ISER[0U] |= (1 << 23); // If we don't use |= the other interupts will be removed; this enables the interrupts
  EXTI->IMR1 |= (1 << 8); // the 1 goes with the IMR not the EXTI;  turns on the interupt on pin 8
	EXTI->RTSR1 |= (1 << 9); // initialize interupt on pin 9
	EXTI->IMR1  |= (1 << 9);
}
//Call to interrupt function (events: pause:1, play:2, jump:3)
void EXTI9_5_IRQHandler(void) {
    if ((EXTI->PR1 & (1<<8)) != 0) {	// If SW5 is pressed, switch to the opposite (play vs pause) 1 is pause, 2 is play
    EXTI->PR1 |= (1 << 8);
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
  static uint32_t lastShift; // have to use static with these variables so we don't lose their value in each function call
	static int direction = 0;
  static int shiftChecker = 0; // two ints for direction of shifting and where on the LCD the Text is
	
	// The below if statement is basically a delay, if the current tick - the last tick we shifted is greater than 500 ticks
	// we can go into the function and shift again, this is required since we are going through this function every tick.
	// This removes the requirement of a delay which would mess up our timing/tick rate since this logic is also used 
	// in other functions/ parts of thegame
  if (now - lastShift >= 400){
    lastShift = now; // note you must update now each time the if statement becomes true so we only go through every 400 ticks
		if ((direction == 0)) { // If shifting right
			if (shiftChecker < 3) { // Occurs while shift is possible
				Shift_LCD(direction); // Shifts Top Line 1
				shiftChecker++; // Increments the checker
			} else {
				direction = 1; // Changes direction to shifting left
				Shift_LCD(direction); // Shifts Top Line Left 1
				shiftChecker--; // Decrements the checker
			}
		} else if ((direction == 1)) {
      if (shiftChecker > 0){
				Shift_LCD(direction);
				shiftChecker--;
      } else {
				direction = 0;
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

// this game state will shift the screen to move objects toward the player jumping and playing/pausing
// will be implemeted in main since we don't want to clutter up this function any more than we have to
// too much in one function will slow down the game greatly (because it gets ran every tick)
void updateGame(uint32_t now, int* difficulty){
	static int lastScreenShift = 0; // time variable to keep up with ticks
	static int shiftKey = 0; // need a place holder to tell when to shift indexer to tell where we are in the state
	static char *line1; // lines for carrying row of 16 characters
	static char *line2;
	if (shiftKey >= 15){ // every 16 screen shifts
		shiftKey = 0;
		Write_Instr_LCD(0x01);// clear the screen
		for(int i = 0; i <16; i++) // shift screen back
			Write_Instr_LCD(0x1C);
		createGameMap(&line1, &line2, *(difficulty)); // create a new map to be displayed
	}
	if (HAL_GetTick() - lastScreenShift >= 500){ // same delay logic as in updateWelcome state
		lastScreenShift = now;
			feed_LCD(line1[shiftKey], line2[shiftKey], shiftKey);
		if(shiftKey <= 15)	
			shiftKey++; // shift 15 times
		Write_Instr_LCD(0x18);
	}
	
}

// function to make button presses simpler with debouncing active
int buttonPress(uint8_t buttonNum){
  static int lastPressTime = 0;
	// since we cannot use delay with these functions (blocks main loop from running for a bit) we must use the clock again here to debounce
	if ((GPIOB->IDR&(0x1<<buttonNum)) != 0){
		if (HAL_GetTick() - lastPressTime >= 20){
      lastPressTime = HAL_GetTick(); // set the last press time every time it is pressed so the if statement works
			return 1;
	  }
	}
	return 0; // if not pressed return 0
}

void feed_LCD(char char1, char char2, int numShift){
  
    Write_Instr_LCD(0x80 | (15+numShift)); // add numshift since shifting the display also shifts cursor positioon
    Delay(1);
    Write_Char_LCD(char1);
    Delay(1);
    Write_Instr_LCD(0xC0 | (15+numShift));
    Delay(1);
    Write_Char_LCD(char2);
}
// this function might be wrong, keeps writing almost random letters for some reason
void createGameMap(char** line1, char** line2, int spacing){
	int obstPosition = rand()%2; // obst position 1 = up, 0 = down;
	if(spacing == 4){
		for(int i = 0; i<4; i++){
			*(line1) = "   "; // start by creating the first 3 spaces
			if(obstPosition == 0){
				strcat(*(line1), "x");
				strcat(*(line2), " ");
			} else {
				strcat(*(line1), " ");// appends str with suffix: strcat(str, suffix);
				strcat(*(line2), "x");
			}
		}
	}
	if(spacing == 8){
		for(int i = 0; i<2; i++){
			*(line1) = "       "; // start by creating the first 7 spaces
			if(obstPosition == 0){
				strcat(*(line1), "x");
				strcat(*(line2), " ");
			} else {
				strcat(*(line1), " ");// appends str with suffix: strcat(str, suffix);
				strcat(*(line2), "x");
			}
		}
	}
	if(spacing == 16){
		for(int i = 0; i<1; i++){
			*(line1) = "               "; // start by creating the first 7 spaces
			if(obstPosition == 0){
				strcat(*(line1), "x");
				strcat(*(line2), " ");
			} else {
				strcat(*(line1), " ");// appends str with suffix: strcat(str, suffix);
				strcat(*(line2), "x");
			}
		}
	}
}
void Shift_LCD(int direction) {
	if (direction == 0)
		Write_Instr_LCD(0x1C);
	if (direction == 1)
		Write_Instr_LCD(0x18);
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
