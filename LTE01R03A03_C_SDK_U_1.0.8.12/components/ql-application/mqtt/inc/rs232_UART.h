/*=================================================================

						EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN			  WHO		  WHAT, WHERE, WHY
------------	 -------	 -------------------------------------------------------------------------------

=================================================================*/


#ifndef UART_DEMO_H
#define UART_DEMO_H


#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Enumeration Definition
 ===========================================================================*/


/*===========================================================================
 * Struct
 ===========================================================================*/
extern uint16_t Rec_char;
extern uint8_t cim_recv_buff[2048];
extern uint8_t SerialTXBuffer[2048];
extern uint8_t DLMS_init_flag;
extern uint8_t ProductionMode;

/*===========================================================================
 * Functions declaration
 ===========================================================================*/
void ql_uart_rs232_init(void);
void parshingDataForProductionMode(void);
void CalculateBulbFailure(void);
//void ql_uart_rb_app_init(void);

#ifdef __cplusplus
} /*"C" */
#endif

#endif /* UART_DEMO_H */


