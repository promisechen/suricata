/* Copyright (C) 2007-2010 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Victor Julien <victor@inliniac.net>
 */

#ifndef __TM_QUEUEHANDLERS_H__
#define __TM_QUEUEHANDLERS_H__

enum {
    TMQH_SIMPLE,
    TMQH_NFQ,
    TMQH_PACKETPOOL,
    TMQH_FLOW,

    TMQH_SIZE,
};

typedef struct Tmqh_ {
    const char *name;                          
    Packet *(*InHandler)(ThreadVars *);           /*< 是针对ThreadVars的输入，而非入队的意思，即从相应队列取出Packet。by clx 20171107 */
    void (*InShutdownHandler)(ThreadVars *);      /*< 貌似是把队列中的报文都拿出来的意思??。by clx 20171107 */
    void (*OutHandler)(ThreadVars *, Packet *);   /*< 将报文压入队列中。by clx 20171107*/
    void *(*OutHandlerCtxSetup)(const char *);    /*< 初始化线程上下文。by clx 20171107*/
    void (*OutHandlerCtxFree)(void *);            /*< 释放线程上下文。by clx 20171107*/
    void (*RegisterTests)(void);
} Tmqh;

Tmqh tmqh_table[TMQH_SIZE];

void TmqhSetup (void);
void TmqhCleanup(void);
Tmqh* TmqhGetQueueHandlerByName(const char *name);

#endif /* __TM_QUEUEHANDLERS_H__ */

