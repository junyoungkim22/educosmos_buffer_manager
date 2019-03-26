/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module: EduOM_NextObject.c
 *
 * Description:
 *  Return the next Object of the given Current Object. 
 *
 * Export:
 *  Four EduOM_NextObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 */


#include "EduOM_common.h"
#include "BfM.h"
#include "EduOM_Internal.h"

/*@================================
 * EduOM_NextObject()
 *================================*/
/*
 * Function: Four EduOM_NextObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  Return the next Object of the given Current Object.  Find the Object in the
 *  same page which has the current Object and  if there  is no next Object in
 *  the same page, find it from the next page. If the Current Object is NULL,
 *  return the first Object of the file.
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by function calls
 *
 * Side effect:
 *  1) parameter nextOID
 *     nextOID is filled with the next object's identifier
 *  2) parameter objHdr
 *     objHdr is filled with the next object's header
 */
Four EduOM_NextObject(
    ObjectID  *catObjForFile,	/* IN informations about a data file */
    ObjectID  *curOID,		/* IN a ObjectID of the current Object */
    ObjectID  *nextOID,		/* OUT the next Object of a current Object */
    ObjectHdr *objHdr)		/* OUT the object header of next object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    PageNo pageNo;		/* a temporary var for next page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    PhysicalFileID pFid;	/* file in which the objects are located */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* data structure for catalog object access */



    /*@
     * parameter checking
     */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (nextOID == NULL) ERR(eBADOBJECTID_OM);
	
	e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);

	if(e < 0) ERR(e);

	GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);

	MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->firstPage);

	if(curOID == NULL)
	{
		e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
		if(e < 0) ERR(e);
		if(apage->header.nSlots == 0)
		{
			if(pid.pageNo == catEntry->lastPage)
			{
				e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
				if(e < 0) ERR(e);
				e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
				if(e < 0) ERR(e);
				return(EOS);
			}
			else
			{
				pageNo = apage->header.nextPage;
				e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
				if(e < 0) ERR(e);
				pid.pageNo = pageNo;
			}
		}
		MAKE_OBJECTID(*nextOID, pid.volNo, pid.pageNo, 0, apage->slot[0].unique);
		objHdr = (ObjectHdr*)&apage->data[apage->slot[0].offset];
		e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
		if(e < 0) ERR(e);
	}
	else
	{
		e = BfM_GetTrain((TrainID*)curOID, (char**)&apage, PAGE_BUF);
		if(e < 0) ERR(e);
		if(curOID->slotNo == (apage->header.nSlots - 1))
		{
			if(curOID->pageNo == catEntry->lastPage)
			{
				e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
				if(e < 0) ERR(e);
				e = BfM_FreeTrain((TrainID*)curOID, PAGE_BUF);
				if(e < 0) ERR(e);
				return(EOS);
			}
			pid.pageNo = apage->header.nextPage;
			e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
			if(e < 0) ERR(e);
			MAKE_OBJECTID(*nextOID, pid.volNo, pid.pageNo, 0, apage->slot[0].unique);
			objHdr = (ObjectHdr*)&apage->data[apage->slot[0].offset];
			e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
			if(e < 0) ERR(e);
		}
		else
		{
			MAKE_OBJECTID(*nextOID, curOID->volNo, curOID->pageNo, curOID->slotNo + 1, apage->slot[-(curOID->slotNo + 1)].unique);
			objHdr = (ObjectHdr*)&apage->data[apage->slot[-(curOID->slotNo + 1)].offset];
		}

		e = BfM_FreeTrain((TrainID*)curOID, PAGE_BUF);
		if(e < 0) ERR(e);
		
	}
	e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
	if(e < 0) ERR(e);

    return(EOS);		/* end of scan */
    
} /* EduOM_NextObject() */
