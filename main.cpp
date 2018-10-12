#include <iostream>
#include <cstring>
#include "StdNmeaParser.h"

const char *buffer = "$GNGGA,024338.00,2307.55027943,N,11322.10352207,E,1,23,0.6,39.014,M,-6.581,M,,*5D\r\n"
					 "$GNGSA,A,3,10,12,15,32,18,24,31,21,25,20,,,1.2,0.6,1.0*24\r\n"
					 "$GNGSA,A,3,69,70,73,68,79,80,,,,,,,1.2,0.6,1.0*2C\r\n"
					 "$GNGSA,A,3,106,109,107,110,101,103,104,,,,,,1.2,0.6,1.0*16\r\n"
					 "$GPGSV,7,1,23,20,18,134,44,10,56,334,50,15,13,076,44,24,31,037,43*70\r\n"
					 "$GPGSV,7,2,23,31,22,228,45,25,38,144,45,32,35,310,49,21,33,203,46*72\r\n"
					 "$GPGSV,7,3,23,12,36,089,50,18,82,049,50*75\r\n"
					 "$GLGSV,7,4,23,79,31,029,46,73,33,200,48,68,17,108,44,80,83,159,48*6B\r\n"
					 "$GLGSV,7,5,23,70,31,341,46,69,50,059,45*60\r\n"
					 "$GBGSV,7,6,23,110,47,199,46,103,61,187,43,104,31,111,39,107,70,170,48*60\r\n"
					 "$GBGSV,7,7,23,109,52,345,46,106,61,038,48,101,49,128,42*62\r\n"
					 "$GNGST,024346.00,7.965,0.746,0.684,114.8,0.695,0.736,1.293*41\r\n"
					 "$GNZDA,024338.01,02,03,2017,00,00*72\r\n";

int main(int argc, char *argv[])
{
	std::cout << buffer << std::endl << std::endl;

	StdNmeaParser smp;
	smp.parseBuffer(buffer, strlen(buffer)); //送入数据，会自动计算，更新结果

	std::cout << "Resault:" << std::endl;

	NmeaTime t = smp.getTime();
	std::cout << "Time:" << t.year << "-" << t.month << "-" << t.day << "\t";
	std::cout << t.hour << ":" << t.minute << ":" << t.second << std::endl << std::endl;

	NmeaPosition pos = smp.getPosition();
	std::cout << "Position:" << std::endl
			  << "status :" << pos.status << std::endl
			  << "Satellites in use :" << pos.satsInUse << std::endl;
	std::cout << "B:" << pos.latitude << "\tL:" << pos.longitude << "\tH:" << pos.altitude << std::endl << std::endl;

	std::vector<NmeaSta> staInfo = smp.getSatelliteInfo();
	std::cout << "Satellites Count :" << staInfo.size() << std::endl;

	std::cout << "Type\tUsed\tPrn\tSn\tAzi\tElv" << std::endl;
	for (auto sta : staInfo)
	{
		std::cout << sta.type << "\t"
				  << (sta.used ? 'T' : 'F') << "\t"
				  << sta.prn << "\t"
				  << sta.sn << "\t"
				  << sta.azi << "\t"
				  << sta.elv << std::endl;
	}

	return 0;
}
