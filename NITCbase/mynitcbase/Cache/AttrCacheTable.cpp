#include "AttrCacheTable.h"
#include<iostream>
#include <cstring>
AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {
	*attrCatBuf = entry->attrCatEntry;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
/* returns the attribute with name `attrName` for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {

  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (!strcmp(attrName,entry->attrCatEntry.attrName)) {
	*attrCatBuf = entry->attrCatEntry;
	attrCache[relId]->dirty=true;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
int AttrCacheTable::setAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {
	entry->attrCatEntry = *attrCatBuf;
	entry->dirty=true;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry *attrCatBuf)
{
	if(relId < 0 || relId >= MAX_OPEN)
	{
		return E_OUTOFBOUND;
	}
	if(attrCache[relId] == nullptr)
	{
		return E_RELNOTOPEN;
	}
	for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (!strcmp(attrName,entry->attrCatEntry.attrName)) {
	 entry->attrCatEntry = *attrCatBuf ;
	 entry->dirty=true;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],
                                          AttrCatEntry* attrCatEntry) {
                                          
  strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
  strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
  attrCatEntry->attrType = (int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
  attrCatEntry->primaryFlag = (bool)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
  attrCatEntry->rootBlock = (int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
  attrCatEntry->offset = (int)record[ATTRCAT_OFFSET_INDEX].nVal;

}
void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry* attrCatEntry,union Attribute record[ATTRCAT_NO_ATTRS]) {
                                          
  strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,attrCatEntry->relName);
  strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal,attrCatEntry->attrName);
  record[ATTRCAT_ATTR_TYPE_INDEX].nVal = (int)attrCatEntry->attrType;
  record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = (bool)attrCatEntry->primaryFlag;
  record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = (int)attrCatEntry->rootBlock;
  record[ATTRCAT_OFFSET_INDEX].nVal = (int)attrCatEntry->offset;

}
int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (!strcmp(attrName,entry->attrCatEntry.attrName)) {
	entry->searchIndex = *searchIndex;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
int AttrCacheTable::setSearchIndex(int relId, int attrOffset, IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {
	entry->searchIndex = *searchIndex;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]) {
  IndexId indexId = {-1,-1};
  return AttrCacheTable::setSearchIndex(relId,attrName,&indexId);
}
int AttrCacheTable::resetSearchIndex(int relId, int attrOffset) {
  IndexId indexId = {-1,-1};
  return AttrCacheTable::setSearchIndex(relId,attrOffset,&indexId);
}
int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (!strcmp(attrName,entry->attrCatEntry.attrName)) {
	*searchIndex = entry->searchIndex;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
int AttrCacheTable::getSearchIndex(int relId, int attrOffset, IndexId *searchIndex) {

  if(relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {
	*searchIndex = entry->searchIndex;
	return SUCCESS;
    }
  }
  return E_ATTRNOTEXIST;
}
