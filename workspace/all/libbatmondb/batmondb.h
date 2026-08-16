// heavily modified from the Onion original: https://github.com/OnionUI/Onion/tree/main/src/batmon
#ifndef __batmon_db_h__
#define __batmon_db_h__

#ifdef __cplusplus
extern "C" {
#endif

sqlite3* open_battery_log_db(void);
void close_battery_log_db(sqlite3* ctx);
int get_best_session_time(sqlite3* ctx, const char* device);

#ifdef __cplusplus
}
#endif

#endif // __batmon_db_h__