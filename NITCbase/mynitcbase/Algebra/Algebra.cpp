#include "Algebra.h"
#include<iostream>
#include <cstring>
extern int countlinear;

bool isNumber(char *str);
/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::join(
    char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE],
    char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE],
    char attribute2[ATTR_SIZE])
{

    // get the srcRelation1's rel-id using OpenRelTable::getRelId() method
    int srcRelId1 = OpenRelTable::getRelId(srcRelation1);

    // get the srcRelation2's rel-id using OpenRelTable::getRelId() method
    int srcRelId2 = OpenRelTable::getRelId(srcRelation2);

    // if either of the two source relations is not open
    //     return E_RELNOTOPEN
    if (srcRelId1 == E_RELNOTOPEN || srcRelId2 == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    // get the attribute catalog entries for the following from the attribute cache
    // (using AttrCacheTable::getAttrCatEntry())
    // - attrCatEntry1 = attribute1 of srcRelation1
    // - attrCatEntry2 = attribute2 of srcRelation2
    // if either of the two attributes is not found in the respective relations
    //     return E_ATTRNOTEXIST
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId1, attribute1, &attrCatEntry1);
    if (ret != SUCCESS)
    {
        return ret;
    }
    ret = AttrCacheTable::getAttrCatEntry(srcRelId2, attribute2, &attrCatEntry2);
    if (ret != SUCCESS)
    {
        return ret;
    }
    // if attribute1 is not present in srcRelation1 or attribute2 is not
    // present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
    //     return E_ATTRNOTEXIST.

    // if attribute1 and attribute2 are of different types return E_ATTRTYPEMISMATCH
    if (attrCatEntry1.attrType != attrCatEntry2.attrType)
    {
        return E_ATTRTYPEMISMATCH;
    }

    // iterate through all the attributes in both the source relations and check if
    // there are any other pair of attributes other than join attributes
    // (i.e. attribute1 and attribute2) with duplicate names in srcRelation1 and
    // srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    // If yes, return E_DUPLICATEATTR

    // get the relation catalog entries for the relations from the relation cache
    // (use RelCacheTable::getRelCatEntry() function)
    RelCatEntry relCatEntry1, relCatEntry2;
    ret = RelCacheTable::getRelCatEntry(srcRelId1, &relCatEntry1);
    if (ret != SUCCESS)
    {
        return ret;
    }
    ret = RelCacheTable::getRelCatEntry(srcRelId2, &relCatEntry2);
    if (ret != SUCCESS)
    {
        return ret;
    }

    int numOfAttributes1 = relCatEntry1.numAttrs;
    int numOfAttributes2 = relCatEntry2.numAttrs;

    for (int i = 0; i < numOfAttributes1; i++)
    {
        AttrCatEntry attrCatBuff1;
        AttrCacheTable::getAttrCatEntry(srcRelId1, i, &attrCatBuff1);
        for (int j = 0; j < numOfAttributes2; j++)
        {
            AttrCatEntry attrCatBuff2;
            AttrCacheTable::getAttrCatEntry(srcRelId2, j, &attrCatBuff2);

            if (i == attrCatEntry1.offset && j == attrCatEntry2.offset)
            {
                continue;
            }

            if (strcmp(attrCatBuff1.attrName, attrCatBuff2.attrName) == 0)
            {
                return E_DUPLICATEATTR;
            }
        }
    }

    // if rel2 does not have an index on attr2
    //     create it using BPlusTree:bPlusCreate()
    //     if call fails, return the appropriate error code
    //     (if your implementation is correct, the only error code that will
    //      be returned here is E_DISKFULL)
    if (attrCatEntry2.rootBlock == -1)
    {
        int ret = BPlusTree::bPlusCreate(srcRelId2, attribute2);
        if (ret != SUCCESS)
        {
            return ret;
        }
    }

    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
    // Note: The target relation has number of attributes one less than
    // nAttrs1+nAttrs2 (Why?)

    // declare the following arrays to store the details of the target relation
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    // iterate through all the attributes in both the source relations and
    // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
    // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    int index = 0;
    for (int i = 0; i < numOfAttributes1; i++)
    {
        AttrCatEntry attrCatBuff1;
        AttrCacheTable::getAttrCatEntry(srcRelId1, i, &attrCatBuff1);
        strcpy(targetRelAttrNames[index], attrCatBuff1.attrName);
        targetRelAttrTypes[index] = attrCatBuff1.attrType;
        index++;
    }

    for (int i = 0; i < numOfAttributes2; i++)
    {
        AttrCatEntry attrCatBuff2;
        AttrCacheTable::getAttrCatEntry(srcRelId2, i, &attrCatBuff2);
        if (attrCatBuff2.offset == attrCatEntry2.offset)
        {
            continue;
        }
        strcpy(targetRelAttrNames[index], attrCatBuff2.attrName);
        targetRelAttrTypes[index] = attrCatBuff2.attrType;
        index++;
    }

    // create the target relation using the Schema::createRel() function
    // by providing appropriate arguments
    ret = Schema::createRel(targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes);

    // if createRel() returns an error, return that error
    if (ret != SUCCESS)
    {
        return ret;
    }

    // Open the targetRelation using OpenRelTable::openRel()
    int newRelId = OpenRelTable::openRel(targetRelation);

    // if openRel() fails (No free entries left in the Open Relation Table)
    if (newRelId < 0 || newRelId >= MAX_OPEN)
    {
        // delete target relation by calling Schema::deleteRel()
        // return the error code
        Schema::deleteRel(targetRelation);
        return newRelId;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];

    // this loop is to get every record of the srcRelation1 one by
    RelCacheTable::resetSearchIndex(srcRelId1);
    AttrCacheTable::resetSearchIndex(srcRelId1, attribute1);
    while (BlockAccess::project(srcRelId1, record1) == SUCCESS)
    {

        // reset the search index of `srcRelation2` in the relation cache
        // using RelCacheTable::resetSearchIndex()
        RelCacheTable::resetSearchIndex(srcRelId2);

        // reset the search index of `attribute2` in the attribute cache
        // using AttrCacheTable::resetSearchIndex()
        AttrCacheTable::resetSearchIndex(srcRelId2, attribute2);

        // this loop is to get every record of the srcRelation2 which satisfies
        // the following condition:
        // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
        while (BlockAccess::search(
                   srcRelId2, record2, attribute2, record1[attrCatEntry1.offset], EQ) == SUCCESS)
        {

            // copy srcRelation1's and srcRelation2's attribute values(except
            // for attribute2 in rel2) from record1 and record2 to targetRecord
            // respectively
            int index = 0;
            for (int i = 0; i < numOfAttributes1; i++)
            {
                targetRecord[index] = record1[i];
                index++;
            }

            for (int i = 0; i < numOfAttributes2; i++)
            {
                if (i == attrCatEntry2.offset)
                {
                    continue;
                }
                targetRecord[index] = record2[i];
                index++;
            }

            // insert the current record into the target relation by calling
            // BlockAccess::insert()

            ret = BlockAccess::insert(newRelId, targetRecord);

            if (ret != SUCCESS)
            {

                // close the target relation by calling OpenRelTable::closeRel()
                // delete targetRelation (by calling Schema::deleteRel())
                Schema::closeRel(targetRelation);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }

    // close the target relation by calling OpenRelTable::closeRel()
    return Schema::closeRel(targetRelation);
}
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel);      // we'll implement this later
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  AttrCatEntry attrCatEntry;
  int ret = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
  if(ret == E_ATTRNOTEXIST)
  {
    return E_ATTRNOTEXIST;
  }
  // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
  //    return E_ATTRNOTEXIST if it returns the error


  // Convert strVal (string) to an attribute of data type NUMBER or STRING 
  int type = attrCatEntry.attrType;
  Attribute attrVal;
  if (type == NUMBER) {
    if (isNumber(strVal)) {       // the isNumber() function is implemented below
      attrVal.nVal = atof(strVal);
    } else {
      return E_ATTRTYPEMISMATCH;
    }
  } else if (type == STRING) {
    strcpy(attrVal.sVal, strVal);
  }

  RelCatEntry relCatBuffer;
  RelCacheTable::getRelCatEntry(srcRelId,&relCatBuffer);
  int srcNumAttrs = relCatBuffer.numAttrs;
  char srcAttrNames[srcNumAttrs][ATTR_SIZE];
  int srcAttrTypes[srcNumAttrs];
  for(int i=0;i<srcNumAttrs;i++)
  {
    AttrCatEntry attrCatBuffer;
    AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatBuffer);
    strcpy(srcAttrNames[i],attrCatBuffer.attrName);
    srcAttrTypes[i]=attrCatBuffer.attrType;
  }
  
  ret = Schema::createRel(targetRel,srcNumAttrs,srcAttrNames,srcAttrTypes);
  if(ret!=SUCCESS)
  	return ret;
  int targetRelId = OpenRelTable::openRel(targetRel);
  if(targetRelId<0 || targetRelId>=MAX_OPEN)
  	return targetRelId;
  // Selecting records from the source relation
  Attribute record[srcNumAttrs];
  // Before calling the search function, reset the search to start from the first hit
  // using RelCacheTable::resetSearchIndex()
  RelCacheTable::resetSearchIndex(srcRelId);
  AttrCacheTable::resetSearchIndex(srcRelId,attr);
  while(BlockAccess::search(srcRelId,record,attr,attrVal,op)==SUCCESS)
  {
  	ret=BlockAccess::insert(targetRelId,record);
  	if(ret!=SUCCESS)
  	{
  		Schema::closeRel(targetRel);
  		Schema::deleteRel(targetRel);
  		return ret;
  	}
  }
  Schema::closeRel(targetRel);
  //std::cout<<"Linear count: "<<countlinear<<std::endl;
  //std::cout<<"B plus count: "<<countbplus<<std::endl;
  /*RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
  // get relCatEntry using RelCacheTable::getRelCatEntry()

  

  printf("|");
  for (int i = 0; i < relCatEntry.numAttrs; ++i) {
    AttrCatEntry attrCatEntry;
    // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
    AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
    printf(" %s |", attrCatEntry.attrName);
  }
  printf("\n");

  while (true) {
    RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

    if (searchRes.block != -1 && searchRes.slot != -1) {

      // get the record at searchRes using BlockBuffer.getRecord
      RecBuffer recBlock(searchRes.block);
      HeadInfo header;
      recBlock.getHeader(&header);
      Attribute rec[relCatEntry.numAttrs];
      recBlock.getRecord(rec,searchRes.slot);
      printf("|");
  for (int i = 0; i < relCatEntry.numAttrs; ++i) {
    AttrCatEntry attrCatEntry;
    // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
    AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
    
    //add condition to check if number or string
    if(attrCatEntry.attrType==NUMBER)
    {
      printf(" %d |", (int)rec[i].nVal);
    }
    else if(attrCatEntry.attrType==STRING)
    {
      printf(" %s |", rec[i].sVal);
    }
    
  }
  printf("\n");
      // print the attribute values in the same format as above

    } else {

      // (all records over)
      break;
    }
  }*/

  return SUCCESS;
}
int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    // return E_NOTPERMITTED;
    if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME))
      return E_NOTPERMITTED;
    // get the relation's rel-id using OpenRelTable::getRelId() method
    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN)
      return E_RELNOTOPEN;
    // if relation is not open in open relation table, return E_RELNOTOPEN
    // (check if the value returned from getRelId function call = E_RELNOTOPEN)
    // get the relation catalog entry from relation cache
    // (use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId,&relCatBuf);
    /* if relCatEntry.numAttrs != numberOfAttributes in relation,
       return E_NATTRMISMATCH */
    if(relCatBuf.numAttrs!=nAttrs)
    {
      return E_NATTRMISMATCH;
    }
    // let recordValues[numberOfAttributes] be an array of type union Attribute
    Attribute recordValues[nAttrs];
    /*
        Converting 2D char array of record values to Attribute array recordValues
     */
    // iterate through 0 to nAttrs-1: (let i be the iterator)
    for(int i=0;i<nAttrs;i++)
    {
        // get the attr-cat entry for the i'th attribute from the attr-cache
        // (use AttrCacheTable::getAttrCatEntry())
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,i,&attrCatEntry);
        int type = attrCatEntry.attrType;
       
        if (type == NUMBER)
        {
            // if the char array record[i] can be converted to a number
            if(isNumber(record[i]))
            {
                /* convert the char array to numeral and store it
                   at recordValues[i].nVal using atof() */
                recordValues[i].nVal = atof(record[i]);
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            // copy record[i] to recordValues[i].sVal
            strcpy((char*)&recordValues[i].sVal,record[i]);
        }
    }
    
    int retVal = BlockAccess::insert(relId,recordValues);
    // insert the record by calling BlockAccess::insert() function
    // let retVal denote the return value of insert call

    return retVal;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {

    int srcRelId = OpenRelTable::getRelId(srcRel);    
    if (srcRelId == E_RELNOTOPEN) {
	  return E_RELNOTOPEN;
    }
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuffer);
    int srcNumAttrs=relCatBuffer.numAttrs;

  
    // if srcRel is not open in open relation table, return E_RELNOTOPEN

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()

    // get the no. of attributes present in relation from the fetched RelCatEntry.

    // attrNames and attrTypes will be used to store the attribute names
    // and types of the source relation respectively
    char srcAttrNames[srcNumAttrs][ATTR_SIZE];
    int srcAttrTypes[srcNumAttrs];

    /*iterate through every attribute of the source relation :
        - get the AttributeCat entry of the attribute with offset.
          (using AttrCacheTable::getAttrCatEntry())
        - fill the arrays `attrNames` and `attrTypes` that we declared earlier
          with the data about each attribute
    */
    for(int i=0;i<srcNumAttrs;i++)
    {
      AttrCatEntry attrCatBuffer;
      AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatBuffer);
      strcpy(srcAttrNames[i],attrCatBuffer.attrName);
      srcAttrTypes[i]=attrCatBuffer.attrType;
    }
    int ret = Schema::createRel(targetRel,srcNumAttrs,srcAttrNames,srcAttrTypes);
    if(ret!=SUCCESS)
  	return ret;
    int targetRelId = OpenRelTable::openRel(targetRel);
    //std::cout<<targetRelId;
    if(targetRelId<0 || targetRelId>=MAX_OPEN)
  	return targetRelId;


    /*** Creating and opening the target relation ***/

    // Create a relation for target relation by calling Schema::createRel()

    // if the createRel returns an error code, then return that value.

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid

    // If opening fails, delete the target relation by calling Schema::deleteRel() of
    // return the error value returned from openRel().


    /*** Inserting projected records into the target relation ***/

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    Attribute record[srcNumAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);

    //std::cout<<"In Algebra\n";
    /*RecBuffer check(30);
    HeadInfo headcheck;
    check.getHeader(&headcheck);
    std::cout<<headcheck.rblock;*/
    //int i=0;
    while (BlockAccess::project(srcRelId, record)==SUCCESS)
    {
        // record will contain the next record
	//std::cout<<"In alg WHile\n";
	//std::cout<<i<<std::endl;
         //i++;
        ret = BlockAccess::insert(targetRelId, record);
         
        if (ret!=SUCCESS) {
             Schema::closeRel(targetRel);
             Schema::deleteRel(targetRel);
             return ret;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {

    

    // if srcRel is not open in open relation table, return E_RELNOTOPEN

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int srcRelId = OpenRelTable::getRelId(srcRel);    
    if (srcRelId == E_RELNOTOPEN) {
	  return E_RELNOTOPEN;
    }
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuffer);
    int srcNumAttrs=relCatBuffer.numAttrs;
    int attr_offset[tar_nAttrs];
    // declare attr_offset[tar_nAttrs] an array of type int.
    // where i-th entry will store the offset in a record of srcRel for the
    // i-th attribute in the target relation.
    int attr_types[tar_nAttrs];
    // let attr_types[tar_nAttrs] be an array of type int.
    // where i-th entry will store the type of the i-th attribute in the
    // target relation.


    /*** Checking if attributes of target are present in the source relation
         and storing its offsets and types ***/

    /*iterate through 0 to tar_nAttrs-1 :
        - get the attribute catalog entry of the attribute with name tar_attrs[i].
        - if the attribute is not found return E_ATTRNOTEXIST
        - fill the attr_offset, attr_types arrays of target relation from the
          corresponding attribute catalog entries of source relation
    */
     for(int i=0;i<tar_nAttrs;i++)
     {
     	AttrCatEntry attrCatBuffer;
     	int ret=AttrCacheTable::getAttrCatEntry(srcRelId,tar_Attrs[i],&attrCatBuffer);
     	if(ret!=SUCCESS)
     		return ret;
     	attr_types[i]=attrCatBuffer.attrType;
     	attr_offset[i]=attrCatBuffer.offset;
     	
     }

    /*** Creating and opening the target relation ***/

    // Create a relation for target relation by calling Schema::createRel()

    // if the createRel returns an error code, then return that value.

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid

    // If opening fails, delete the target relation by calling Schema::deleteRel()
    // and return the error value from openRel()
    int ret = Schema::createRel(targetRel,tar_nAttrs,tar_Attrs,attr_types);
    if(ret!=SUCCESS)
    	return ret;
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId<0 || targetRelId>=MAX_OPEN)
    {
    	Schema::deleteRel(targetRel);
    	return targetRelId;
    }


    /*** Inserting projected records into the target relation ***/

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()

    Attribute record[srcNumAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);

    while (BlockAccess::project(srcRelId, record)== SUCCESS) {
        // the variable `record` will contain the next record

        Attribute proj_record[tar_nAttrs];

        //iterate through 0 to tar_attrs-1:
        //    proj_record[attr_iter] = record[attr_offset[attr_iter]]
	for(int i=0;i<tar_nAttrs;i++)
	{
		proj_record[i] = record[attr_offset[i]];
	}
        ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret!=SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS;
}

// will return if a string can be parsed as a floating point number
bool isNumber(char *str) {
  int len;
  float ignore;
  /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}

