/**
 * helper.c
 *
 * Functions that are required by FreeRTOS when 
 * static allocation is enabled.
 *
 * Copyright (C) 2024  Oliver Kayser-Herold
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/****************************************************************************
 * Name: vApplicationGetIdleTaskMemory
 *
 * Description:
 *   Needs to be defined when static memory allocation is enabled in
 *   the configuration.
 *
 * Input Parameters:
 *  ppxIdleTaskTCBBuffer   - A handle to a statically allocated TCB buffer
 *  ppxIdleTaskStackBuffer - A handle to a statically allocated Stack buffer 
 *                           for the idle task
 *  puxIdleTaskStackSize   - A pointer to the number of elements that will
 *                           fit in the allocated stack buffer
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
       state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
       configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*-----------------------------------------------------------*/

/****************************************************************************
 * Name: vApplicationGetTimerTaskMemory
 *
 * Description:
 *   Provides implementation of vApplicationGetTimerTaskMemory
 *
 * Input Parameters:
 *  ppxTimerTaskTCBBuffer   - A handle to a statically allocated TCB buffer
 *  ppxTimerTaskStackBuffer - A handle to a statically allocated Stack buffer
 *                            for the idle task
 *  puxTimerTaskStackSize   - A pointer to the number of elements that will 
 *                            fit in the allocated stack buffer
 *   parm   - As provided to the initiatlization of the callback. Should
 *            be the respective FreeRTOS Queue
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
       task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
      configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

