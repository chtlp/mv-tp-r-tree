#ifndef __PARA_H
#define __PARA_H
//////////////////////////////////////////////////////////////////////////////
//File Discription: This file contains the parameters needed to run this program
//Uses need to set up these parameters to suit different situations
//////////////////////////////////////////////////////////////////////////////
#define FUTURE            20        // This is how far we should look into future when computing
                                    //overlap and area
#define STRONGVERSIONPARA 0.85      //This states the percentage of the node when it gets strong
					                //version overflow
#define STRONGVERUNDER    0.4       //This parameter states the percentage of the node when it gets
                                    //strong version underflow
#define WVERSIONUF        0.3       //This is the weak version underflow parameter
                                    //In the deletion, when the number of alive records in a node drops
                                    //lower than this capacity ratio, reinsert will be performed
#define PHYUF             0.3       //This is the strong version underflow

#define DETRATIO          0.01      //this parameter is useful only when we use multireinsert to insert
                                    //entries.  It specfies the farthest other candidate branches can deteriorate
                                    //from the optimal one
#define GOODRATIO         0.96      //this value is used in reinserting the past data.  if the penalty incurred
                                    //is greater than this ratio of the original value we do not make the change.
#define MULTI             0.9       //This defines to what degree the multi-reinsert in a split will stop
                                    //This is valid only when MULTIREINSERT=true or KEYSPLITREINSERT=true;
//////////////////////////////////////////////////////////////////////////////
//Program constants
//////////////////////////////////////////////////////////////////////////////
#define FINALVER          100       //this parameter states the final timestamp

#define MAXBLOCK          100000    //This constant specifies the maximum number of blocks used
                                    //by the index
#define NOWTIME           30000     //We treat "nowtime" as a reserved word in the database

#define OBJECTNUM         51000     //this is the total number of objects being indexed

#define SCALETIME         100       //since the data set represent time within the range [0,1], we
							        //need to scale up the time to integers
#define E3RSCALE          320         //This is the scaling we will use when building the 3D R-tree under-
                                    //neath the MVR-tree
//////////////////////////////////////////////////////////////////////////////
//These are the switches
//////////////////////////////////////////////////////////////////////////////
#define AVOIDLEAFSPLIT   true

#define DOSTRONGOVER     true 

#define DOSTRONGUNDER    false

#define USEPOOLING       false

#define TRYMORESUBTREE   false        //if we will try multiple subtrees in choose_subtree

#define INSERTAFTERDEL   false        //if we will try inserting a just-deleted entry

#define COMPUTE3D        false

#define DELAREASORT      true	      //if sorting the deletion candidates by area decreament

#define DELDISTSORT      false        //if sorting the deletion candidates by the distance from centers 

#define MULTIREINSERT    false        //if we will try to reinsert multiple times in a node to be split

#define KEYSPLITREINSERT false        //Whether to use reinsert to handle key split
//////////////////////////////////////////////////////////////////////////////

// simple MBR growth algorithm
#define SIMPLE_MBR_GROWTH true

// the accuracy radius
#define ACCURACY 0.05

#define TRIGGER_TIME 1

// time parametric
#define COMPUTETP true

#define PREDICT_TIME 2

#endif