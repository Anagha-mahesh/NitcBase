#include "OpenRelTable.h"
#include<cstdlib>
#include <cstring>
#include<iostream>
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
void freeLinkedList(AttrCacheEntry *head)
{
    for (AttrCacheEntry *it = head, *next; it != nullptr; it = next)
    {
        next = it->next;
        free(it);
    }
}
int OpenRelTable::closeRel(int relId)
{
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;

    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (tableMetaInfo[relId].free == true)
        return E_RELNOTOPEN;

    RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];

    if (relCacheEntry && relCacheEntry->dirty == true)
    {
        RecBuffer relCatBlock((relCacheEntry->recId).block);

        RelCatEntry relCatEntry = relCacheEntry->relCatEntry;
        Attribute record[RELCAT_NO_ATTRS];

        RelCacheTable::relCatEntryToRecord(&relCatEntry, record);

        relCatBlock.setRecord(record, (relCacheEntry->recId).slot);
    }

    free(relCacheEntry);
    RelCacheTable::relCache[relId] = nullptr;

    for (auto attrCacheEntry = AttrCacheTable::attrCache[relId]; attrCacheEntry != nullptr; attrCacheEntry = attrCacheEntry->next)
    {
        if (attrCacheEntry && attrCacheEntry->dirty == true)
        {
            RecBuffer attrCatBlock((attrCacheEntry->recId).block);

            AttrCatEntry attrCatEntry = attrCacheEntry->attrCatEntry;
            Attribute record[ATTRCAT_NO_ATTRS];

            AttrCacheTable::attrCatEntryToRecord(&attrCatEntry, record);

            attrCatBlock.setRecord(record, (attrCacheEntry->recId).slot);
        }
    }

    freeLinkedList(AttrCacheTable::attrCache[relId]);
    AttrCacheTable::attrCache[relId] = nullptr;

    tableMetaInfo[relId].free = true;
    return SUCCESS;
}
OpenRelTable::OpenRelTable() {

  for (int i = 0; i < MAX_OPEN; ++i) 
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free = true;
  }

  /************ Setting up Relation Cache entries ************/
  
  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
  
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

  tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
  tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);
  //Stage 3 Exercise
  /*relCatBlock.getRecord(relCatRecord,2);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = 2;
  
  RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[2]) = relCacheEntry;*/
  /************ Setting up Attribute cache entries ************/
  
  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);

  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  struct AttrCacheEntry *head=nullptr;struct AttrCacheEntry *prev=nullptr;
  for(int i=0;i<RELCAT_NO_ATTRS;i++)
  {
    struct AttrCacheEntry *attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    attrCatBlock.getRecord(attrCatRecord,i);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->next=nullptr;
    if(prev==nullptr)
    {
      head=attrCacheEntry;
      prev=head;
    }
    else
    {
      prev->next=attrCacheEntry;
      prev=prev->next;
    }
  }

  AttrCacheTable::attrCache[RELCAT_RELID] = head;

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  head=nullptr;prev=nullptr;
  for(int i=6;i<12;i++)
  {
    struct AttrCacheEntry *attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    attrCatBlock.getRecord(attrCatRecord,i);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    //std::cout<<attrCacheEntry->attrCatEntry.attrName;
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->next=nullptr;
    if(prev==nullptr)
    {
      head=attrCacheEntry;
      prev=head;
      
    }
    else
    {
      prev->next=attrCacheEntry;
      prev=prev->next;
      
    }
  }
  (AttrCacheTable::attrCache[ATTRCAT_RELID]) = head;
  
  //Stage 3:Exercise
  /*head=nullptr;prev=nullptr;
  for(int i=12;i<16;i++)
  {
    struct AttrCacheEntry *attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    attrCatBlock.getRecord(attrCatRecord,i);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    //std::cout<<attrCacheEntry->attrCatEntry.attrName;
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->next=nullptr;
    if(prev==nullptr)
    {
      head=attrCacheEntry;
      prev=head;
      
    }
    else
    {
      prev->next=attrCacheEntry;
      prev=prev->next;
      
    }
  }
  (AttrCacheTable::attrCache[2]) = head;*/

}
/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
  for(int i=0;i<MAX_OPEN;i++)
  {
    if(!strcmp(relName,tableMetaInfo[i].relName) && tableMetaInfo[i].free==false)
      return i;
  }
  /*if(!strcmp(relName,RELCAT_RELNAME))
  {
    return RELCAT_RELID;
  }
  if(!strcmp(relName,ATTRCAT_RELNAME))
    return ATTRCAT_RELID;
  if(!strcmp(relName,"Students"))
    return 2;*/
  // if relname is RELCAT_RELNAME, return RELCAT_RELID
  // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID

  return E_RELNOTOPEN;
}
int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  
  int relId=getRelId(relName);
  if(relId>=0){
    return relId;
  }
  relId=getFreeOpenRelTableEntry();
  
  if (relId==E_CACHEFULL){
    return E_CACHEFULL;
  }

  Attribute attrVal;
  strcpy(attrVal.sVal,relName);
  char temp[]="RelName";
  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID,temp,attrVal,EQ);

  if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  
  RecBuffer relBuffer(relcatRecId.block);
  Attribute relRecord[RELCAT_NO_ATTRS];
  relBuffer.getRecord(relRecord,relcatRecId.slot);
  RelCacheEntry *relCacheBuffer = nullptr;
  relCacheBuffer= (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(relRecord,&(relCacheBuffer->relCatEntry));
  relCacheBuffer->recId.block=relcatRecId.block;
  relCacheBuffer->recId.slot=relcatRecId.slot;
  RelCacheTable::relCache[relId]=relCacheBuffer;
  //std::cout<<relCacheBuffer->relCatEntry.numAttrs;
 RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry* listHead=nullptr,*prev=nullptr;
  int numAttrs=relCacheBuffer->relCatEntry.numAttrs;
  
  for(int i=0;i<numAttrs;i++)
  //while(numAttrs--)
  {
      
      RecId attrcatRecId=BlockAccess::linearSearch(ATTRCAT_RELID,temp,attrVal,EQ);
     
      RecBuffer attrCatBlock(attrcatRecId.block);
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      struct AttrCacheEntry *attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    attrCatBlock.getRecord(attrCatRecord,attrcatRecId.slot);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block=attrcatRecId.block;
    attrCacheEntry->recId.slot=attrcatRecId.slot;
    attrCacheEntry->next=nullptr;
    if(prev==nullptr)
    {
      listHead=attrCacheEntry;
      prev=listHead;
    }
    else
    {
      prev->next=attrCacheEntry;
      prev=prev->next;
    }
  

  AttrCacheTable::attrCache[relId] = listHead;
     
      
  }

  
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName,relName);
  
  return relId;
}
/*int OpenRelTable::closeRel(int relId) {
  if (relId==RELCAT_RELID ||relId==ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free==true) {
    return E_RELNOTOPEN;
  }
  RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];
  if (relCacheEntry->dirty==true && relCacheEntry) 
  {

    
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry),relCatRecord);

    // declaring an object of RecBuffer class to write back to the buffer
    RecId recId = RelCacheTable::relCache[relId]->recId;
    RecBuffer relCatBlock(recId.block);
    relCatBlock.setRecord(relCatRecord,recId.slot);
    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
  }

  

  // free the memory allocated in the attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  // (because we are not modifying the attribute cache at this stage,
  // write-back is not required. We will do it in subsequent
  // stages when it becomes needed)


  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  free(RelCacheTable::relCache[relId]);
  RelCacheTable::relCache[relId]=nullptr;
  struct AttrCacheEntry* entry = AttrCacheTable::attrCache[relId];
    while (entry != nullptr) {
    if(entry->dirty==true && entry)
    {
    	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    	AttrCacheTable::attrCatEntryToRecord(&(entry->attrCatEntry),attrCatRecord);
    	RecId recId = AttrCacheTable::attrCache[relId]->recId;
    	RecBuffer attrCatBlock(recId.block);
    	attrCatBlock.setRecord(attrCatRecord,recId.slot);
    	
    }
      struct AttrCacheEntry* next = entry->next;
      free(entry);
      entry = next;
    }
    //free(AttrCacheTable::attrCache[relId]);
  tableMetaInfo[relId].free = true;
  
  
  AttrCacheTable::attrCache[relId]=nullptr;
  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr

  return SUCCESS;
}*/
int OpenRelTable::getFreeOpenRelTableEntry() {
  for(int i=0;i<MAX_OPEN;i++)
  {
    if(tableMetaInfo[i].free==true)
      return i;
  }
  
  return E_CACHEFULL;
  // if found return the relation id, else return E_CACHEFULL.
}
OpenRelTable::~OpenRelTable() {
  // close all open relations (from rel-id = 2 onwards. Why?)
  for (int i = 2; i < MAX_OPEN; ++i) {
    if (!tableMetaInfo[i].free) {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }
  if(RelCacheTable::relCache[ATTRCAT_RELID]->dirty)
  {
  	RelCatEntry relCatBuffer;
  	RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatBuffer);
  	Attribute relCatRecord[RELCAT_NO_ATTRS];
  	RelCacheTable::relCatEntryToRecord(&relCatBuffer,relCatRecord);
  	RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;
  	RecBuffer relCatBlock(recId.block);
  	relCatBlock.setRecord(relCatRecord,recId.slot);
  	
  }
  //free(RelCacheTable::relCache[ATTRCAT_RELID]);
  if(RelCacheTable::relCache[RELCAT_RELID]->dirty)
  {
  	RelCatEntry relCatBuffer;
  	RelCacheTable::getRelCatEntry(RELCAT_RELID,&relCatBuffer);
  	Attribute relCatRecord[RELCAT_NO_ATTRS];
  	RelCacheTable::relCatEntryToRecord(&relCatBuffer,relCatRecord);
  	RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
  	RecBuffer relCatBlock(recId.block);
  	relCatBlock.setRecord(relCatRecord,recId.slot);
  	
  }
  //free(RelCacheTable::relCache[RELCAT_RELID]);
  // free all the memory that you allocated in the constructor
  for (int i = 0; i <= 1; ++i) {
    if (RelCacheTable::relCache[i] != nullptr) {
      free(RelCacheTable::relCache[i]);
      RelCacheTable::relCache[i] = nullptr;
    }

    struct AttrCacheEntry* entry = AttrCacheTable::attrCache[i];
    while (entry != nullptr) {
      struct AttrCacheEntry* next = entry->next;
      free(entry);
      entry = next;
    }
    AttrCacheTable::attrCache[i] = nullptr;
  }
}
