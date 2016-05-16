#include <string>
#include <iostream>
#include <stdio.h>
#include <ctime>
#include <unistd.h>

using namespace std;



string getTime()
{
	time_t now;
	    time(&now);
	    char buf[sizeof "2011-10-08T07:07:09Z"];
	    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
	    // this will work too, if your compiler doesn't support %F or %T:
	    //strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

	    string str(buf);

	    return buf;
}




int main() {
	FILE *in;
	char buff[512];

	cout << "IoTX poc" << endl;

	cout << endl << endl;

	string userName = "suru.dissanaike@himinds.com";
	string password = "Apelsin555";
	char value[4] = { '\0'};

	string test = "";

	string curl2 = "curl -u suru.dissanaike@himinds.com:Apelsin555 https://himinds.cumulocity.com/inventory/ | python -mjson.tool";

	//cout << curl1;


	for(int i = 0; i < 10; i++)
	{

		string curl1 = "curl -v -u " +userName +":" +password +" -H ";

		curl1 += "'Accept: application/vnd.com.nsn.cumulocity.measurement+json; charset=UTF-8; ver=0.9' -H 'Content-type: application/vnd.com.nsn.cumulocity.measurement+json; charset=UTF-8; ver=0.9' -X POST -d ";
		curl1 += "'{\"c8y_TemperatureMeasurement\":{\"T\":{\"value\":";
		curl1 += value;
		curl1 += ",\"unit\":\"C\"}},\"time\":\"" +getTime() +"\",\"source\":{\"id\":\"10322\"},\"type\":\"c8y_PTCMeasurement\"}' http://himinds.cumulocity.com/measurement/measurements/ | python -mjson.tool";


		if(!(in = popen(curl1.c_str(), "r"))){
			return 1;
		}

		while(fgets(buff, sizeof(buff), in)!=NULL){
			cout << buff;
		}

		
		sprintf(value, "l%d", i);

		sleep(10);
	}

	pclose(in);

    std::cout << getTime() << "\n";

	return 0;
}

