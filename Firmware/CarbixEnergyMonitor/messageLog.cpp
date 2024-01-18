#include "CarbixEnergyMonitor.h"

            messageLog::messageLog()
                :bufPos(0)
                ,bufLen(60)
                ,newMsg(true)
                ,restart(true)
                {}

void        messageLog::endMsg(){
                this->println();
                Serial.write(buf, bufPos);
                msgFile = SD.open(CARBIX_MESSAGE_LOG_PATH, FILE_WRITE);
                if(msgFile) {
                    msgFile.write(buf, bufPos);
                    msgFile.close();
                }
                delete[] buf;
                newMsg= true;
                return;
            }

size_t      messageLog::write(const uint8_t byte){
                if(newMsg){
                    newMsg = false;
                    buf = new uint8_t[bufLen];
                    bufPos = 0;
                    if(restart){
                        restart = false;
                        this->printf_P("\r\n** Restart **\r\n\n");
                    }
                    if(RTCrunning){
                        this->printf("%s ", datef(localTime(),"M/DD/YY hh:mm:ss").c_str());
                        if(localTimeDiff == 0){
                            buf[bufPos-1] = 'z';
                            buf[bufPos++] = ' ';
                        }
                    }
                }
                if(bufPos >= bufLen) {
                    Serial.write(buf, bufPos);
                    msgFile = SD.open(CARBIX_MESSAGE_LOG_PATH, FILE_WRITE);
                    if(! msgFile){
                        String msgDir = CARBIX_MESSAGE_LOG_PATH;
                        msgDir.remove(msgDir.indexOf('/',1));
                        SD.mkdir(msgDir.c_str());
                        msgFile = SD.open(CARBIX_MESSAGE_LOG_PATH, FILE_WRITE);
                    }
                    if(msgFile) {
                        msgFile.write(buf, bufPos);
                        msgFile.close();
                    }
                    bufPos = 0;
                }
                buf[bufPos++] = byte;
                return 1;
            }

size_t      messageLog::write(const uint8_t* buf, const size_t len){
                for(int i=0; i<len; i++) write(buf[i]);
                return len;
            }


  
  
