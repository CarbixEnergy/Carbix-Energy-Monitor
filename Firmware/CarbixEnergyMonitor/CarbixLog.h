#ifndef CarbixLog_h
#define CarbixLog_h
#include "SPI.h"
#include "SD.h"

#define CARBIXLOG_BLOCK_SIZE 512
#define CARBIXLOG_PREFORMAT_RECORDS 24

/*******************************************************************************************************
********************************************************************************************************
Class CarbixLog
Maintains a log of fixed length, consecutive entries.
All entries must be written with increasing keys.
Entries are read by key value.
When reading by key, the entry with the requested or next lower key is returned with the requested key.
********************************************************************************************************
********************************************************************************************************/
struct CarbixLogRecord {
      uint32_t UNIXtime;        // Time period represented by this record
      int32_t serial;           // record number in file
      double logHours;          // Total hours of monitoring logged to date in this log
      union {
        struct {                // import/export log record (total size 32 bytes)
          double Import;
          double Export;
        };
        struct {                // Full datalog record (total size 256 bytes)
          double accum1[15];
          double accum2[15];
        };
      };
      CarbixLogRecord()
      :UNIXtime(0)
      ,serial(0)
      ,logHours(0){};
    };    

class CarbixLog
{
  public:

	CarbixLog(size_t recordSize = 256, int interval=5, int days = 365, int preformat=CARBIXLOG_PREFORMAT_RECORDS)
      :_path(0)
      ,_interval(interval)
      ,_recordSize(recordSize)
      ,_fileSize(0)
      ,_physicalSize(0)
      ,_lastReadKey(0)
      ,_lastReadSerial(0)
      ,_readKeyIO(0)
      ,_wrap(0)
      ,_preformat(preformat)
      ,_firstKey(0)
      ,_firstSerial(0)
      ,_lastKey(0)
      ,_lastSerial(-1)
      ,_entries(0)
      ,_cacheSize(10)
      ,_cacheWrap(0)
      ,_writeCacheBuf(0)
      ,_writeCachePos(-CARBIXLOG_BLOCK_SIZE)
      ,_writeCache(false)
    {
    _cacheKey = new uint32_t[_cacheSize];
    _cacheSerial = new int32_t[_cacheSize];
    setDays(days);     
	  }
	
	~CarbixLog(){
    CarbixFile.close();
    delete[] _path;
    delete[] _cacheKey;
    delete[] _cacheSerial;
    delete[] _writeCacheBuf;
  }
	      
    int begin (const char* /* filepath */);
    int write (CarbixLogRecord* /* pointer to record to be written*/);
    int readKey (CarbixLogRecord* /* pointer to caller's buffer */);
    int readSerial(CarbixLogRecord* callerRecord, int32_t serial);
    int readNext(CarbixLogRecord* /* pointer to caller's buffer */);
    void writeCache(bool on);
    int end();
    
    boolean  isOpen();
    uint32_t firstKey();
    int32_t  firstSerial();
    uint32_t lastKey();
    int32_t  lastSerial();
    uint32_t fileSize();
    uint32_t readKeyIO();
    uint32_t interval();
    uint32_t setDays(uint32_t); 
	 	      
    void     dumpFile();

  protected:
        
	  File 	 CarbixFile;

    char*    _path;                         // file pathname
    uint16_t _interval;	                    // Posting interval to log. Currently tested only using 5.
    uint16_t _recordSize;      	  		      // Size of a log record
    uint32_t _fileSize;                     // Logical file size in bytes
    uint32_t _physicalSize;                 // Physical file size in bytes
    uint32_t _entries;                      // Number of entries (fileSize / recsize(256))
    uint32_t _maxFileSize;				        	// Maximum filesize in bytes
    uint32_t _firstKey;                     // Key of first logical record
    int32_t  _firstSerial;                  // Serial of...
    uint32_t _lastKey;                      // Key of last logical record
    int32_t  _lastSerial;                   // Serial of...
    uint32_t _wrap;                         // Offset of logical record zero (0 if file not wrapped)
    uint32_t _preformat;                    // Number of records to preformat (or 0)

    uint32_t  _cacheSize = 10;
    uint32_t  _cacheWrap = 0;  
    uint32_t* _cacheKey;
    int32_t*  _cacheSerial;
  
    uint32_t _lastReadKey;           	      // Key of last record read with readKey
    int32_t  _lastReadSerial;         	    // Serial of last...
    uint32_t _readKeyIO;              	    // Running count of I/Os for keyed reads

    uint8_t *_writeCacheBuf;
    uint32_t _writeCachePos;
    bool     _writeCache;

    uint32_t  findWrap(uint32_t highPos, uint32_t highKey, uint32_t lowPos, uint32_t lowKey);
    void      searchKey(CarbixLogRecord* callerRecord, const uint32_t key,
                        const uint32_t lowKey, const int32_t lowSerial, 
                        const uint32_t highKey, const int32_t highSerial);
      
};

#endif