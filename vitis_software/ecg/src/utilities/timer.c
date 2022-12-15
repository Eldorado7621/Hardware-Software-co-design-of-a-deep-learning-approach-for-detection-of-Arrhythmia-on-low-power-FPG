/*
 * timer.c
 *
 *  Created on: Feb 24th, 2020
 *      Author: Yarib Nevarez
 */
/***************************** Include Files *********************************/
#include "timer.h"
#include "miscellaneous.h"

#include "stdlib.h"
#include "string.h"

/***************** Macros (Inline Functions) Definitions *********************/

/**************************** Type Definitions *******************************/

/************************** Constant Definitions *****************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

/*****************************************************************************/

Timer * Timer_new (uint8_t num_samples)
{
  Timer * timer = NULL;
  ASSERT(0 < num_samples);
  if (0 < num_samples)
  {
    size_t size = sizeof(Timer) + ((num_samples - 1) * sizeof(XTime));
    timer = (Timer *) malloc (size);
    ASSERT(timer != NULL);
    if (timer != NULL)
    {
      memset (timer, 0x00, size);
      timer->num_samples = num_samples;
    }
  }

  return timer;
}

void Timer_delete (Timer ** timer)
{
  ASSERT (timer != NULL);
  ASSERT (*timer != NULL);

  if ((timer != NULL) && (*timer != NULL))
  {
    free (*timer);
    *timer = NULL;
  }
}

void Timer_start (Timer * timer)
{
  ASSERT(timer != NULL);
  if (timer != NULL)
    XTime_GetTime (&timer->start_time);
}

double Timer_getCurrentTime (Timer * timer)
{
  double time = 0.0;
  ASSERT(timer != NULL);
  if (timer != NULL)
  {
    XTime temp;
    XTime_GetTime (&temp);
    time = ((double) (temp - timer->start_time)) / ((double) COUNTS_PER_SECOND);
  }
  return time;
}

void Timer_takeSample (Timer * timer, uint8_t index, double * sample)
{
  ASSERT(timer != NULL);
  ASSERT(index < timer->num_samples);
  if ((timer != NULL) && (index < timer->num_samples))
  {
    XTime time;
    XTime_GetTime (&time);
    timer->sample_array[index] = time;
    if (sample != NULL)
      *sample = ((double) (timer->sample_array[index] - timer->start_time))
          / ((double) COUNTS_PER_SECOND);
  }
}

double Timer_getSample(Timer * timer, uint8_t index)
{
  double sample = 0.0;
  ASSERT(timer != NULL);
  ASSERT(index < timer->num_samples);
  if ((timer != NULL) && (index < timer->num_samples))
    sample = ((double) (timer->sample_array[index] - timer->start_time))
                      / ((double) COUNTS_PER_SECOND);
  return sample;
}
