
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PRIVATE char buffer[10];
PRIVATE int now;
PRIVATE int readOrWrite;
PRIVATE int waiting;
PRIVATE int readcount;
PRIVATE SEMAPHORE semaphores[5] = {
	{1, 0}, {1, 0},{1,0},{2,0}
	//rmutex,wmutex,s
};
PRIVATE SEMAPHORE *first_semaphore;
/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS *p;
	int greatest_ticks = 0;

	//minus sleep time
	for (p = proc_table; p < proc_table + NR_TASKS; p++)
	{
		if (p->sleep_ticks > 0)
		{
			p->sleep_ticks--;
		}
	}

	while (!greatest_ticks)
	{ 
		for (p = proc_table; p < proc_table + NR_TASKS; p++)
		{
			if (p->wait || p->sleep_ticks)
			{
				continue;
			}
			if (p->ticks > greatest_ticks)
			{ //调整最大时钟
				greatest_ticks = p->ticks;
				p_proc_ready = p;
				
			}
		}

		if (!greatest_ticks)
		{
			for (p = proc_table; p < proc_table + NR_TASKS; p++)
			{
				if (p->wait || p->sleep_ticks)
				{
					continue;
				}
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_process_sleep
 *======================================================================*/
PUBLIC int sys_process_sleep(int milli_sec)
{
	p_proc_ready->sleep_ticks = milli_sec / 10;
	schedule();

	return 0;
}

/*======================================================================*
                           sys_disp_str
 *======================================================================*/
PUBLIC int sys_disp_str(char *str)
{
	//choose color for each process
	char color = colors[p_proc_ready - proc_table - 1];

	//output str
	char *temp = str;
	while (*temp != 0)
	{
		out_char(current_con, *temp, color);
		temp++;
	}

	return 0;
}

/*======================================================================*
                           sys_P
 *======================================================================*/
PUBLIC int sys_P(SEMAPHORE *s)
{
	s->value--;
	if (s->value < 0)
	{
		//sleep process
		p_proc_ready->wait = 1;
		if (s->queue == 0)
		{
			s->queue = p_proc_ready;
		}
		else
		{
			PROCESS *rear = s->queue;
			while (rear->next != 0)
			{
				rear = rear->next;
			}
			rear->next = p_proc_ready;
		}
		//
		schedule();
	}
	return 0;
}

/*======================================================================*
                           sys_V
 *======================================================================*/
PUBLIC int sys_V(SEMAPHORE *s)
{
	s->value++;
	if (s->value <= 0)
	{
		//wake up process
		p_proc_ready = s->queue;

		s->queue = s->queue->next;
		p_proc_ready->next = 0;
		p_proc_ready->wait = 0;
	}
	return 0;
}

/*======================================================================*
                           init
 *======================================================================*/
PUBLIC void init()
{
	readOrWrite = 0;
	waiting = 0;
	readcount = 0;
	first_semaphore = semaphores;
}

/*======================================================================*
                           writer
 *======================================================================*/
PUBLIC void writers(char *name,int time)
{
	while (1)
	{
		system_P(first_semaphore+2);
		system_P(first_semaphore+1);
		system_disp_str(name);
		system_disp_str(" is starting to write\n");
		system_disp_str(name);
		system_disp_str(" is writing\n");
		readOrWrite  = 1;
		system_process_sleep(time);
		system_disp_str(name);
		system_disp_str(" finish writing\n");
		system_V(first_semaphore+1);
		system_V(first_semaphore+2);
	}
}

/*======================================================================*
                           readers
 *======================================================================*/
PUBLIC void readers(char *name,int time)
{
	while (1)
	{
		system_P(first_semaphore+3);
		system_P(first_semaphore+2);
		
		system_P(first_semaphore);
		if (readcount == 0)
			system_P(first_semaphore + 1);
		readcount++;
		system_disp_str(name);
		system_disp_str(" is starting to read\n");
		system_V(first_semaphore);
		
		system_V(first_semaphore+2);
		system_disp_str(name);
		system_disp_str(" is reading\n");
		readOrWrite  = 0;
		system_process_sleep(time);
		system_disp_str(name);
		system_disp_str(" finish reading\n");
		system_P(first_semaphore);
		readcount--;
		if (readcount == 0)
			system_V(first_semaphore + 1);
		system_V(first_semaphore);
		system_V(first_semaphore+3);
	}
}

/*======================================================================*
                          	info
 *======================================================================*/
PUBLIC void info(){
	while (1)
	{
		system_disp_str(!readOrWrite?"read:":"write");
		if(!readOrWrite){
			itoa(buffer,readcount);
			system_disp_str(buffer);
		} 
		system_disp_str("\n");
		milli_delay(10000);
	};
	

	
}
