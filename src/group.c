/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * Group.c
 *
 * Link droids together into a group for AI etc.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "objects.h"
#include "group.h"
#include "orderdef.h"

#include "multiplay.h"

// sizes for the group heap
#define GRP_HEAP_INIT	45
#define GRP_HEAP_EXT	15

// initialise the group system
BOOL grpInitialise(void)
{
	return TRUE;
}


// shutdown the group system
void grpShutDown(void)
{
}


// create a new group
BOOL grpCreate(DROID_GROUP	**ppsGroup)
{
	*ppsGroup = malloc(sizeof(DROID_GROUP));
	if (*ppsGroup == NULL)
	{
		debug(LOG_ERROR, "grpCreate: Out of memory");
		return FALSE;
	}

	(*ppsGroup)->type = GT_NORMAL;
	(*ppsGroup)->refCount = 0;
	(*ppsGroup)->psList = NULL;
	(*ppsGroup)->psCommander = NULL;
	memset(&(*ppsGroup)->sRunData, 0, sizeof(RUN_DATA));

	return TRUE;
}


// add a droid to a group
void grpJoin(DROID_GROUP *psGroup, DROID *psDroid)
{
	psGroup->refCount += 1;

	ASSERT( psGroup != NULL,
		"grpJoin: invalid group pointer" );

	// if psDroid == NULL just increase the refcount don't add anything to the list
	if (psDroid != NULL)
	{
		if (psGroup->psList && psDroid->player != psGroup->psList->player)
		{
			ASSERT( FALSE,"grpJoin: Cannot have more than one players droids in a group" );
			return;
		}

		if (psDroid->psGroup != NULL)
		{
			grpLeave(psDroid->psGroup, psDroid);
		}

		psDroid->psGroup = psGroup;

		if (psDroid->droidType == DROID_TRANSPORTER)
		{
			ASSERT( (psGroup->type == GT_NORMAL),
				"grpJoin: Cannot have two transporters in a group" );
			psGroup->type = GT_TRANSPORTER;
			psDroid->psGrpNext = psGroup->psList;
			psGroup->psList = psDroid;
		}
		else if ((psDroid->droidType == DROID_COMMAND) &&
				 (psGroup->type != GT_TRANSPORTER))
		{
			ASSERT( (psGroup->type == GT_NORMAL) && (psGroup->psCommander == NULL),
				"grpJoin: Cannot have two command droids in a group" );
			psGroup->type = GT_COMMAND;
			psGroup->psCommander = psDroid;
		}
		else
		{
			psDroid->psGrpNext = psGroup->psList;
			psGroup->psList = psDroid;
		}
	}
}


// add a droid to a group at the end of the list
void grpJoinEnd(DROID_GROUP *psGroup, DROID *psDroid)
{
	DROID		*psPrev, *psCurr;

	psGroup->refCount += 1;

	ASSERT( psGroup != NULL,
		"grpJoin: invalid group pointer" );

	// if psDroid == NULL just increase the refcount don't add anything to the list
	if (psDroid != NULL)
	{
		if (psGroup->psList && psDroid->player != psGroup->psList->player)
		{
			ASSERT( FALSE,"grpJoin: Cannot have more than one players droids in a group" );
			return;
		}

		if (psDroid->psGroup != NULL)
		{
			grpLeave(psDroid->psGroup, psDroid);
		}

		psDroid->psGroup = psGroup;

		if (psDroid->droidType == DROID_COMMAND)
		{
			ASSERT( (psGroup->type == GT_NORMAL) && (psGroup->psCommander == NULL),
				"grpJoin: Cannot have two command droids in a group" );
			psGroup->type = GT_COMMAND;
			psGroup->psCommander = psDroid;
		}
		else
		{
			// add the droid to the end of the list
			psPrev = NULL;
			psDroid->psGrpNext = NULL;
			for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
			{
				psPrev = psCurr;
			}
			if (psPrev != NULL)
			{
				psPrev->psGrpNext = psDroid;
			}
			else
			{
				psGroup->psList = psDroid;
			}
		}
	}
}


// remove a droid from a group
void grpLeave(DROID_GROUP *psGroup, DROID *psDroid)
{
	DROID	*psPrev, *psCurr;

	ASSERT( psGroup != NULL,
		"grpLeave: invalid group pointer" );

	if (   (psDroid != NULL )
		&& (psDroid->psGroup != psGroup) )
	{
		ASSERT( FALSE, "grpLeave: droid group does not match" );
		return;
	}


	psGroup->refCount -= 1;

	// if psDroid == NULL just decrease the refcount don't remove anything from the list
	if (psDroid != NULL &&
		(psDroid->droidType != DROID_COMMAND ||
		 psGroup->type != GT_COMMAND))
	{
		psPrev = NULL;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			if (psCurr == psDroid)
			{
				break;
			}
			psPrev = psCurr;
		}
		ASSERT( psCurr != NULL, "grpLeave: droid not found" );
		if (psCurr != NULL)
		{
			if (psPrev)
			{
				psPrev->psGrpNext = psCurr->psGrpNext;
			}
			else
			{
				psGroup->psList = psGroup->psList->psGrpNext;
			}
		}
	}

	if (psDroid)
	{
		psDroid->psGroup = NULL;
		psDroid->psGrpNext = NULL;

		if ((psDroid->droidType == DROID_COMMAND) &&
			(psGroup->type == GT_COMMAND))
		{
			psGroup->type = GT_NORMAL;
			psGroup->psCommander = NULL;
		}
		else if ((psDroid->droidType == DROID_TRANSPORTER) &&
				 (psGroup->type == GT_TRANSPORTER))
		{
			psGroup->type = GT_NORMAL;
		}
	}

	// free the group structure if necessary
	if (psGroup->refCount <= 0)
	{
		free(psGroup);
	}
}

// count the members of a group
SDWORD grpNumMembers(DROID_GROUP *psGroup)
{
	DROID	*psCurr;
	SDWORD	num;

	ASSERT( psGroup != NULL,
		"grpNumMembers: invalid droid group" );

	num = 0;
	for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		num += 1;
	}

	return num;
}


// remove all droids from a group
void grpReset(DROID_GROUP *psGroup)
{
	DROID	*psCurr, *psNext;

	ASSERT( psGroup != NULL,
		"grpReset: invalid droid group" );

	for(psCurr = psGroup->psList; psCurr; psCurr = psNext)
	{
		psNext = psCurr->psGrpNext;
		grpLeave(psGroup, psCurr);
	}
}

/* Give a group an order */
//void orderGroupBase(DROID_GROUP *psGroup, DROID_ORDER_DATA *psData)
//{
//	DROID *psCurr;
//	BOOL usedgrouporder=FALSE;

//	ASSERT( psGroup != NULL,
//		"orderGroupBase: invalid droid group" );
//
//	if (bMultiPlayer && SendGroupOrder(	psGroup, psData->x,	psData->y,	psData->psObj) )
//	{	// turn off multiplay messages,since we've send a group one instead.
//		bMultiPlayer =FALSE;
//		usedgrouporder = TRUE;
//	}
//
//	for (psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
//	{
//		orderDroidBase(psCurr, psData);
//	}

//	if( usedgrouporder)
//	{
//		bMultiPlayer = TRUE;
//	}
//}

/* Give a group an order */
void orderGroup(DROID_GROUP *psGroup, DROID_ORDER order)
{
	DROID *psCurr;

	ASSERT( psGroup != NULL,
		"orderGroup: invalid droid group" );

	for (psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		orderDroid(psCurr, order);
	}
}

/* Give a group of droids an order */
void orderGroupLoc(DROID_GROUP *psGroup, DROID_ORDER order, UDWORD x, UDWORD y)
{
	DROID	*psCurr;

	ASSERT( psGroup != NULL,
		"orderGroupLoc: invalid droid group" );

	if(bMultiPlayer)
	{
		SendGroupOrderGroup(psGroup,order,x,y,NULL);
		bMultiPlayer = FALSE;

		for(psCurr=psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidLoc(psCurr, order, x,y);
		}

		bMultiPlayer = TRUE;
	}
	else
	{
		for(psCurr=psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidLoc(psCurr, order, x,y);
		}
	}
}


/* Give a group of droids an order */
void orderGroupObj(DROID_GROUP *psGroup, DROID_ORDER order, BASE_OBJECT *psObj)
{
	DROID	*psCurr;
	DROID_OACTION_INFO oaInfo = {{(BASE_OBJECT *)psObj}};

	ASSERT( psGroup != NULL,
		"orderGroupObj: invalid droid group" );

	if(bMultiPlayer)
	{
		SendGroupOrderGroup(psGroup,order,0,0,psObj);
		bMultiPlayer = FALSE;

		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidObj(psCurr, order, &oaInfo);
		}

		bMultiPlayer = TRUE;
	}
	else
	{
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidObj(psCurr, order, &oaInfo);
		}
	}
}

/* set the secondary state for a group of droids */
void grpSetSecondary(DROID_GROUP *psGroup, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psCurr;

	ASSERT( psGroup != NULL,
		"grpSetSecondary: invalid droid group" );

	for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		secondarySetState(psCurr, sec, state);
	}
}
