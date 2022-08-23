#include "delay.h"
#include "usart.h"

int main(void) 
{	
	//抢占式优先级 2 bit ，响应式优先级:2 bit
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x20000);
	
	delay_init();
	Usart1_Init(115200);

	while(1)
	{
		printf("\n1234567890\t\n");
		delay_ms(1000);
	}
} 





