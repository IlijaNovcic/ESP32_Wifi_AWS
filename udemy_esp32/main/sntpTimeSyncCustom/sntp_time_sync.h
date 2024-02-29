/*
 * sntp_type_sync.h
 *
 *  Created on: 12.07.2023.
 *      Author: Novcic
 */

#ifndef MAIN_SNTP_TIME_SYNC_H_
#define MAIN_SNTP_TIME_SYNC_H_

/*
 * Starts the sntp server sync task.
 */
void sntp_time_sync_task_start(void);

/*
 * Returns local time if set.
 * @return local time buffer.
 */
char* sntp_time_sync_get_time(void);

#endif /* MAIN_SNTP_TIME_SYNC_H_ */
