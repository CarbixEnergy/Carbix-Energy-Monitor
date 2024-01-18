/*
  CarbixLog - Library for CarbixLog energy monitor
  Created by Carbix Energy
*/
#include "CarbixEnergyMonitor.h"

struct {
  uint32_t UNIXtime;
  uint32_t serial; 
} recordKey;

int CarbixLog::begin (const char* path ){
	if(CarbixFile) return 0;
	_path = charstar(path);
	if(!SD.exists(_path)){
		String logPath = _path;
		if(logPath.lastIndexOf('/') > 0){
			String  dir = logPath.substring(0,logPath.lastIndexOf('/'));
			if( ! SD.exists(dir)){
				if( ! SD.mkdir(dir)){
					Serial.printf_P(PSTR("mkdir failed: %s\r\n"), dir.c_str());
					return 2;
				}
			}
		}
		CarbixFile = SD.open(_path, FILE_WRITE);
		if(!CarbixFile){
			return 2;
		}
		CarbixFile.close();
	}
	CarbixFile = SD.open(_path, FILE_WRITE);
	if(!CarbixFile){
		return 2;
	}
	
	_fileSize = _physicalSize = CarbixFile.size();

	if(_fileSize){
			CarbixFile.seek(0);
			CarbixFile.read((uint8_t*)&recordKey, sizeof(recordKey));
			_firstKey = recordKey.UNIXtime;
			_firstSerial = recordKey.serial;
			CarbixFile.seek(_fileSize - _recordSize);
			CarbixFile.read((uint8_t*)&recordKey, sizeof(recordKey));
			_lastKey = recordKey.UNIXtime;
			_lastSerial = recordKey.serial;
			_entries = _fileSize / _recordSize;
	}

			// If there are trailing zero recordKeys at the end,
			// try to adjust _filesize down to match logical end of file.

	while(_fileSize && _lastSerial == 0){
		CarbixLogRecord* logRec = new CarbixLogRecord;
		_fileSize -= _recordSize;
		CarbixFile.seek(_fileSize - _recordSize);
		CarbixFile.read((uint8_t*)logRec, _recordSize);
		_lastSerial = logRec->serial;
		_lastKey = logRec->UNIXtime;
		_entries--;	 
		delete logRec;
	}
	
	if(_firstKey > _lastKey){
		_wrap = findWrap(0,_firstKey, _fileSize - _recordSize, _lastKey);
		CarbixFile.seek(_wrap);
		CarbixFile.read((uint8_t*)&recordKey, sizeof(recordKey));
		_firstKey = recordKey.UNIXtime;
		_firstSerial = recordKey.serial;
		CarbixFile.seek(_wrap - _recordSize);
		CarbixFile.read((uint8_t*)&recordKey, sizeof(recordKey));
		_lastKey = recordKey.UNIXtime;
		_lastSerial = recordKey.serial;
	}

	_lastReadKey = _firstKey;
	_lastReadSerial = _firstSerial;
	_maxFileSize = max(_fileSize, _maxFileSize);
	
	if(((int32_t) _lastSerial - _firstSerial + 1) != _entries){
		log("CarbixLog: file damaged %s\r\n", _path);
		log("CarbixLog: Creating diagnostic file.");
		dumpFile();
		log("CarbixLog: Deleting %s and restarting.\r\n", _path);
		CarbixFile.close();
		SD.remove(_path);
		ESP.restart();
	}
		
	for(int i=0; i<_cacheSize; i++){
		_cacheKey[i] = _firstKey;
		_cacheSerial[i] = _firstSerial;
	}

	return 0;
}

uint32_t CarbixLog::findWrap(uint32_t highPos, uint32_t highKey, uint32_t lowPos, uint32_t lowKey){
	struct {
		uint32_t UNIXtime;
		uint32_t serial; 
	} recordKey;
	
	if((lowPos - highPos) == _recordSize) {
			return lowPos;
	}
	uint32_t midPos = (highPos + lowPos) / 2;
	midPos += midPos % _recordSize;
	CarbixFile.seek(midPos);
	CarbixFile.read((uint8_t*)&recordKey, sizeof(recordKey));
	uint32_t midKey = recordKey.UNIXtime;
	if(midKey > highKey){
			return findWrap(midPos, midKey, lowPos, lowKey);
	}
	return findWrap(highPos, highKey, midPos, midKey);
}

int CarbixLog::readKey (CarbixLogRecord* callerRecord){
	uint32_t key = callerRecord->UNIXtime - (callerRecord->UNIXtime % _interval);
	if(!CarbixFile){
		return 2;
	}
	if(_entries == 0){
		return 1;
	}
	if(key < _firstKey){													// Before the beginning of time
		readSerial(callerRecord, _firstSerial);
		callerRecord->UNIXtime = key;
		return 1;
	}
	if(key >= _lastKey){														// Back to the future
		readSerial(callerRecord, _lastSerial);
		callerRecord->UNIXtime = key;
		if(key = _lastKey) return 0;
		return 1;
	}
	
	uint32_t lowKey = _firstKey;
	int32_t lowSerial = _firstSerial;
	uint32_t highKey = _lastKey;
	int32_t highSerial = _lastSerial;
	
	for(int i=0; i<_cacheSize; i++){
		uint32_t cacheKey = _cacheKey[i];
		if(cacheKey == key){												// Deja Vu
			_cacheWrap = (_cacheWrap + _cacheSize - 1) % _cacheSize;
			readSerial(callerRecord, _cacheSerial[i]);
			return 0;	
		}
		else if(cacheKey > lowKey && cacheKey < key) {
			lowKey = cacheKey;
			lowSerial = _cacheSerial[i];
		}
		else if(cacheKey < highKey && cacheKey > key){
			highKey = cacheKey;
			highSerial = _cacheSerial[i];
		}
	}
	if((highSerial - lowSerial) == 1) {						// Hole in file
		_cacheWrap = (_cacheWrap + _cacheSize - 1) % _cacheSize;
		readSerial(callerRecord, lowSerial);
		callerRecord->UNIXtime = key;
		return 0;	
	}
	searchKey(callerRecord, key, lowKey, lowSerial, highKey, highSerial);
	callerRecord->UNIXtime = key;
	return 0;
}

void CarbixLog::searchKey(CarbixLogRecord* callerRecord, const uint32_t key, const uint32_t lowKey, const int32_t lowSerial, const uint32_t highKey, const int32_t highSerial){

  	int32_t floorSerial = max(lowSerial, highSerial - (int32_t)((highKey - key) / _interval));
	int32_t ceilingSerial = min(highSerial, lowSerial + (int32_t)((key - lowKey) / _interval));

	//Serial.printf("low %d(%d), high %d(%d), floor %d, Ceiling %d\r\n", lowKey, lowSerial, highKey, highSerial,floorSerial, ceilingSerial); 

  	if(ceilingSerial < highSerial || floorSerial == ceilingSerial){
		readSerial(callerRecord, ceilingSerial);
		
		if(callerRecord->UNIXtime == key){
			return;
		}
		searchKey(callerRecord, key, lowKey, lowSerial, callerRecord->UNIXtime, callerRecord->serial);
		return;
	}
	if(floorSerial > lowSerial){
		readSerial(callerRecord, floorSerial);
		if(callerRecord->UNIXtime == key){
			return;
		}
		searchKey(callerRecord, key, callerRecord->UNIXtime, callerRecord->serial, highKey, highSerial);
		return;
	}
	if((highSerial - lowSerial) <= 1){		
		readSerial(callerRecord, lowSerial);
		return;
	}
	readSerial(callerRecord, (lowSerial + highSerial) / 2);
	_readKeyIO++;
	if(callerRecord->UNIXtime == key){
		return;
	}
	if(callerRecord->UNIXtime < key){
		searchKey(callerRecord, key, callerRecord->UNIXtime, callerRecord->serial, highKey, highSerial);
		return;
  }
  searchKey(callerRecord, key, lowKey, lowSerial, callerRecord->UNIXtime, callerRecord->serial);
  return;
}

int CarbixLog::readNext(CarbixLogRecord* callerRecord){
	if(!CarbixFile) return 2;
	if(callerRecord->serial == _lastSerial) return 1;
	return readSerial(callerRecord, callerRecord->serial + 1);
}

int CarbixLog::end(){
	CarbixFile.close();
	return 0;
}

boolean CarbixLog::isOpen(){
	if(CarbixFile) return true;
	return false;
}

uint32_t CarbixLog::firstKey(){return _firstKey;}
int32_t CarbixLog::firstSerial(){return _firstSerial;}
uint32_t CarbixLog::lastKey(){return _lastKey;}
int32_t CarbixLog::lastSerial(){return _lastSerial;}
uint32_t CarbixLog::fileSize(){return _fileSize;}
uint32_t CarbixLog::readKeyIO(){return _readKeyIO;}
uint32_t CarbixLog::interval(){return _interval;}

uint32_t CarbixLog::setDays(uint32_t days){
	_maxFileSize = max(_fileSize, (uint32_t)(days * _recordSize * (86400UL / _interval)));
	_maxFileSize = max(_maxFileSize, (uint32_t)(_recordSize * (3600UL / _interval)));
	return _maxFileSize / (_recordSize * (86400 / _interval));
}
  
int CarbixLog::readSerial(CarbixLogRecord* callerRecord, int32_t serial){
	if(serial < _firstSerial || serial > _lastSerial){
			return 1;
	}
	int pos = ((serial - _firstSerial) * _recordSize + _wrap) % _fileSize;
	if(_writeCache && pos >= _writeCachePos && pos < (_writeCachePos + CARBIXLOG_BLOCK_SIZE)){
		memcpy(callerRecord, _writeCacheBuf + (pos % CARBIXLOG_BLOCK_SIZE), _recordSize);
	}
	else {
		CarbixFile.seek(pos);
		CarbixFile.read((uint8_t*)callerRecord, _recordSize);
	}
	_cacheKey[_cacheWrap] = callerRecord->UNIXtime;
	_cacheSerial[_cacheWrap++] = callerRecord->serial;
	_cacheWrap %= _cacheSize;
	_readKeyIO++;
	return 0;
};
   
int CarbixLog::write (CarbixLogRecord* callerRecord){

	if(!CarbixFile){
			return 2;
	}
	if(callerRecord->UNIXtime <= _lastKey) {
			return 1;
	}
	callerRecord->serial = ++_lastSerial;
	_lastKey = callerRecord->UNIXtime;

		// if log is (or should) wrap,
		// overwrite oldest and set first to following record.

	if(_wrap || _fileSize >= _maxFileSize){
		if(_writeCache){
			writeCache(false);
		}
		CarbixFile.seek(_wrap);
		_wrap = (_wrap + _recordSize) % _fileSize;
		CarbixFile.write((char*)callerRecord, _recordSize);
		CarbixFile.read((uint8_t*)callerRecord,8);
		_firstKey = callerRecord->UNIXtime;
		_firstSerial = callerRecord->serial;
		callerRecord->UNIXtime = _lastKey;
		callerRecord->serial = _lastSerial;
		return 0;
	}

		// not wrapped.

		// If write cache active,
		// Add record to cache
		// Write cache block if full

	if (_writeCache){
		memcpy(_writeCacheBuf + (_fileSize % CARBIXLOG_BLOCK_SIZE), callerRecord, _recordSize);
		_fileSize += _recordSize;
		if((_fileSize % CARBIXLOG_BLOCK_SIZE) == 0){
			CarbixFile.seek(_writeCachePos);
			CarbixFile.write(_writeCacheBuf, CARBIXLOG_BLOCK_SIZE);
			_writeCachePos += CARBIXLOG_BLOCK_SIZE;
			CarbixFile.flush();
			_physicalSize = CarbixFile.size();
		}
	}

		// No write cache active
		// If no prewrite records (SD wear reduction)
		// Write this record and follow with prewrite records.

	else if(_fileSize == _physicalSize){
		CarbixLogRecord formatRecord;
		CarbixFile.seek(_fileSize);
		CarbixFile.write((char*)callerRecord, _recordSize);
		_fileSize += _recordSize;
		_physicalSize += _recordSize;
		int count = _preformat;
		while(count-- && _physicalSize < _maxFileSize){
			CarbixFile.write((char*)&formatRecord, _recordSize);
			_physicalSize += _recordSize;
		}
		CarbixFile.flush();
	}

		// No write cache active
		// write this record over a prewrite record

	else {
		CarbixFile.seek(_fileSize);
		CarbixFile.write((char*)callerRecord, _recordSize);
		_fileSize += _recordSize;
	}

	_entries++;
	if(_firstKey == 0){
		_firstKey = callerRecord->UNIXtime;
	}
	return 0;
}

void CarbixLog::writeCache(bool on){
	if((on && _writeCache) || (!on && !_writeCache)){
		return;
	}
	if(on){
		_writeCacheBuf = new uint8_t[CARBIXLOG_BLOCK_SIZE];
		_writeCachePos = _fileSize & ~(CARBIXLOG_BLOCK_SIZE - 1);
		CarbixFile.seek(_writeCachePos);
		CarbixFile.read(_writeCacheBuf, MIN(_physicalSize - _writeCachePos, CARBIXLOG_BLOCK_SIZE));
		_writeCache = true;
	}
	else {
		CarbixFile.seek(_writeCachePos);
		CarbixFile.write(_writeCacheBuf, MIN(_fileSize - _writeCachePos, CARBIXLOG_BLOCK_SIZE));
		CarbixFile.flush();
		_physicalSize = CarbixFile.size();
		delete[] _writeCacheBuf;
		_writeCacheBuf = nullptr;
		_writeCache = false;
	}
} 

void CarbixLog::dumpFile(){
	setLedCycle(LED_DUMPING_LOG);
	char diagPath[] = "CarbixEnergyMonitor/logDiag.txt";
	SD.remove(diagPath);
	File logDiag = SD.open(diagPath, FILE_WRITE);
	if(logDiag){
		DateTime now = DateTime(localTime());
	logDiag.printf_P(PSTR("%d/%02d/%02d %02d:%02d:%02d\r\nfilesize %d, entries %d\r\n"),
	now.month(), now.day(), now.year()%100, now.hour(), now.minute(), now.second(),
		CarbixFile.size(), _entries);
		logDiag.close();
	}
	CarbixFile.seek(0);
	CarbixFile.read((uint8_t*)&recordKey,sizeof(recordKey));
	uint32_t begKey = recordKey.UNIXtime;
	uint32_t begSerial = recordKey.serial;
	uint32_t endKey = recordKey.UNIXtime;
	uint32_t endSerial = recordKey.serial;
	uint32_t filePos = 0;
  	do {
		filePos += _recordSize;
		CarbixFile.seek(filePos);
		CarbixFile.read((uint8_t*)&recordKey,sizeof(recordKey));
		if(recordKey.UNIXtime - endKey != _interval || recordKey.serial - endSerial != 1 || filePos >= _fileSize){
			Serial.printf_P(PSTR("%d,%d,%d,%d\r\n"), begKey, begSerial, endKey, endSerial);
			logDiag = SD.open(diagPath, FILE_WRITE);
			if(logDiag){
				logDiag.printf_P(PSTR("%d,%d,%d,%d\r\n"), begKey, begSerial, endKey, endSerial);
				if(filePos >= CarbixFile.size()){
					logDiag.printf_P(PSTR("End of file\r\n"));
				}
				logDiag.close();
			}
			begKey = recordKey.UNIXtime;
			begSerial = recordKey.serial;
		}
		endKey = recordKey.UNIXtime;
		endSerial = recordKey.serial;
	} while(filePos < CarbixFile.size());
	endLedCycle();
}
