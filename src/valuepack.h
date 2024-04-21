#ifndef _ESP32_value_pack_
#define _ESP32_value_pack_
 
#include <Arduino.h>
#define VALUEPACK_BUFFER_SIZE 512
// 3.指定接收数据包的结构-----------------------------------------------------------------------------------
//    根据实际需要的变量，定义数据包中 bool byte short int float 五种类型的数目
 
#define RX_BOOL_NUM  0
#define RX_BYTE_NUM  0
#define RX_SHORT_NUM 9
#define RX_INT_NUM   0
#define RX_FLOAT_NUM 0
 
 
 
// typedef struct   
// {	
// 	#if TX_BOOL_NUM > 0
// 	unsigned char bools[TX_BOOL_NUM];
// 	#endif
	
// 	#if TX_BYTE_NUM > 0
//   char bytes[TX_BYTE_NUM];
//   #endif
	
// 	#if TX_SHORT_NUM > 0
// 	short shorts[TX_SHORT_NUM];	
// 	#endif
	
// 	#if TX_INT_NUM > 0
// 	int  integers[TX_INT_NUM];
// 	#endif
	
// 	#if TX_FLOAT_NUM > 0
// 	float floats[TX_FLOAT_NUM];
// 	#endif
// 	char space; //只是为了占一个空，当所有变量数目都为0时确保编译成功
// }
// TxPack;
typedef struct 
{	
#if RX_BOOL_NUM > 0
	unsigned char bools[RX_BOOL_NUM];
	#endif
	
	#if RX_BYTE_NUM > 0
  char bytes[RX_BYTE_NUM];
  #endif
	
	#if RX_SHORT_NUM > 0
	short shorts[RX_SHORT_NUM];	
	#endif
	
	#if RX_INT_NUM > 0
	int  integers[RX_INT_NUM];
	#endif
	
	#if RX_FLOAT_NUM > 0
	float floats[RX_FLOAT_NUM];
	#endif
	char space; //只是为了占一个空，当所有变量数目都为0时确保编译成功
}RxPack;
// 初始化 valuepack 包括一些必要的硬件外设配置
// 并指定要传输的数据包
void startValuePack( unsigned char *buffer);
 
// 2. 向数据包中放入各类数据，由于数据包的结构是固定的，因此注意严格以如下的顺序进行存放，否则会出现错误
 
void putBool(unsigned char b);
void putByte(char b);
void putShort(short s);
void putInt(int i);
void putFloat(float f);
 
// 3. 结束打包,函数将返回 数据包的总长度
unsigned short endValuePack(void);
 
// 4. 发送数据
unsigned char readValuePack(RxPack *rx_pack_ptr);
 
 
 
#define PACK_HEAD 0xa5   
#define PACK_TAIL 0x5a
 
#endif