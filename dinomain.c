#include "main.h"
#include <stdlib.h> // import for rand()

void Delay(unsigned int);
void Init_Buzzer(void);
void Init_PushButtons(void);
void LCD_Init(void);
void Write_SR_LCD(uint8_t);
void LCD_nibble_write(uint8_t, uint8_t);
void Write_Instr_LCD(uint8_t);
void Write_Char_LCD(uint8_t);
void Write_String_LCD(char*);
void Init_7seg(void);
void Write_SR_7S(uint8_t, uint8_t);
void Write_7Seg(uint8_t, uint8_t);
void SystemClock_Config(void);

//Main code 
int main(){
  // initialization functions
    
  HAL_Init();
  SystemClock_Config();
  LCD_Init(); // lcd latch: PA10 |clock: PB5 |serial input: PA5 (all output)
  Init_7seg(); // lcd latch: PC10 |clock: PB5 |serial input: PA5 (all output)
  Init_PushButtons(); // SW 2: PB11, SW 3: PB10, SW 4: PB9, SW 5: PB8
  Init_Buzzer(); // PC9

  char line1Chars[16] = {0}, line2Chars[16] = {0};
  char dino = '*', obstacle = '|';
  char* gameOver = "Sorry, you lost", welcome = "Push to Start", difficutys = "0=E 1=M 2=H";
  int startBool = 0, difficulty; // boolean to say when to start the game

  Write_String_LCD(welcome);
  Write_Instr_LCD(0xC0); // go to the second line to display difficulty ratings
  Write_String_LCD(difficutys);

  while(1){
    while (startBool == 0){ // wait till a difficulty button is pressed until we start the game
			if ((GPIOB->IDR&(0x1<<9))!=0 && (GPIOB->IDR&(0x1<<10))!=0 && (GPIOB->IDR&(0x1<<11))!=0){
        Delay(25); // make sure we are still pressed to avoid debouncing
        if ((GPIOB->IDR&(0x1<<9))!=0 && (GPIOB->IDR&(0x1<<10))!=0 && (GPIOB->IDR&(0x1<<11))!=0){
          if((GPIOB->IDR&(0x1<<11))!=0){
            difficulty = 0;
          } else if((GPIOB->IDR&(0x1<<10))!=0){
            difficulty = 1;
          } else if((GPIOB->IDR&(0x1<<9))!=0){
            difficulty = 2;
          }
          startBool = 1;
          Write_Instr_LCD(0x01); // clear screen
        }
      // stay while button is still pressed
      while((GPIOB->IDR&(0x1<<9))!=0 && (GPIOB->IDR&(0x1<<10))!=0 && (GPIOB->IDR&(0x1<<11))!=0) {}
      delay(25); // more debouncing
		  }  
    }

    // to do : rest of the program
  
  }
}

void Delay(unsigned int n){
	int i;
	if(n!=0)
	{
    for (; n > 0; n--)
        for (i = 0; i < 136; i++) ;
}
}

void Init_Buzzer(){
  uint32_t temp;

  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

  temp=GPIOC->MODER;
  temp &= ~(0x03<<(2*9));
  temp|=(0x01<<(2*9));
  GPIOC->MODER = temp;

  temp=GPIOC->OTYPER;
  temp &=~(0x01<<9);
  GPIOC->OTYPER=temp;

  temp=GPIOC->PUPDR;
  temp&=~(0x03<<(2*9));
  GPIOC->PUPDR=temp;
}

void Init_PushButtons(){
  uint32_t temp; 

  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;  /* enable GPIOB clock */
  
  // SW5
  temp = GPIOB->MODER;
  temp &= ~(0x03<<(2*8));
  GPIOB->MODER = temp; 

  temp=GPIOB->OTYPER;
  temp &=~(0x01<<8);
  GPIOB->OTYPER=temp;

  temp=GPIOB->PUPDR;
  temp&=~(0x03<<(2*8));
  GPIOB->PUPDR=temp;

  // SW4
  temp = GPIOB->MODER;
  temp &= ~(0x03<<(2*9));
  GPIOB->MODER = temp; 

  temp=GPIOB->OTYPER;
  temp &=~(0x01<<9);
  GPIOB->OTYPER=temp;

  temp=GPIOB->PUPDR;
  temp&=~(0x03<<(2*9));
  GPIOB->PUPDR=temp;

  // SW 3
  temp = GPIOB->MODER;
  temp &= ~(0x03<<(2*10));
  GPIOB->MODER = temp; 

  temp=GPIOB->OTYPER;
  temp &=~(0x01<<10);
  GPIOB->OTYPER=temp;

  temp=GPIOB->PUPDR;
  temp&=~(0x03<<(2*10));
  GPIOB->PUPDR=temp;

  // SW 2
  temp = GPIOB->MODER;
  temp &= ~(0x03<<(2*11));
  GPIOB->MODER = temp; 

  temp=GPIOB->OTYPER;
  temp &=~(0x01<<11);
  GPIOB->OTYPER=temp;

  temp=GPIOB->PUPDR;
  temp&=~(0x03<<(2*11));
  GPIOB->PUPDR=temp;

}

void LCD_Init(){
  uint32_t temp;
  /* enable GPIOA clock */ 
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; 
  /* enable GPIOB clock */ 
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  /*PB5 MOSI, PA10 /CS_7 latch , PA5 shift clock */
  
  /*PA5 and PA10 are outputs*/ temp = GPIOA->MODER;
  temp &= ~(0x03<<(2*5)); temp|=(0x01<<(2*5)); 
  temp &= ~(0x03<<(2*10)); temp|=(0x01<<(2*10)); 
  GPIOA->MODER = temp;
  
  temp=GPIOA->OTYPER;
  temp &=~(0x01<<5);
  temp &=~(0x01<<10); GPIOA->OTYPER=temp;

  temp=GPIOA->PUPDR;
  temp&=~(0x03<<(2*5));
  temp&=~(0x03<<(2*10)); GPIOA->PUPDR=temp;

  /*PB5 is output*/

  temp = GPIOB->MODER;
  temp &= ~(0x03<<(2*5)); 
  temp|=(0x01<<(2*5)); 
  GPIOB->MODER = temp;
  
  temp=GPIOB->OTYPER;
  temp &=~(0x01<<5); 
  GPIOB->OTYPER=temp;
      
  temp=GPIOB->PUPDR;
  temp&=~(0x03<<(2*5)); 
  GPIOB->PUPDR=temp;


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
  Write_SR_LCD(temp);	}

/*writing data*/ 
else if (s==1) {
	temp=temp&0xF0;
  temp=temp|0x03;	/*RS(bit 0)=1 for data EN (bit1) = high*/ 
  Write_SR_LCD(temp);

  temp=temp&0xFD; /*RS(bit 0)=1 for data EN(bit1) = low*/ 
  Write_SR_LCD(temp); 
}}

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

void Init_7seg(){
  // Serial clock
  temp = GPIOA->MODER;
	temp &= ~(0x03<<(2*5));
	temp|=(0x01<<(2*5));
	GPIOA->MODER = temp; 
			 
	temp=GPIOA->OTYPER;
	temp &=~(0x01<<5);
	GPIOA->OTYPER=temp;
	
	temp=GPIOA->PUPDR;
	temp&=~(0x03<<(2*5));
	GPIOA->PUPDR=temp;
	
  // serial input
	temp = GPIOB->MODER;
	temp &= ~(0x03<<(2*5));
	temp|=(0x01<<(2*5));
	GPIOB->MODER = temp; 
			 
	temp=GPIOB->OTYPER;
	temp &=~(0x01<<5);
	GPIOB->OTYPER=temp;
	
	temp=GPIOB->PUPDR;
	temp&=~(0x03<<(2*5));
	GPIOB->PUPDR=temp;
	
  // latch
	temp = GPIOC->MODER;
	temp &= ~(0x03<<(2*10));
	temp|=(0x01<<(2*10));
	GPIOC->MODER = temp; 
			 
	temp=GPIOC->OTYPER;
	temp &=~(0x01<<10);
	GPIOC->OTYPER=temp;
	
	temp=GPIOC->PUPDR;
	temp&=~(0x03<<(2*10));
	GPIOC->PUPDR=temp;	
	
	temp = GPIOA->MODER;
  temp &= ~(0x03<<(2*10));
  temp|=(0x01<<(2*10));
  GPIOA->MODER = temp; 
  
  temp=GPIOA->OTYPER;
  temp &=~(0x01<<10);
  GPIOA->OTYPER=temp;

  temp=GPIOA->PUPDR;
  temp&=~(0x03<<(2*10));
  GPIOA->PUPDR=temp;

}

void Write_SR_7S(uint8_t temp_Enable, uint8_t temp_Digit){
  int i;
  uint8_t mask=0b10000000;
  for(i=0; i<8; i++){
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
  for(i=0; i<8; i++)
  {
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  // Initializes the RCC Oscillators according to the specified parameters
  //  in the RCC_OscInitTypeDef structure.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  // Initializes the CPU, AHB and APB buses clocks
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}