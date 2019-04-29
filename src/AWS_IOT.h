/***************************************************************************************************
                                    ExploreEmbedded Copyright Notice    
****************************************************************************************************
 * File:   AWS_IOT.h
 * Version: 1.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: ESP32  Arduino library for AWS IOT.
 
This code has been developed and tested on ExploreEmbedded boards.  
We strongly believe that the library works on any of development boards for respective controllers. 
Check this link http://www.exploreembedded.com/wiki for awesome tutorials on 8051,PIC,AVR,ARM,Robotics,RTOS,IOT.
ExploreEmbedded invests substantial time and effort developing open source HW and SW tools, to support consider buying the ExploreEmbedded boards.
 
The ExploreEmbedded libraries and examples are licensed under the terms of the new-bsd license(two-clause bsd license).
See also: http://www.opensource.org/licenses/bsd-license.php

EXPLOREEMBEDDED DISCLAIMS ANY KIND OF HARDWARE FAILURE RESULTING OUT OF USAGE OF LIBRARIES, DIRECTLY OR
INDIRECTLY. FILES MAY BE SUBJECT TO CHANGE WITHOUT PRIOR NOTICE. THE REVISION HISTORY CONTAINS THE INFORMATION 
RELATED TO UPDATES.
 

Permission to use, copy, modify, and distribute this software and its documentation for any purpose
and without fee is hereby granted, provided that this copyright notices appear in all copies 
and that both those copyright notices and this permission notice appear in supporting documentation.
**************************************************************************************************/
 
 
#ifndef _HORNBILL_AWS_IOT_LIB_H_
#define _HORNBILL_AWS_IOT_LIB_H_

#include "aws_iot_shadow_interface.h"

typedef void (*pSubCallBackHandler_t)(char *topicName, int payloadLen, char *payLoad);


static char deleteAcceptedTopic[MAX_SHADOW_TOPIC_LENGTH_BYTES];

void aws_iot_shadow_task(void *param);

class AWS_IOT{
    
 private:

 public:
    
  int connect(const char *hostAddress, const char *clientID, 
	      const char *aws_root_ca_pem, 
	      const char *certificate_pem_crt, 
	      const char *private_pem_key);
  int attach_shadow(const ShadowConnectParameters_t*, jsonStruct_t *pStruct);
  int publish(const char *pubtopic, const char *pubPayLoad);
  int subscribe(const char *subTopic, pSubCallBackHandler_t pSubCallBackHandler);
  int shadow_yield(int t);
  int shadow_update(char *JsonDocumentBuffer);
  int shadow_disconnect(void);
};


#endif

