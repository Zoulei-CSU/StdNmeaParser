#ifndef MY_NMEA_PARSER_H
#define MY_NMEA_PARSER_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#define BUFFER_LEN 256

struct NmeaSta
{
	unsigned int prn;
	unsigned int azi;
	unsigned short elv;
	unsigned short sn;
	bool used;
	char type;//'P' : GPS, 'L' : Glonass, 'B' : Beidou

	NmeaSta(){ prn = 0; sn = 0; used = false; type = '\0'; }
};

struct NmeaTime
{
	unsigned int year;
	unsigned short month;
	unsigned short day;

	unsigned short hour;
	unsigned short minute;
	unsigned short second;
};

struct NmeaPosition
{
	double latitude;
	double longitude;
	double altitude;

	unsigned short status;//0初始化， 1单点定位， 2码差分， 3无效PPS， 4固定解， 5浮点解， 6正在估算 7，人工输入固定值， 8模拟模式， 9WAAS差分
	unsigned short satsInUse;
};

struct NmeaDOP
{
	double pdop;
	double hdop;
	double vdop;

	NmeaDOP(){ pdop = hdop = vdop = 99; }
};

struct NmeaRMS
{
	double totalRMS;//Total RMS standard deviation of ranges inputs to the navigation solution
	double latRMS;//Standard deviation (meters) of latitude error
	double lngRMS;//Standard deviation (meters) of longitude error
	double altRMS;//Standard deviation (meters) of latitude error

	NmeaRMS(){ totalRMS = latRMS = lngRMS = altRMS = 1e6; }
	double getHRMS(){ return sqrt(latRMS * latRMS + lngRMS * lngRMS); }
	double getVRMS(){ return altRMS; }
};

class StdNmeaParser
{
public:
	StdNmeaParser();
	void reset();
	void parseBuffer(const char *buffer,unsigned int maxLength);

	NmeaTime getTime();
	NmeaPosition getPosition();
	NmeaDOP getDOP();
	NmeaRMS getRMS();
	std::vector<NmeaSta> getSatelliteInfo();

private:
	void processChar(char c);
	void processNMEA();
	void processGGA(std::vector< std::string > &cmd);
	void processGSA(std::vector< std::string > &cmd);
	void processGSV(std::vector< std::string > &cmd);
	void processGST(std::vector< std::string > &cmd);
	void processZDA(std::vector< std::string > &cmd);

private:
	enum S_STATE
	{
		S_STATE_SEARCH = 0,		// 还没开始
		S_STATE_DATA,						// 接收普通数据
		S_STATE_END,				// 停止符号
		S_STATE_OTHER_DATA			//用于接收其他数据包，比如@符号开头的数据包
	};

private:
	char _buffer[BUFFER_LEN];
	unsigned short _bufferIndex;//缓冲区写入位置

	S_STATE _state;//接收语句状态

	NmeaTime _time;
	NmeaPosition _position;
	NmeaDOP _dop;
	NmeaRMS _rms;
	std::vector<unsigned int> _stasInUse;
	std::vector<NmeaSta> _stasInfoInView;
	std::vector<NmeaSta> _stasInfoInView_temp;
};

#endif