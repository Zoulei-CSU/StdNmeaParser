#include "StdNmeaParser.h"


//注意：当字符串为空时，也会返回一个空字符串
static void split(const std::string& s, const std::string& delim, std::vector< std::string >* ret)
{
	size_t last = 0;
	size_t index = s.find_first_of(delim, last);
	while (index != std::string::npos)
	{
		ret->push_back(s.substr(last, index - last));
		last = index + 1;
		index = s.find_first_of(delim, last);
	}
	if (index - last > 0)
	{
		ret->push_back(s.substr(last, index - last));
	}
}

StdNmeaParser::StdNmeaParser()
{
	reset();
}

void StdNmeaParser::reset()
{
	_bufferIndex = 0;
	_state = S_STATE_SEARCH;

	_stasInUse.clear();
	_stasInfoInView.clear();
	_stasInfoInView_temp.clear();
}

void StdNmeaParser::parseBuffer(const char *buffer, unsigned int maxLength)
{
	unsigned int i = 0;
	while (i < maxLength && buffer[i] != '\0')
	{
		processChar(buffer[i]);
		++i;
	}
}

NmeaTime StdNmeaParser::getTime()
{
	return _time;
}

NmeaPosition StdNmeaParser::getPosition()
{
	return _position;
}

NmeaDOP StdNmeaParser::getDOP()
{
	return _dop;
}

NmeaRMS StdNmeaParser::getRMS()
{
	return _rms;
}

std::vector<NmeaSta> StdNmeaParser::getSatelliteInfo()
{
	return _stasInfoInView;
}

void StdNmeaParser::processChar(char c)
{
	switch (_state)
	{
	case StdNmeaParser::S_STATE_SEARCH://查找语句开头
		if (c == '$')
		{
			_bufferIndex = 0;
			_state = S_STATE_DATA;
		}
		else if (c == '@')
		{
		}
		break;
	case StdNmeaParser::S_STATE_DATA://接收数据
		if (c == '\r')
		{
			_state = S_STATE_END;//如果是\r准备结束
		}
		else
		{
			if (_bufferIndex < BUFFER_LEN)
				_buffer[_bufferIndex++] = c;
			else
				_state = S_STATE_SEARCH;//缓存满了，说明数据有问题，重新搜索 
		}
		break;
	case StdNmeaParser::S_STATE_END:
		if (c == '\n')
		{
			//语句真的结束了
			_buffer[_bufferIndex] = '\0';
			processNMEA();
		}
		_state = S_STATE_SEARCH;//重新搜索
		break;
	default:
		break;
	}
}

void StdNmeaParser::processNMEA()
{
	std::vector< std::string > cmds;
	split(std::string(_buffer), std::string(","), &cmds);

	std::string flag(_buffer + 2, 3);
	if (flag.compare("GGA") == 0)
		processGGA(cmds);
	else if (flag.compare("GSA") == 0)
		processGSA(cmds);
	else if (flag.compare("GSV") == 0)
		processGSV(cmds);
	else if (flag.compare("GST") == 0)
		processGST(cmds);
	else if (flag.compare("ZDA") == 0)
		processZDA(cmds);
}

void StdNmeaParser::processGGA(std::vector< std::string > &cmd)
{
	if (cmd.size() != 15)
		return;

	std::string buf;

	//UTC时间，格式为hhmmss.sss
	{
		buf = cmd[1].substr(0, 2);//hour
		_time.hour = atoi(buf.c_str());
		buf = cmd[1].substr(2, 2);//minute
		_time.minute = atoi(buf.c_str());
		buf = cmd[1].substr(4, 2);//second
		_time.second = atoi(buf.c_str());
	}
	
	//纬度，格式为ddmm.mmmm(第一位是零也将传送)
	{
		buf = cmd[2].substr(0, 2);
		_position.latitude = atoi(buf.c_str());
		buf = cmd[2].substr(2);
		_position.latitude += atof(buf.c_str()) / 60.0;

		//纬度半球，N或S(北纬或南纬)
		if (cmd[3].compare("S") == 0)
			_position.latitude *= -1;
	}

	//经度，格式为dddmm.mmmm(第一位零也将传送)
	{
		buf = cmd[4].substr(0, 3);
		_position.longitude = atoi(buf.c_str());
		buf = cmd[4].substr(3);
		_position.longitude += atof(buf.c_str()) / 60.0;

		//经度半球，E或W(东经或西经)
		if (cmd[5].compare("W") == 0)
			_position.longitude *= -1;
	}

	//GPS状态， 0初始化， 1单点定位， 2码差分， 3无效PPS， 4固定解， 5浮点解， 6正在估算 7，人工输入固定值， 8模拟模式， 9WAAS差分
	buf = cmd[6];
	_position.status = atoi(buf.c_str());
	if (_position.status == 0)
	{
		_position.latitude = _position.longitude = _position.altitude = 0;
	}

	//使用卫星数量，从00到12(第一个零也将传送)
	buf = cmd[7];
	_position.satsInUse = atoi(buf.c_str());

	//HDOP-水平精度因子，0.5到99.9，一般认为HDOP越小，质量越好。
	buf = cmd[8];
	_dop.hdop = atof(buf.c_str());

	//椭球高，-9999.9到9999.9米
	buf = cmd[9];
	_position.altitude = atof(buf.c_str());
}

void StdNmeaParser::processGSA(std::vector< std::string > &cmd)
{
	if (cmd.size() != 18)
		return;

	//先判断调用了多少次GSA，如果调用次数太多就清空一下列表，去掉已经丢失的卫星
	static short times = 0;
	if (times++ > 6)
		_stasInUse.clear();

	unsigned int prn;
	//PRN码（伪随机噪声码），信道正在使用的卫星PRN码编号（00）（前导位数不足则补0）
	for (int i = 3; i <= 14; ++i)
	{
		if (cmd[i].size() > 0)
		{
			prn = atoi(cmd[i].c_str());
			if (std::find(_stasInUse.begin(), _stasInUse.end(),prn) == _stasInUse.end())
			{
				_stasInUse.push_back(prn);
			}
		}
	}

	//PDOP综合位置精度因子（0.5 - 99.9）
	_dop.pdop = atof(cmd[15].c_str());

	//HDOP水平精度因子（0.5 - 99.9）
	_dop.hdop = atof(cmd[16].c_str());

	//VDOP垂直精度因子（0.5 - 99.9）
	//_dop.vdop = atof(cmd[17].c_str());
	int pos = cmd[17].find_first_of('*');
	if (pos != std::string::npos)
		_dop.vdop = atof(cmd[17].substr(0, pos).c_str());
}

void StdNmeaParser::processGSV(std::vector< std::string > &cmd)
{
	int len = cmd.size();
	const char *flag = cmd[0].c_str();

	int i = 4;
	while (true)
	{
		if (i + 3 >= len)
			break;

		NmeaSta s;
		s.type = *(flag + 1);
		s.prn = atoi(cmd[i].c_str());
		s.elv = atoi(cmd[i + 1].c_str());
		s.azi = atoi(cmd[i + 2].c_str());
		s.sn = atoi(cmd[i + 3].c_str());

		if (std::find(_stasInUse.begin(), _stasInUse.end(), s.prn) != _stasInUse.end())
		{
			s.used = true;
		}

		_stasInfoInView_temp.push_back(s);
		i += 4;
	}

	//GSV语句收完了
	if (cmd[1] == cmd[2])
	{
		_stasInfoInView = _stasInfoInView_temp;
		_stasInfoInView_temp.clear();
	}
}

void StdNmeaParser::processGST(std::vector< std::string > &cmd)
{
	if (cmd.size() != 9)
		return;

	_rms.totalRMS = atof(cmd[2].c_str());
	_rms.latRMS = atof(cmd[6].c_str());
	_rms.lngRMS = atof(cmd[7].c_str());

	int pos = cmd[8].find_first_of('*');
	if (pos != std::string::npos)
		_rms.altRMS = atof(cmd[8].substr(0, pos).c_str());
}

void StdNmeaParser::processZDA(std::vector< std::string > &cmd)
{
	if (cmd.size() != 7)
		return;

	_time.day = atoi(cmd[2].c_str());
	_time.month = atoi(cmd[3].c_str());
	_time.year = atoi(cmd[4].c_str());
}

