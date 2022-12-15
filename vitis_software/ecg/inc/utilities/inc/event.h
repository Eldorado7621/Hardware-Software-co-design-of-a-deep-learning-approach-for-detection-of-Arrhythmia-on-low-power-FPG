/*
 * event.h
 *
 *  Created on: Feb 24th, 2020
 *      Author: Yarib Nevarez
 */
#ifndef LIBS_UTILITIES_EVENT_H_
#define LIBS_UTILITIES_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/

#include "timer.h"
#include "xil_types.h"

/***************** Macros (Inline Functions) Definitions *********************/
typedef struct _Event Event;
/**************************** Type Definitions *******************************/

typedef enum
{
 EVENT_MODEL,
 EVENT_OPERATION,
 EVENT_DELEGATE,
 EVENT_HARDWARE
} EventType;

struct  _Event
{
  Event * parent;
  Event * next;
  Event * prev;
  Event * first_child;
  char  * data;
  EventType type;
  Timer * timer;
  double  absolute_offset;
  double  relative_offset;
  double  latency;
};

/************************** Constant Definitions *****************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

Event * Event_new       (Event * parent, EventType type, void * data);
void    Event_delete    (Event ** event);
void    Event_setParent (Event * event, Event * parent);
void    Event_start     (Event * event);
void    Event_stop      (Event * event);
double  Event_getCurrentRelativeTime  (Event * event);
double  Event_getCurrentAbsoluteTime  (Event * event);

/************************* Print event tree hierachy *************************/
void    Event_print (Event * event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBS_UTILITIES_EVENT_H_ */
