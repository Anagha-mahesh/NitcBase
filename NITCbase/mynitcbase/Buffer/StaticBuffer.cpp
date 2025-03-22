#include "StaticBuffer.h"
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() 
{
	//int blockAllocMapSlot=0;
	for(int blockIndex=0,blockAllocMapSlot=0;blockIndex<4;blockIndex++)
	{
	  unsigned char buffer[BLOCK_SIZE];
	  Disk::readBlock(buffer,blockIndex);
	  for(int slot=0;slot<BLOCK_SIZE;slot++,blockAllocMapSlot++)
	  {
	    StaticBuffer::blockAllocMap[blockAllocMapSlot]=buffer[slot];
	  }
	}
	
	// initialise all blocks as free
	for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) 
	{
		metainfo[bufferIndex].free = true;
		metainfo[bufferIndex].dirty = false;
		metainfo[bufferIndex].timeStamp = -1;
		metainfo[bufferIndex].blockNum = -1;
	}	
}
StaticBuffer::~StaticBuffer() {

  //int blockAllocMapSlot=0;
  for(int blockIndex=0,blockAllocMapSlot=0;blockIndex<4;blockIndex++)
  {
    unsigned char buffer[BLOCK_SIZE];
   
    for(int slot=0;slot<BLOCK_SIZE;slot++,blockAllocMapSlot++)
    {
      buffer[slot]=blockAllocMap[blockAllocMapSlot];
    }
    Disk::writeBlock(buffer,blockIndex);
  }
  for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
  {
    if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true)
    {
      Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
    }
  }
  for(int bufferIndex = 0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
  {
  	if(metainfo[bufferIndex].free==false && metainfo[bufferIndex].dirty==true)
  	Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
  }
}
int StaticBuffer::getFreeBuffer(int blockNum) 
{
	if (blockNum < 0 || blockNum >= DISK_BLOCKS) 
	{
	  return E_OUTOFBOUND;
	}
	for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	{
	  metainfo[bufferIndex].timeStamp++;
	}
	int allocatedBuffer=0;
	

	for (allocatedBuffer=0;allocatedBuffer<BUFFER_CAPACITY;allocatedBuffer++) 
	{
		if(metainfo[allocatedBuffer].free == true)
		{
			//allocatedBuffer=bufferIndex;
			break;
		}
	}
	if(allocatedBuffer==BUFFER_CAPACITY)
	{
	  int lastTime=-1,bufferNum=-1;
	  for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	  {
	    if(metainfo[bufferIndex].timeStamp>lastTime)
	    {
	      lastTime=metainfo[bufferIndex].timeStamp;
	      bufferNum=bufferIndex;
	    }
	  }
	  allocatedBuffer=bufferNum;
	
		if(metainfo[allocatedBuffer].dirty==true)
		{
		  Disk::writeBlock(StaticBuffer::blocks[allocatedBuffer],metainfo[allocatedBuffer].blockNum);
		}
	}
	metainfo[allocatedBuffer].free = false;
	metainfo[allocatedBuffer].dirty = false;
	metainfo[allocatedBuffer].timeStamp = 0;
	metainfo[allocatedBuffer].blockNum = blockNum;

  return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
	if (blockNum < 0 || blockNum >= DISK_BLOCKS) 
	{
	  return E_OUTOFBOUND;
	}
  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
	for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) 
	{
		if(metainfo[bufferIndex].blockNum == blockNum && metainfo[bufferIndex].free==false) 
		{
			return bufferIndex;
		}
	}
  // if block is not in the buffer
	return E_BLOCKNOTINBUFFER;
}
int StaticBuffer::getStaticBlockType(int blockNum){
    // Check if blockNum is valid (non zero and less than number of disk blocks)
    // and return E_OUTOFBOUND if not valid.
    if (blockNum < 0 || blockNum >= DISK_BLOCKS) 
	{
	  return E_OUTOFBOUND;
	}
    return (int)blockAllocMap[blockNum];
    // Access the entry in block allocation map corresponding to the blockNum argument
    // and return the block type after type casting to integer.
}
int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
      int bufIndex=getBufferNum(blockNum);
      if(bufIndex==E_BLOCKNOTINBUFFER)
      {
        return E_BLOCKNOTINBUFFER;
      }
    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
      if(bufIndex==E_OUTOFBOUND)
      {
        return E_OUTOFBOUND;
      }
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
      metainfo[bufIndex].dirty = true;
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo

      return SUCCESS;
}
