#include "stdafx.h"
#include "Global.h"
static void _dummy(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data ){
}
void GlobalInitialize(){
    srand((unsigned)time(NULL));
    g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG,_dummy, NULL);
}
