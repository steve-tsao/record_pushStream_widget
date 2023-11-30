#include "log.h"
#include <fstream>
#include <chrono>
#include <QMutex>

extern QMutex logMutex;

Log::Log()
{

}

 void Log::writeLog(std::string content)
 {
     auto now = std::chrono::system_clock::now();
     uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
             - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
     time_t tt = std::chrono::system_clock::to_time_t(now);
     auto time_tm = localtime(&tt);
     char strTime[25] = { 0 };
     sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d %03d", time_tm->tm_year + 1900,
             time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
             time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds);

     logMutex.lock();
     std::ofstream ofs;
     ofs.open("program_record.log",std::ios::app );
     ofs<<strTime<<": "<<content<<std::endl;
     ofs.close();
     logMutex.unlock();
 }
