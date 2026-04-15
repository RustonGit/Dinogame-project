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
void feed_LCD(char , char);
void createGameMap(char**, char**, int);

//Main code 
int main(){
  // initialization functions
    
  HAL_Init();
  SystemClock_Config();
  Init_GPIO_Ports();

  char line1Chars[16] = {0}, line2Chars[16] = {0};
  char dino = '*', obstacle = '|';
  char* gameOver = "Sorry, you lost";
	char* welcome = "Push to Start";
	char* difficutys = " 0=E 1=M 2=H ";
  int start = 0;
	int difficulty; // int to say when to start the game and int for difficulty
  int object;
  int i;

  int gameState = 0; // 0:welcome 1:gameplay 2:gameover

	Write_String_LCD(welcome); // place welcome text on screen
 	Write_Instr_LCD(0xC0); // go to the second line to display difficulty ratings
  Write_String_LCD(difficutys);


while (1) { // start the gameplay loop

    uint32_t now = HAL_GetTick(); // create a timer variable to tell where we are in the game

    switch(gameState) { // create a switch statement to look for and run which state we are in

        case 0:
            gameState = updateWelcome(now, &difficulty);
            break;

        case 1:
            //updateGame(now);
						
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

// game states:
int updateWelcome(uint32_t now, int* difficulty){ // take in which tick we are on
  static uint32_t lastShift; // have to use static with these variables so we don't lose their value in each function call
	static int direction = 0;
  static int shiftChecker = 0; // two ints for direction of shifting and where on the LCD the Text is

  if (now - lastShift >= 400){// if there has been more than 400 ticks since the last shift
    lastShift = now; // only update last shift per shift so we can continue to poll every 400 ticks
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

void feed_LCD(char char1, char char2){
  
    Write_Instr_LCD(0x80 | 15);
    Delay(1);
    Write_Char_LCD(char1);
    Delay(1);
    Write_Instr_LCD(0xC0 | 15);
    Delay(1);
    Write_Char_LCD(char2);
}
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
    HAL_Delay(1);
    GPIOA->ODR|=(1<<5);
    HAL_Delay(1);
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
