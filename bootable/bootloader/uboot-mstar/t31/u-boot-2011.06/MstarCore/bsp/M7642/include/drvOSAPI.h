/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */
/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2019 MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#ifndef _OS_API_H_
#define _OS_API_H_

#include <MsCommon.h>
//#include <cyg/hal/hal_if.h> // NUSED
// Note, include for kernel service functions of eCos
//#include <cyg/kernel/kapi.h> // NUSED
//#include <cyg/hal/hal_intr.h> // NUSED
//#include <cyg/hal/hal_misc.h> // for BITn
//#include "drvPorts.h"

#define IRQ_NONE              0
#define IRQ_HANDLED           1
#define TASK_UNINTERRUPTIBLE  0
/* ========================================  Data structures  ======================================== */

typedef int spinlock_t;
typedef int atomic_t;

#define SPIN_LOCK_UNLOCKED  (spinlock_t)0
#define SPIN_LOCK_LOCKED    (spinlock_t)1

/* ========================================  Function Declaration  ======================================== */
#define DECLARE_WAITQUEUE(x,y)
#define init_waitqueue_head(x)
#define add_wait_queue(x,y)
#define remove_wait_queue(x,y)
#define wake_up(x)
#define signal_pending(x)     0

// Spin lock functions
#define osapi_spin_lock_init(lock)
#define osapi_spin_lock(lock)
#define osapi_spin_unlock(lock)
#define osapi_spin_lock_irqsave(lock,flag)      {flag = MsOS_DisableAllInterrupts();}
#define osapi_spin_unlock_irqrestore(lock,flag) MsOS_RestoreAllInterrupts(flag)
#define osapi_spin_lock_irq(lock) MsOS_DisableAllInterrupts()
#define osapi_spin_unlock_irq(lock) MsOS_EnableAllInterrupts()

// Semaphore functions
#define osapi_up(x)
#define osapi_down(x)

// Mutex functions
#define osapi_mutex_init(x, str) \
    x = MsOS_CreateMutex(E_MSOS_FIFO, str, MSOS_PROCESS_SHARED)
#define osapi_mutex_lock(x)\
    MsOS_ObtainMutex(x, MSOS_WAIT_FOREVER)
#define osapi_mutex_unlock(x) \
    MsOS_ReleaseMutex(x)
#define osapi_mutex_delete(x) \
    MsOS_DeleteMutex(x)
//#define osapi_init_MUTEX(x) // NUSED
//#define osapi_init_MUTEX_LOCKED(x) // NUSED
#define osapi_atomic_read(v)        *(v)
#define osapi_ATOMIC_INIT(value)    value

// Kernel function porting (Unsupport now, so just skip)
#define set_current_state(x)
//#define osapi_local_irq_restore(x) // NUSED
//#define osapi_local_irq_save(x) // NUSED
//#define osapi_local_irq_disable() // NUSED
//#define osapi_local_irq_enable() // NUSED
//#define osapi_lock_kernel() // NUSED
//#define osapi_unlock_kernel() // NUSED
//#define osapi_schedule() // NUSED

static __inline__ void osapi_atomic_inc(int *cnt_p)
{
  (*cnt_p)++;
}

static __inline__ void osapi_atomic_dec(int *cnt_p)
{
  (*cnt_p)--;
}

static __inline__ int osapi_atomic_dec_and_test(int *count)
{
  (*count)--;
  return (*count == 0);
}

static __inline__ void osapi_atomic_set(int *count, int value)
{
  (*count) = value;
}

static __inline__ void osapi_schedule_timeout(int ms)
{
	mdelay(ms);
}
#endif

