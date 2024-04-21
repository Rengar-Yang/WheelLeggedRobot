#include "valuepack.h"
 
unsigned char *valuepack_tx_buffer;
unsigned short valuepack_tx_index;
unsigned char valuepack_tx_bit_index;
unsigned char valuepack_stage;
// 接收数据包的字节长度
const unsigned short  RXPACK_BYTE_SIZE = ((RX_BOOL_NUM+7)>>3)+RX_BYTE_NUM+(RX_SHORT_NUM<<1)+(RX_INT_NUM<<2)+(RX_FLOAT_NUM<<2);
// 接收数据包的原数据加上包头、校验和包尾 之后的字节长度
unsigned short rx_pack_length = RXPACK_BYTE_SIZE+3;
 
// 接收计数-记录当前的数据接收进度
// 接收计数每次随串口的接收中断后 +1
long rxIndex=0;
 
// 读取计数-记录当前的数据包读取进度，读取计数会一直落后于接收计数，当读取计数与接收计数之间距离超过一个接收数据包的长度时，会启动一次数据包的读取。
// 读取计数每次在读取数据包后增加 +(数据包长度)
long rdIndex=0;
 
// 用于环形缓冲区的数组，环形缓冲区的大小可以在.h文件中定义VALUEPACK_BUFFER_SIZE
unsigned char vp_rxbuff[VALUEPACK_BUFFER_SIZE];
 
// 数据读取涉及到的变量
unsigned short rdi,rdii,idl,idi,bool_index,bool_bit;
// 变量地址
uint32_t  idc;
// 记录读取的错误字节的次数
unsigned int err=0;
// 用于和校验
unsigned char sum=0;
 
// 存放数据包读取的结果
unsigned char isok;
 
// 1. 开始将数据打包，需传入定义好的数组（需保证数组长度足以存放要发送的数据）
 
void startValuePack(unsigned char *buffer)
{
	valuepack_tx_buffer = buffer;
	valuepack_tx_index = 1;
	valuepack_tx_bit_index = 0;
	valuepack_tx_buffer[0] = PACK_HEAD;
	valuepack_stage = 0;
}
 
// 2. 向数据包中放入各类数据，由于数据包的顺序结构是固定的，因此注意严格以如下的顺序进行存放，否则会出现错误
 
void putBool(unsigned char b)
{
if(valuepack_stage<=1)
{
   if(b)
      valuepack_tx_buffer[valuepack_tx_index] |= 0x01<<valuepack_tx_bit_index;
	 else
			valuepack_tx_buffer[valuepack_tx_index] &= ~(0x01<<valuepack_tx_bit_index);
 
  valuepack_tx_bit_index++;
	if(valuepack_tx_bit_index>=8)
	{
		valuepack_tx_bit_index = 0;
		valuepack_tx_index++;
	}
	valuepack_stage = 1;
}
}
 
 
void putByte(char b)
{
if(valuepack_stage<=2)
{
	if(valuepack_tx_bit_index!=0)
	{	
		valuepack_tx_index++;
	  valuepack_tx_bit_index = 0;
	}
	valuepack_tx_buffer[valuepack_tx_index] = b;
	valuepack_tx_index++;
	
	valuepack_stage = 2;
}
}
void putShort(short s)
{
if(valuepack_stage<=3)
{
		if(valuepack_tx_bit_index!=0)
	{	
		valuepack_tx_index++;
	  valuepack_tx_bit_index = 0;
	}
	valuepack_tx_buffer[valuepack_tx_index] = s&0xff;
	valuepack_tx_buffer[valuepack_tx_index+1] = s>>8;
	
	valuepack_tx_index +=2;
	valuepack_stage = 3;
}
}
void putInt(int i)
{
if(valuepack_stage<=4)
{
		if(valuepack_tx_bit_index!=0)
	{	
		valuepack_tx_index++;
	  valuepack_tx_bit_index = 0;
	}
	
	valuepack_tx_buffer[valuepack_tx_index] = i&0xff;
	valuepack_tx_buffer[valuepack_tx_index+1] = (i>>8)&0xff;
	valuepack_tx_buffer[valuepack_tx_index+2] = (i>>16)&0xff;
	valuepack_tx_buffer[valuepack_tx_index+3] = (i>>24)&0xff;
	
	valuepack_tx_index +=4;
	
	valuepack_stage = 4;
}
}
int fi;
void putFloat(float f)
{
if(valuepack_stage<=5)
{
		if(valuepack_tx_bit_index!=0)
	{	
		valuepack_tx_index++;
	  valuepack_tx_bit_index = 0;
	}
	
	fi = *(int*)(&f);
	valuepack_tx_buffer[valuepack_tx_index] = fi&0xff;
	valuepack_tx_buffer[valuepack_tx_index+1] = (fi>>8)&0xff;
	valuepack_tx_buffer[valuepack_tx_index+2] = (fi>>16)&0xff;
	valuepack_tx_buffer[valuepack_tx_index+3] = (fi>>24)&0xff;
	valuepack_tx_index +=4;
	valuepack_stage = 5;
}
}
 
 
 
 
// 3. 结束打包,函数将返回 数据包的总长度
unsigned short endValuePack()
{
  unsigned char sum=0;
	for(int i=1;i<valuepack_tx_index;i++)
	{
		sum+=valuepack_tx_buffer[i];
	}
	valuepack_tx_buffer[valuepack_tx_index] = sum;
	valuepack_tx_buffer[valuepack_tx_index+1] = PACK_TAIL;
	return valuepack_tx_index+2;
}
 
 
 
// unsigned char readValuePack(RxPack *rx_pack_ptr)
// 尝试从缓冲区中读取数据包
// 参数   - RxPack *rx_pack_ptr： 传入接收数据结构体的指针，从环形缓冲区中读取出数据包，并将各类数据存储到rx_pack_ptr指向的结构体中
// 返回值 - 如果成功读取到数据包，则返回1，否则返回0
// 
 
unsigned char readValuePack(RxPack *rx_pack_ptr)
{
	
	isok = 0;
	
	// 确保读取计数和接收计数之间的距离小于2个数据包的长度
	while(rdIndex<(rxIndex-((rx_pack_length)*2)))
          rdIndex+=rx_pack_length;	
	
	// 如果读取计数落后于接收计数超过 1个 数据包的长度，则尝试读取
	while(rdIndex<=(rxIndex-rx_pack_length))
	{
		rdi = rdIndex % VALUEPACK_BUFFER_SIZE;
		rdii=rdi+1;
		if( vp_rxbuff[rdi]==PACK_HEAD) // 比较包头
		{
			if(vp_rxbuff[(rdi+RXPACK_BYTE_SIZE+2)%VALUEPACK_BUFFER_SIZE]==PACK_TAIL) // 比较包尾 确定包尾后，再计算校验和
			{
				//  计算校验和
				sum=0;
			      for(short s=0;s<RXPACK_BYTE_SIZE;s++)
				{
					rdi++;
					if(rdi>=VALUEPACK_BUFFER_SIZE)
					  rdi -= VALUEPACK_BUFFER_SIZE;
					sum += vp_rxbuff[rdi];
				}	
						rdi++;
					if(rdi>=VALUEPACK_BUFFER_SIZE)
					  rdi -= VALUEPACK_BUFFER_SIZE;
					
                                if(sum==vp_rxbuff[rdi]) // 校验和正确，则开始将缓冲区中的数据读取出来
				{
					//  提取数据包数据 一共有五步， bool byte short int float
					// 1. bool
					#if  RX_BOOL_NUM>0
						
					  idc = (uint32_t)rx_pack_ptr->bools;
					  idl = (RX_BOOL_NUM+7)>>3;
					
					bool_bit = 0;
					for(bool_index=0;bool_index<RX_BOOL_NUM;bool_index++)
					{
						*((unsigned char *)(idc+bool_index)) = (vp_rxbuff[rdii]&(0x01<<bool_bit))?1:0;
						bool_bit++;
						if(bool_bit>=8)
						{
						  bool_bit = 0;
							rdii ++;
						}
					}
					if(bool_bit)
						rdii ++;
					
				        #endif
					// 2.byte
					#if RX_BYTE_NUM>0
						idc = (uint32_t)(rx_pack_ptr->bytes);
					  idl = RX_BYTE_NUM;
					  for(idi=0;idi<idl;idi++)
					  {
					    if(rdii>=VALUEPACK_BUFFER_SIZE)
					      rdii -= VALUEPACK_BUFFER_SIZE;
					    (*((unsigned char *)idc))= vp_rxbuff[rdii];
							rdii++;
							idc++;
					  }
				        #endif
					// 3.short
					#if RX_SHORT_NUM>0
						idc = (uint32_t)(rx_pack_ptr->shorts);
					  idl = RX_SHORT_NUM<<1;
					  for(idi=0;idi<idl;idi++)
					  {
					    if(rdii>=VALUEPACK_BUFFER_SIZE)
					      rdii -= VALUEPACK_BUFFER_SIZE;
					    (*((unsigned char *)idc))= vp_rxbuff[rdii];
							rdii++;
							idc++;
					  }
				        #endif
					// 4.int
					#if RX_INT_NUM>0
						idc = (uint32_t)(&(rx_pack_ptr->integers[0]));
					  idl = RX_INT_NUM<<2;
					  for(idi=0;idi<idl;idi++)
					  {
					    if(rdii>=VALUEPACK_BUFFER_SIZE)
					      rdii -= VALUEPACK_BUFFER_SIZE;
					    (*((unsigned char *)idc))= vp_rxbuff[rdii];
							rdii++;
							idc++;
					  }
				        #endif
					// 5.float
					#if RX_FLOAT_NUM>0
						idc = (uint32_t)(&(rx_pack_ptr->floats[0]));
					  idl = RX_FLOAT_NUM<<2;
					  for(idi=0;idi<idl;idi++)
					  {
					    if(rdii>=VALUEPACK_BUFFER_SIZE)
					      rdii -= VALUEPACK_BUFFER_SIZE;
					    (*((unsigned char *)idc))= vp_rxbuff[rdii];
							rdii++;
							idc++;
					  }
				  #endif
				      
				        // 更新读取计数
					rdIndex+=rx_pack_length;
					isok = 1;
				}else
				{ 
				// 校验值错误 则 err+1 且 更新读取计数
				  rdIndex++;
			          err++;
				}
			}else
			{
				// 包尾错误 则 err+1 且 更新读取计数
				rdIndex++;
				err++;
			}		
		}else
		{ 
			// 包头错误 则 err+1 且 更新读取计数
			rdIndex++;
			err++;
		}		
	}
    
	return isok;
}