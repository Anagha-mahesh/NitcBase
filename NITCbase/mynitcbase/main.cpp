#include<iostream>
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
  
  
  // Stage 1:
  /*unsigned char buffer3[BLOCK_SIZE];
  char values[10];
  Disk::readBlock(buffer3,0);
  memcpy(values,buffer3,10);
  for(int x:values)
  std::cout<<int(x)<<std::endl;
  
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer,7000);
  
  char message[]="Hello";
  memcpy(buffer+20,message,6);
  Disk::writeBlock(buffer,7000);
  
  unsigned char buffer2[BLOCK_SIZE];
  Disk::readBlock(buffer2,7000);
  char messageout[6];
  memcpy(messageout,buffer2+20,6);
  std::cout << messageout<<std::endl;*/
  
  
  //Stage 2:
  /*RecBuffer relCatBuffer(RELCAT_BLOCK);
  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;
 
  relCatBuffer.getHeader(&relCatHeader);
  int attrbnum=ATTRCAT_BLOCK,flag=0;
  for(int i=0;i<relCatHeader.numEntries;i++)
  {
  	
  	Attribute relCatRecord[RELCAT_NO_ATTRS];
  	relCatBuffer.getRecord(relCatRecord,i);
  	printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
  	while(attrbnum!=-1)
  	{
  		RecBuffer attrCatBuffer(attrbnum);
  		attrCatBuffer.getHeader(&attrCatHeader);
  		flag=0;
  		for(int j=0;j<attrCatHeader.numEntries;j++)
		  {
		  	//std::cout<<j<<std::endl;
		  	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
		  	attrCatBuffer.getRecord(attrCatRecord,j);
		  	
		  	if(!strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relCatRecord[RELCAT_REL_NAME_INDEX].sVal))
		  	{
		  		const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
		  		printf(" %s: %s\n",attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrType);
		  		flag=j;
		  		//std::cout<<flag;
		  		if(!strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,"Students")&&!strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Class"))
		  		{
		  			unsigned char buffer1[BLOCK_SIZE];
  					Disk::readBlock(buffer1,attrbnum);
		  			char val[6]="Batch";
		  			memcpy(buffer1+52+(16*6*j+16),val,5);
		  			Disk::writeBlock(buffer1,attrbnum);
		  		}
		  	}
		  	
	  		
		  }
	  	if(flag==attrCatHeader.numEntries-1)
	  	attrbnum=attrCatHeader.rblock;
	  	else
	  	{printf("\n");
	  	break;}
	  	
  	}
  		
   }*/
   
   //Stage 3:
   /*for (int i = 0; i <= 2; i++) 
   { // i.e., RELCAT_RELID and ATTRCAT_RELID 
	    RelCatEntry relCatEntry;
	    RelCacheTable::getRelCatEntry(i, &relCatEntry); 
	    printf("Relation: %s\n", relCatEntry.relName); 
    	for (int j = 0; j < relCatEntry.numAttrs; ++j) 
    	{ 
    
	    AttrCatEntry attrCatEntry; 
	    AttrCacheTable::getAttrCatEntry(i, j, &attrCatEntry); 
	    
	    const char *attrType = attrCatEntry.attrType == NUMBER ? "NUM" : "STR";
	    printf(" %s: %s\n", attrCatEntry.attrName, attrType); 
	} 
	std::cout<<"\n";
    } */
    
  return FrontendInterface::handleFrontend(argc, argv);
  //return 0;
}
