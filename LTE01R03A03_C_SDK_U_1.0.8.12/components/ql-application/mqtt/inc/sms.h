/**  @file
  sms_demo.h

  @brief
  This file is used to define sms demo for different Quectel Project.

*/

/*================================================================
  Copyright (c) 2020 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------
10/10/2020  marvin          create

=================================================================*/


#ifndef SMS_DEMO_H
#define SMS_DEMO_H


#ifdef __cplusplus
extern "C" {
#endif


/*========================================================================
 *  Variable Definition
 *========================================================================*/
extern char SMSAlarmType;
extern char send_buffer[64];

/*========================================================================
 *  function Definition
 *========================================================================*/
void ql_sms_app_init(void);



#ifdef __cplusplus
} /*"C" */
#endif

#endif /* SMS_DEMO_H */


