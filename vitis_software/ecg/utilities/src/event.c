/*
 * event.c
 *
 *  Created on: Feb 24th, 2020
 *      Author: Yarib Nevarez
 */
/***************************** Include Files *********************************/
#include "event.h"
#include "miscellaneous.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/***************** Macros (Inline Functions) Definitions *********************/

/**************************** Type Definitions *******************************/

/************************** Constant Definitions *****************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

/*****************************************************************************/

/************************** Constant Definitions *****************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

Event * Event_new (Event * parent, EventType type, void * data)
{
  Event * event = (Event *) malloc (sizeof(Event));
  ASSERT (event != NULL);

  if (event != NULL)
  {
    memset (event, 0, sizeof(Event));

    event->data = (char *) data;

    event->type = type;

    event->timer = Timer_new (1);

    ASSERT (event->timer != NULL);

    if (parent != NULL)
    {
      if (parent->first_child != NULL)
      {
        Event * child = parent->first_child;

        while (child->next != NULL)
        {
          child = child->next;
        }

        child->next = event;

        event->prev = child;
      }
      else
      {
        parent->first_child = event;
      }
      event->parent = parent;
    }
  }

  return event;
}

void Event_setParent (Event * event, Event * parent)
{
  ASSERT (event != NULL);
  ASSERT (event->parent == NULL)

  if ((event != NULL) && (event->parent == NULL))
  {
    event->parent = parent;

    if (parent->first_child != NULL)
    {
      Event * child = parent->first_child;

      while (child->next != NULL)
      {
        child = child->next;
      }

      child->next = event;

      event->prev = child;
    }
    else
    {
      parent->first_child = event;
    }
  }
}

void Event_delete (Event ** event)
{
  ASSERT (event != NULL);
  ASSERT (*event != NULL);

  if ((event != NULL) && (*event != NULL))
  {
    if (((*event)->parent != NULL) && ((*event)->parent->first_child == *event))
    {
      (*event)->parent->first_child = (*event)->next;

      if ((*event)->parent->first_child != NULL)
      {
        (*event)->parent->first_child->prev = NULL;
      }
    }

    if ((*event)->prev != NULL)
    {
      (*event)->prev->next = (*event)->next;
    }

    for (Event *child = (*event)->first_child; child != NULL; child = child->next)
    {
      child->parent = NULL;
    }

    Timer_delete (&(*event)->timer);

    free (*event);
    *event = NULL;
  }
}

void Event_start (Event * event)
{
  ASSERT (event != NULL);

  if (event != NULL)
  {
    if (event->parent != NULL)
    {
      event->relative_offset = Event_getCurrentRelativeTime (event->parent);
      event->absolute_offset = event->parent->absolute_offset + event->relative_offset;
    }
    else
    {
      event->relative_offset = 0;
      event->absolute_offset = 0;
    }

    Timer_start (event->timer);

    event->latency = 0;
  }
}

double Event_getCurrentRelativeTime (Event * event)
{
  double current_time = 0;

  ASSERT (event != NULL);

  if (event != NULL)
  {
    current_time = Timer_getCurrentTime (event->timer);
  }

  return current_time;
}

double  Event_getCurrentAbsoluteTime (Event * event)
{
  double absolute_time = 0;

  ASSERT (event != NULL);

  if (event != NULL)
  {
    absolute_time = Event_getCurrentRelativeTime (event) + event->absolute_offset;
  }

  return absolute_time;
}

void Event_stop (Event * event)
{
  ASSERT (event != NULL);

  if (event != NULL)
  {
    event->latency = Event_getCurrentRelativeTime (event);
  }
}

/*****************************************************************************/

typedef enum
{
  NAV_CONTINUE,
  NAV_ABORT,
} NavigationReturn;

typedef NavigationReturn (*EventFunctionP) (Event *, void *);

static NavigationReturn Event_navegate (Event * event,
                                        EventFunctionP function,
                                        void * data)
{
  NavigationReturn result = NAV_ABORT;
  ASSERT (event != NULL);
  ASSERT (function != NULL);

  if ((event != NULL) && (function != NULL))
  {
    result = function (event, data);

    for (Event * child = event->first_child;
        (result != NAV_ABORT) && (child != NULL);
        child = child->next)
    {
      result = Event_navegate (child, function, data);
    }
  }

  return result;
}

/*****************************************************************************/

typedef char TextLines[4][4096];

static NavigationReturn Event_collectScheduleData (Event * event, void * data)
{
  NavigationReturn result = NAV_ABORT;
  ASSERT (event != NULL);
  ASSERT (data != NULL);

  if ((event != NULL) && (data != NULL))
  {
    TextLines * text = (TextLines*) data;
    char * color;

    switch (event->type)
    {
      case EVENT_MODEL:
        color = (char*) "#1864ab";
        break;
      case EVENT_OPERATION:

//        if (event->first_child != NULL)
//          return result = NAV_CONTINUE;

        color = (char*) "#4a98c9";
        break;
      case EVENT_DELEGATE:
        color = (char*) "#6faed4";
        break;
      case EVENT_HARDWARE:
        color = (char*) "#94c4df";
        break;
      default:
        ASSERT(0);
    }

    sprintf (&(*text)[0][strlen ((*text)[0])], "%.3lf, ", event->absolute_offset * 1000);
    sprintf (&(*text)[1][strlen ((*text)[1])], "%.3lf, ", event->latency * 1000);
    sprintf (&(*text)[2][strlen ((*text)[2])], "\"%s\", ", (char*) event->data);
    sprintf (&(*text)[3][strlen ((*text)[3])], "\"%s\", ", color);

    result = NAV_CONTINUE;
  }

  return result;
}

static NavigationReturn Event_collectLatencyData (Event * event, void * data)
{
  NavigationReturn result = NAV_ABORT;
  ASSERT (event != NULL);
  ASSERT (data != NULL);

  if ((event != NULL) && (data != NULL))
  {
    if (event->type == EVENT_OPERATION)
    { /* Then is a hardware event */
      TextLines * text = (TextLines*) data;
      double hw_latency =
          (event->first_child != NULL)
       && (event->first_child->first_child != NULL) ?
           event->first_child->first_child->latency : 0.0;

      sprintf (&(*text)[0][strlen ((*text)[0])], "%.3lf, ", event->absolute_offset * 1000);
      sprintf (&(*text)[1][strlen ((*text)[1])], "%.3lf, ", event->latency * 1000);
      sprintf (&(*text)[2][strlen ((*text)[2])], "%.3lf, ", hw_latency * 1000);
      sprintf (&(*text)[3][strlen ((*text)[3])], "\"%s\", ", (char*) event->data);
    }
    result = NAV_CONTINUE;
  }

  return result;
}

void Event_print (Event * event)
{
  ASSERT (event != NULL);

  if (event != NULL)
  {
    TextLines scheduleData;
    TextLines latencyData;

    memset (scheduleData, 0, sizeof(scheduleData));
    Event_navegate (event, Event_collectScheduleData, scheduleData);

    printf ("\nSchedule\n");
    printf ("Absolute offset: [%s]\n", scheduleData[0]);
    printf ("Latency:         [%s]\n", scheduleData[1]);
    printf ("Name:            [%s]\n", scheduleData[2]);
    printf ("Color:           [%s]\n", scheduleData[3]);

    memset (latencyData, 0, sizeof(latencyData));
    Event_navegate (event, Event_collectLatencyData, latencyData);

    printf ("\nPerformance\n");
    printf ("II offset:   [%s]\n", latencyData[0]);
    printf ("SW Latency:  [%s]\n", latencyData[1]);
    printf ("HW Latency:  [%s]\n", latencyData[2]);
    printf ("Name:        [%s]\n", latencyData[3]);

////////////////////////////////////////////////////////////
    printf ("\n#Python:\n");
    printf ("begin   = np.array([%s])\n", scheduleData[0]);
    printf ("latency = np.array([%s])\n", scheduleData[1]);
    printf ("event   = [%s]\n", scheduleData[2]);
    printf ("colors  = [%s]\n", scheduleData[3]);

    printf ("\n");

    printf ("data = [[%s],\n", latencyData[0]);
    printf ("        [ %s],\n", latencyData[1]);
    printf ("        [ %s]]\n", latencyData[2]);

    printf ("columns = (%s)\n\n\n", latencyData[3]);
  }
}
