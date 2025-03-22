
#include "BlockAccess.h"
#include <iostream>
#include <cstring>
int countlinear = 0;
RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    struct RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    
    // let block and slot denote the record id of the record being currently checked
    int block=-1,slot=-1;
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relBuf;
        RelCacheTable::getRelCatEntry(relId,&relBuf);
        block=relBuf.firstBlk;
        slot=0;
        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
        block=prevRecId.block;
        slot=prevRecId.slot+1;
        // block = search index's block
        // slot = search index's slot + 1
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId,&relCatBuf);
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer blk(block);
        HeadInfo header;
        blk.getHeader(&header);
        unsigned char slotMap[header.numSlots];
        Attribute record[header.numAttrs];
        
        blk.getSlotMap(slotMap);
        //blk.getRecord(record,slot);
        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function
        if(slot>=relCatBuf.numSlotsPerBlk)
        // If slot >= the number of slots per block(i.e. no more slots in this block)
        {
           block=header.rblock;
           slot=0;
            // update block = right block of block
            // update slot = 0
            continue;  // continue to the beginning of this while loop
        }
        if(slotMap[slot]==SLOT_UNOCCUPIED)
        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        {
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }
        blk.getRecord(record,slot);
        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
        int offset=attrCatBuf.offset;
        
        /* use the attribute offset to get the value of the attribute from
           current record */

        int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
	cmpVal=compareAttrs(record[offset],attrVal,attrCatBuf.attrType);
	countlinear++;
        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            
            */
            RecId newRecId = {block,slot};
         
	    RelCacheTable::setSearchIndex(relId,&newRecId);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    if(ret!=SUCCESS)
    	return ret;
    int rootBlock = attrCatEntry.rootBlock;
    if(rootBlock==-1)
    {
    	recId=linearSearch(relId,attrName,attrVal,op);
    }
    else
    {
    	recId=BPlusTree::bPlusSearch(relId,attrName,attrVal,op);
    }
    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
    //recId = linearSearch(relId,attrName,attrVal,op);
    if(recId.block==-1 && recId.slot==-1)
    {
    	return E_NOTFOUND;
    }
    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    RecBuffer recordBuffer(recId.block);
    recordBuffer.getRecord(record,recId.slot);
    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    //if the relation to delete is either Relation Catalog or Attribute Catalog,
    if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME))
         return E_NOTPERMITTED;
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy((char *)relNameAttr.sVal,(const char*)relName);
    char temp[]="RelName";
    RecId relCatRecId = linearSearch(RELCAT_RELID,temp,relNameAttr,EQ);
    if(relCatRecId.block == -1 && relCatRecId.slot == -1)
    	return E_RELNOTEXIST;

    //  linearSearch on the relation catalog for RelName = relNameAttr

    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    RecBuffer relCatBuffer(relCatRecId.block);
    relCatBuffer.getRecord(relCatEntryRecord,relCatRecId.slot);

    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    /*
     Delete all the record blocks of the relation
    */
    int currBlockNum= firstBlock;
    while(currBlockNum!=-1)
    {
    	RecBuffer currBlock(currBlockNum);
    	HeadInfo currBlockHeader;
    	currBlock.getHeader(&currBlockHeader);
    	currBlockNum=currBlockHeader.rblock;
    	currBlock.releaseBlock();
    }
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1


    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributesDeleted = 0;

    while(true) {
        RecId attrCatRecId = linearSearch(ATTRCAT_RELID,temp,relNameAttr,EQ);
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;
	if(attrCatRecId.block == -1 && attrCatRecId.slot == -1)
	{
		break;
	}
        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        // get the header of the block
        // get the record corresponding to attrCatRecId.slot
	RecBuffer attrBlock(attrCatRecId.block);
	HeadInfo attrBlockHeader;
	attrBlock.getHeader(&attrBlockHeader);
	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
	attrBlock.getRecord(attrCatRecord,attrCatRecId.slot);
        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        int rootBlock = attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        // (This will be used later to delete any indexes if it exists)

        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
	unsigned char slotMap[attrBlockHeader.numSlots];
	attrBlock.getSlotMap(slotMap);
	slotMap[attrCatRecId.slot]=SLOT_UNOCCUPIED;
	attrBlock.setSlotMap(slotMap);
        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
	attrBlockHeader.numEntries--;
	attrBlock.setHeader(&attrBlockHeader);
        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if (attrBlockHeader.numEntries==0) {
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */
	    RecBuffer prevBlock(attrBlockHeader.lblock);
	    HeadInfo leftHeader;
	    prevBlock.getHeader(&leftHeader);
	    leftHeader.rblock=attrBlockHeader.rblock;
	    prevBlock.setHeader(&leftHeader);
            // create a RecBuffer for lblock and call appropriate methods

            if (attrBlockHeader.rblock!=-1) {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                RecBuffer nextBlock(attrBlockHeader.rblock);
                HeadInfo rightHeader;
                nextBlock.getHeader(&rightHeader);
                rightHeader.lblock=attrBlockHeader.lblock;
                nextBlock.setHeader(&rightHeader);
                // create a RecBuffer for rblock and call appropriate methods

            } else {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
                   RelCatEntry relCatBuffer;
                   RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatBuffer);
                   relCatBuffer.lastBlk = attrBlockHeader.lblock;
                   RelCacheTable::setRelCatEntry(ATTRCAT_RELID,&relCatBuffer);
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)
	    attrBlock.releaseBlock();
	    RecId nextSearchIndex = {attrBlockHeader.rblock,0};
	    RelCacheTable::setSearchIndex(ATTRCAT_RELID,&nextSearchIndex);
            // call releaseBlock()
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        if (rootBlock != -1) {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
            BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block

    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */
       HeadInfo relCatHeader;
       relCatBuffer.getHeader(&relCatHeader);
       relCatHeader.numEntries--;
       relCatBuffer.setHeader(&relCatHeader);
    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
       unsigned char slotMap[relCatHeader.numSlots];
       relCatBuffer.getSlotMap(slotMap);
       slotMap[relCatRecId.slot]=SLOT_UNOCCUPIED;
       relCatBuffer.setSlotMap(slotMap);

    /*** Updating the Relation Cache Table ***/
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID,&relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID,&relCatEntry);

    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatEntry);
    relCatEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID,&relCatEntry);

    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)

    return SUCCESS;
}
int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal,newName);
    char temp[]="RelName";
    RecId searchIndex=linearSearch(RELCAT_RELID,temp,newRelationName,EQ);
    // search the relation catalog for an entry with "RelName" = newRelationName
    if(searchIndex.block!=-1 && searchIndex.slot!=-1)
    {
      return E_RELEXIST;
    }
    
    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;


    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);
    // search the relation catalog for an entry with "RelName" = oldRelationName
    searchIndex=linearSearch(RELCAT_RELID,temp,oldRelationName,EQ);
    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if(searchIndex.block==-1 && searchIndex.slot==-1)
    {
      return E_RELNOTEXIST;
    }
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer relCatBuffer(RELCAT_BLOCK);
    Attribute RelCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(RelCatRecord,searchIndex.slot);
    strcpy(RelCatRecord[RELCAT_REL_NAME_INDEX].sVal,newName);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    relCatBuffer.setRecord(RelCatRecord,searchIndex.slot);
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */
     RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    int numAttrs=RelCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    for(int i=0;i<numAttrs;i++)
    {
      searchIndex=linearSearch(ATTRCAT_RELID,temp,oldRelationName,EQ);
      RecBuffer attrCatBuffer(searchIndex.block);
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord,searchIndex.slot);
      strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,newName);
      attrCatBuffer.setRecord(attrCatRecord,searchIndex.slot);
    }
    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);
    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    char temp[]="RelName";
    RecId searchIndex=linearSearch(RELCAT_RELID,temp,relNameAttr,EQ);
    if(searchIndex.block==-1 && searchIndex.slot==-1)
    {
      return E_RELNOTEXIST;
    }
    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
         searchIndex=linearSearch(ATTRCAT_RELID,temp,relNameAttr,EQ);
        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if(searchIndex.block==-1 && searchIndex.slot==-1)
        {
          break;
        }
        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
         RecBuffer attrCatBuffer(searchIndex.block);
         Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
         attrCatBuffer.getRecord(attrCatEntryRecord,searchIndex.slot);
        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
         if(!strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName))
         {
           attrToRenameRecId = searchIndex;
           break;
         }
         if(!strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName))
         {
           return E_ATTREXIST;
         }
        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
    }
     if(attrToRenameRecId.block == -1 &&  attrToRenameRecId.slot == -1)
     {
       return E_ATTRNOTEXIST;
     }
    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
     RecBuffer attrCatBuffer(attrToRenameRecId.block);
     Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
     attrCatBuffer.getRecord(attrCatRecord,attrToRenameRecId.slot);
     strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
     attrCatBuffer.setRecord(attrCatRecord,attrToRenameRecId.slot);
    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    //std::cout<<"insertenter\n";
    //std::cout<<relId<<std::endl;
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(relId,&relCatBuffer);
    int blockNum = relCatBuffer.firstBlk;
	//std::cout<<blockNum<<std::endl;
    // rec_id will be used to store where the new record will be inserted
    RecId rec_id;
    rec_id.slot=-1;
    rec_id.block=-1;

    int numOfSlots = relCatBuffer.numSlotsPerBlk;
    int numOfAttributes = relCatBuffer.numAttrs;

    int prevBlockNum = -1;

    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
	RecBuffer blockBuffer(blockNum);
        // get header of block(blockNum) using RecBuffer::getHeader() function
        HeadInfo blockHeader;
	blockBuffer.getHeader(&blockHeader);
        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
	unsigned char slotMap[blockHeader.numSlots];
	blockBuffer.getSlotMap(slotMap);
        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        
        int slotIndex=0;
	for(slotIndex=0;slotIndex<blockHeader.numSlots;slotIndex++)
	{
		if(slotMap[slotIndex]==SLOT_UNOCCUPIED)
		{
			//rec_id = RecId{blockNum,slotIndex};
			rec_id.block=blockNum;
			rec_id.slot=slotIndex;
			break;
		}
	}
	if(rec_id.block!=-1 && rec_id.slot!=-1)
		break;
	prevBlockNum = blockNum;
	blockNum=blockHeader.rblock;
        
    }
    //std::cout<<blockNum<<std::endl;
    //std::cout<<bool(rec_id.block == -1 && rec_id.slot == -1)<<std::endl;
    //std::cout<<blockNum<<std::endl;
    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if(rec_id.block == -1 && rec_id.slot == -1)
    {
    //std::cout<<"here";
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
	if(relId==RELCAT_RELID)
		return E_MAXRELATIONS;
	
        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        
        RecBuffer blockBuffer;
        blockNum=blockBuffer.getBlockNum();
        
        //std::cout<<blockNum<<std::endl;
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        if (blockNum == E_DISKFULL) {
            return E_DISKFULL;
        }
	rec_id.block=blockNum;
	rec_id.slot=0;
        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0

        HeadInfo blockHeader;
        blockHeader.blockType = REC;
        blockHeader.pblock = -1;
        blockHeader.rblock = -1;
        blockHeader.lblock = prevBlockNum;
        blockHeader.numSlots = numOfSlots;
        blockHeader.numAttrs = numOfAttributes;
        blockHeader.numEntries = 0;
        blockBuffer.setHeader(&blockHeader);

	unsigned char slotMap[numOfSlots];
	for(int i=0;i<numOfSlots;i++)
	{
		slotMap[i]=SLOT_UNOCCUPIED;
	}
	blockBuffer.setSlotMap(slotMap);
        if(prevBlockNum != -1)
        {
            // create a RecBuffer object for prevBlockNum
            RecBuffer prevBlockBuffer(prevBlockNum);
            HeadInfo prevHeader;
            prevBlockBuffer.getHeader(&prevHeader);
            prevHeader.rblock=blockNum;
            prevBlockBuffer.setHeader(&prevHeader);
            // get the header of the block prevBlockNum and
            // update the rblock field of the header to the new block
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            relCatBuffer.firstBlk=blockNum;
            RelCacheTable::setRelCatEntry(relId,&relCatBuffer);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relCatBuffer.lastBlk = blockNum;
        RelCacheTable::setRelCatEntry(relId,&relCatBuffer);
    }

    // create a RecBuffer object for rec_id.block
    RecBuffer buffer(rec_id.block);
    buffer.setRecord(record,rec_id.slot);
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    unsigned char slotmap[numOfSlots];
    buffer.getSlotMap(slotmap);
    slotmap[rec_id.slot]=SLOT_OCCUPIED;
    buffer.setSlotMap(slotmap);
    
    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    HeadInfo head;
    buffer.getHeader(&head);
    head.numEntries++;
    buffer.setHeader(&head);
    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relCatBuffer.numRecs++;
    RelCacheTable::setRelCatEntry(relId,&relCatBuffer);
    /* B+ Tree Insertions */
    // (the following section is only relevant once indexing has been implemented)

    int flag = SUCCESS;
    // Iterate over all the attributes of the relation
    for(int attrOffset =0;attrOffset<numOfAttributes;attrOffset++)
    {
        // get the attribute catalog entry for the attribute from the attribute cache
        // (use AttrCacheTable::getAttrCatEntry() with args relId and attrOffset)
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId,attrOffset,&attrCatBuf); 
        int rootBlock = attrCatBuf.rootBlock;

        // get the root block field from the attribute catalog entry

        // if index exists for the attribute(i.e. rootBlock != -1)
        if(rootBlock!=-1)
        {
            /* insert the new record into the attribute's bplus tree using
             BPlusTree::bPlusInsert()*/
            int retVal = BPlusTree::bPlusInsert(relId, attrCatBuf.attrName,
                                                record[attrOffset], rec_id);

            if (retVal == E_DISKFULL) {
                //(index for this attribute has been destroyed)
                flag = E_INDEX_BLOCKS_RELEASED;
            }
        }
    }

    return flag;
    //return SUCCESS;
}

/*
NOTE: the caller is expected to allocate space for the argument `record` based
      on the size of the relation. This function will only copy the result of
      the projection onto the array pointed to by the argument.
*/
int BlockAccess::project(int relId, Attribute *record) {
    // get the previous search index of the relation relId from the relation
    // cache (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block, slot;

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relCatBuf;
        RelCacheTable::getRelCatEntry(relId,&relCatBuf);
        block = relCatBuf.firstBlk;
        slot=0;

        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (a project/search operation is already in progress)

        // block = previous search index's block
        // slot = previous search index's slot + 1
        block = prevRecId.block;
        slot = prevRecId.slot+1;
    }


    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        // create a RecBuffer object for block (using appropriate constructor!)
        RecBuffer currBuffer(block);
        HeadInfo currHeader;
	currBuffer.getHeader(&currHeader);
	unsigned char slotMap[currHeader.numSlots];
	currBuffer.getSlotMap(slotMap);
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        if(slot>=currHeader.numSlots)
        {
            // (no more slots in this block)
            // update block = right block of block
            // update slot = 0
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
            block = currHeader.rblock;
            slot = 0;
            
        }
        else if (slotMap[slot]==SLOT_UNOCCUPIED)
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)

            // increment slot
            slot++;
        }
        else {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1){
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId;
    nextRecId.block=block;
    nextRecId.slot=slot;

    // set the search index to nextRecId using RelCacheTable::setSearchIndex
    RelCacheTable::setSearchIndex(relId,&nextRecId);

    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recordBuffer(block);
    recordBuffer.getRecord(record,slot);

    return SUCCESS;
}
