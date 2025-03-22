#include "BlockBuffer.h"
#include<iostream>
#include <cstdlib>
#include <cstring>
BlockBuffer::BlockBuffer(int blockNum)
{
	this->blockNum = blockNum;
}
RecBuffer::RecBuffer(int blockNum):BlockBuffer::BlockBuffer(blockNum){}
BlockBuffer::BlockBuffer(char blocktype)
{
	//int blockType= blocktype=='R'?REC:UNUSED_BLK;
	//int blockNum= getFreeBlock(blockType);
	/*if(blockNum<0 || blockNum>=DISK_BLOCKS)
	{
		std::cout<<"Error:Blocknotavailable\n";
		this->blockNum=blockNum;
		return;
	}*/
	int blockType=UNUSED_BLK;
	if(blocktype=='R')
		blockType=REC;
	else if(blocktype=='I')
		blockType=IND_INTERNAL;
	else if(blocktype=='L')
		blockType=IND_LEAF;
	int blockNum=getFreeBlock(blockType);
	/*if(blockNum==E_DISKFULL)
	{
		this.blockNum=blockNum;
	}*/
	this->blockNum=blockNum;
}
RecBuffer::RecBuffer():BlockBuffer('R'){}
int BlockBuffer::getBlockNum(){
  return this->blockNum;
}
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}
IndInternal::IndInternal() : IndBuffer('I'){}
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}
IndLeaf::IndLeaf() : IndBuffer('L'){} 
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}
/*int BlockBuffer::getHeader(struct HeadInfo *head)
{
	//Stage 2:
	/*unsigned char buffer[BLOCK_SIZE];
	Disk::readBlock(buffer,this->blockNum);
	
	memcpy(&head->numSlots,buffer+24,4);
	memcpy(&head->numEntries,buffer+16,4);
	memcpy(&head->numAttrs,buffer+20,4);
	memcpy(&head->rblock,buffer+12,4);
	memcpy(&head->lblock,buffer+8,4);
	
	return SUCCESS;
	//Stage 3:
	unsigned char *bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS) 
	{
		return ret;   
	}
	memcpy(&head->numSlots,bufferPtr+24,4);
	memcpy(&head->numEntries,bufferPtr+16,4);
	memcpy(&head->numAttrs,bufferPtr+20,4);
	memcpy(&head->rblock,bufferPtr+12,4);
	memcpy(&head->lblock,bufferPtr+8,4);
	//head->pblock = header->pblock;
	return SUCCESS;
}*/
int BlockBuffer::getHeader(struct HeadInfo *head)
{

    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret; // return any errors that might have occured in the process
    }

    HeadInfo *header = (HeadInfo *)bufferPtr;

    head->numSlots = header->numSlots;
    head->numEntries = header->numEntries;
    head->numAttrs = header->numAttrs;
    head->lblock = header->lblock;
    head->rblock = header->rblock;
    head->pblock = header->pblock;

    return SUCCESS;
}
int BlockBuffer::setHeader(struct HeadInfo *head)
{
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if(ret != SUCCESS)
  	return ret;
  struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
  bufferHeader->numSlots=head->numSlots;
  bufferHeader->numAttrs=head->numAttrs;
  bufferHeader->numEntries=head->numEntries;
  bufferHeader->rblock=head->rblock;
  bufferHeader->lblock=head->lblock;
  bufferHeader->pblock=head->pblock;
  //bufferHeader->blockType=head->blockType;
  ret = StaticBuffer::setDirtyBit(this->blockNum);
  if(ret!=SUCCESS)
  	return ret;
  return SUCCESS;
}
int BlockBuffer::setBlockType(int blockType)
{
	unsigned char *bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  	if(ret != SUCCESS)
  		return ret;
  	int32_t *blockTypePtr = (int32_t*) bufferPtr;
  	*blockTypePtr = blockType;
  	//*((int32_t *)bufferPtr)=blockType;
  	StaticBuffer::blockAllocMap[this->blockNum]=blockType;
  	ret=StaticBuffer::setDirtyBit(this->blockNum);
  	if(ret!=SUCCESS)
  		return ret;
  	return SUCCESS;
}
int BlockBuffer::getFreeBlock(int blockType)
{
	int blockNum=0;
	for(blockNum=0;blockNum<DISK_BLOCKS;blockNum++)
	{
		if(StaticBuffer::blockAllocMap[blockNum]==UNUSED_BLK)
		{
			break;
		}
	}
	if(blockNum==DISK_BLOCKS)
		return E_DISKFULL;
	this->blockNum=blockNum;
	int bufferNum=StaticBuffer::getFreeBuffer(blockNum);
	/*if(bufferNum<0 || bufferNum>=BUFFER_CAPACITY)
	{
		printf("Error:Buffer full\n");
	}*/
	struct HeadInfo header;
	header.pblock=-1;
	header.rblock=-1;
	header.lblock=-1;
	header.numEntries=0;
	header.numAttrs=0;
	header.numSlots=0;
	setHeader(&header);
	setBlockType(blockType);
	return blockNum;
	
}
int RecBuffer::getRecord(union Attribute *rec,int slotNum)
{
	//Stage 2:
	/*struct HeadInfo head;
	this->getHeader(&head);
	
	int attrCount = head.numAttrs;
	int slotCount = head.numSlots;
	
	unsigned char buffer[BLOCK_SIZE];
	Disk::readBlock(buffer,this->blockNum);
	
	
	int recordSize = attrCount * ATTR_SIZE;
	unsigned char *slotPointer = buffer+HEADER_SIZE+slotCount+(recordSize*slotNum);
	memcpy(rec,slotPointer,recordSize);
	return SUCCESS;*/
	//Stage 3:
	unsigned char *bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS) 
	{
		return ret;
	}
	struct HeadInfo head;
	this->getHeader(&head);
	
	int attrCount = head.numAttrs;
	int slotCount = head.numSlots;
	
	int recordSize = attrCount * ATTR_SIZE;
	unsigned char *slotPointer = bufferPtr+HEADER_SIZE+slotCount+(recordSize*slotNum);
	memcpy(rec,slotPointer,recordSize);
	return SUCCESS;
	
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
  if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

  if (bufferNum != E_BLOCKNOTINBUFFER) {
  
    for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
    {
      StaticBuffer::metainfo[bufferIndex].timeStamp++;
    }
    StaticBuffer::metainfo[bufferNum].timeStamp=0;
  }
  else if(bufferNum == E_BLOCKNOTINBUFFER){
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
    if(bufferNum==E_OUTOFBOUND)
    {
      return E_OUTOFBOUND;
    }
    
    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }
  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];
  
  return SUCCESS;
}
int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    
    unsigned char *bufferPtr;
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS) 
	{
		return ret;
	}
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

    /* get the header of the block using the getHeader() function */
       struct HeadInfo head;
       this->getHeader(&head);
    // get number of attributes in the block.
       int attrCount = head.numAttrs;
       int slotCount = head.numSlots;
    // get the number of slots in the block.
       if(slotNum>=slotCount)
       {
         return E_OUTOFBOUND;
       }
    // if input slotNum is not in the permitted range return E_OUTOFBOUND.

    /* offset bufferPtr to point to the beginning of the record at required
       slot. the block contains the header, the slotmap, followed by all
       the records. so, for example,
       record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
       copy the record from `rec` to buffer using memcpy
       (hint: a record will be of size ATTR_SIZE * numAttrs)
    */
       int recordSize = attrCount * ATTR_SIZE;
	unsigned char *slotPointer = bufferPtr+HEADER_SIZE+slotCount+(recordSize*slotNum);
	memcpy(slotPointer,rec,recordSize);
    // update dirty bit using setDirtyBit()
       ret = StaticBuffer::setDirtyBit(this->blockNum);
       if(ret!=SUCCESS)
       {
         std::cout<<"Code error"<<std::endl;
         exit(1);
       }
    /* (the above function call should not fail since the block is already
       in buffer and the blockNum is valid. If the call does fail, there
       exists some other issue in the code) */

       return SUCCESS;
}
/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  this->getHeader(&head);
  // get the header of the block using getHeader() function

  int slotCount = head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
  memcpy(slotMap,slotMapInBuffer,slotCount);
  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)

  return SUCCESS;
}
int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using
       loadBlockAndGetBufferPtr(&bufferPtr). */

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }
    // get the header of the block using the getHeader() function
  struct HeadInfo head;
  this->getHeader(&head);
  int numSlots = head.numSlots;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
  unsigned char *slotPointer = bufferPtr + HEADER_SIZE;
  memcpy(slotPointer,slotMap,numSlots);
  
    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call
  ret = StaticBuffer::setDirtyBit(this->blockNum);
  if(ret!=SUCCESS)
    return ret;
  return SUCCESS;
}
void BlockBuffer::releaseBlock(){

    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if(blockNum==INVALID_BLOCKNUM || blockNum==UNUSED_BLK)
    {
    	return;
    }
    // else
        /* get the buffer number of the buffer assigned to the block
           using StaticBuffer::getBufferNum().
           (this function return E_BLOCKNOTINBUFFER if the block is not
           currently loaded in the buffer)
            */
   int bufferIndex=StaticBuffer::getBufferNum(blockNum);
   if(bufferIndex!=E_BLOCKNOTINBUFFER)
   {
   	StaticBuffer::metainfo[bufferIndex].free = true;
   }
        // if the block is present in the buffer, free the buffer
        // by setting the free flag of its StaticBuffer::tableMetaInfo entry
        // to true.
   StaticBuffer::blockAllocMap[blockNum]=UNUSED_BLK;
   this->blockNum=-1;
        // free the block in disk by setting the data type of the entry
        // corresponding to the block number in StaticBuffer::blockAllocMap
        // to UNUSED_BLK.

        // set the object's blockNum to INVALID_BLOCK (-1)
}
int IndInternal::getEntry(void *ptr, int indexNum) {
    // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
    //     return E_OUTOFBOUND.
    if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
    	return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
       int ret = loadBlockAndGetBufferPtr(&bufferPtr);
       if(ret!=SUCCESS)
       	return ret;

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.

    // typecast the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    /*
    - copy the entries from the indexNum`th entry to *internalEntry
    - make sure that each field is copied individually as in the following code
    - the lChild and rChild fields of InternalEntry are of type int32_t
    - int32_t is a type of int that is guaranteed to be 4 bytes across every
      C++ implementation. sizeof(int32_t) = 4
    */

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
       from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
    memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
    memcpy(&(internalEntry->rChild), entryPtr + 20, 4);

    return SUCCESS;
}
int IndLeaf::getEntry(void *ptr, int indexNum) {

    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
    if(indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
    	return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
       int ret = loadBlockAndGetBufferPtr(&bufferPtr);
       if(ret!=SUCCESS)
       	return ret;

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.

    // copy the indexNum'th Index entry in buffer to memory ptr using memcpy

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

    return SUCCESS;
}
/*int IndLeaf::setEntry(void *ptr, int indexNum) {
  if(indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
    	return E_OUTOFBOUND;
  unsigned char *bufferPtr;
    
       int ret = loadBlockAndGetBufferPtr(&bufferPtr);
       if(ret!=SUCCESS)
       	return ret;
   unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
   memcpy(entryPtr,(struct Index*)ptr,LEAF_ENTRY_SIZE);
   ret = StaticBuffer::setDirtyBit(this->blockNum);
  if(ret!=SUCCESS)
    return ret;
  return SUCCESS; 	
}*/
int IndLeaf::setEntry(void *ptr, int indexNum)
{
    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
    if (indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
        return E_OUTOFBOUND;

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }

    // copy the Index at ptr to indexNum'th entry in the buffer using memcpy

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    struct Index *index = (struct Index *)ptr;
    memcpy(entryPtr, &(index->attrVal), sizeof(Attribute));
    memcpy(entryPtr + 16, &(index->block), sizeof(int));
    memcpy(entryPtr + 20, &(index->slot), sizeof(int));

    // update dirty bit using setDirtyBit()
    return StaticBuffer::setDirtyBit(this->blockNum);
    // if setDirtyBit failed, return the value returned by the call
}
int IndInternal::setEntry(void *ptr, int indexNum) {
  if(indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
    	return E_OUTOFBOUND;
  unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
       int ret = loadBlockAndGetBufferPtr(&bufferPtr);
       if(ret!=SUCCESS)
       	return ret;
  struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(entryPtr, &(internalEntry->lChild),4);
    memcpy(entryPtr + 4,&(internalEntry->attrVal), ATTR_SIZE);
    memcpy(entryPtr + 20,&(internalEntry->rChild), 4);
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret!=SUCCESS)
      return ret;
  
    return SUCCESS;
}
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;
    if (attrType != NUMBER)
        diff = strcmp(attr1.sVal, attr2.sVal);

    else
        diff = attr1.nVal - attr2.nVal;

    
    if (diff > 0 )
      return 1;
    if (diff < 0 )
      return -1;
    //else if (diff == 0 )
      return 0;
    
}


