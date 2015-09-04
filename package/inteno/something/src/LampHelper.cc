#include "LampHelper.h"
#include "SomethingListener.h"




QStatus LampHelper::setColorTemp(const uint32_t& colortemp){
    const uint32_t convertedColorTemp = mapRanges(colortemp, 0, 100, 0x10000, 0xffffffff/2);
    return remotObj.SetProperty("org.allseen.LSF.LampState", "ColorTemp", convertedColorTemp, 5000);
}

QStatus LampHelper::setBrightness(const uint32_t& brightness){
    const uint32_t convertedBrightness = mapRanges(brightness, 0, 100, 0x10000, 0xffffffff);
    return remotObj.SetProperty("org.allseen.LSF.LampState", "Brightness", convertedBrightness, 5000);
}

QStatus LampHelper::setOnOff(const bool& onoff){
	MsgArg value;
	QStatus status = value.Set("b", onoff);
    cout << "status setOnOff = " << SomethingListener::QStatusToString(status) << endl;
    return remotObj.SetProperty("org.allseen.LSF.LampState", "OnOff", value, 5000);
}
QStatus LampHelper::getOnOff(bool& onoff){
	MsgArg value;
	bool boo;
	QStatus status = remotObj.GetProperty("org.allseen.LSF.LampState", "OnOff", value, 5000);
	if(status == ER_OK){
		status = value.Get("b", &boo);
		if(status == ER_OK){
			onoff = boo;
		}
	}
	return status;
}	


uint32_t LampHelper::mapRanges(float x, float inLow, float inHigh, float outLow, float outHigh){
    if(x < inLow)return (uint32_t)outLow;
    if(x > inHigh)return (uint32_t)outHigh;
    return (uint32_t)((x-inLow)*(outHigh-outLow)/(inHigh-inLow) + outLow);
}


QStatus LampHelper::getInfo(uint32_t& brightness, uint32_t& colortemp){
    MsgArg value;
    uint32_t uint;
    QStatus status = remotObj.GetProperty("org.allseen.LSF.LampState", "ColorTemp", value, 5000);
    if(status == ER_OK){
        status = value.Get("u", &uint);
        if(status == ER_OK){
            colortemp = mapRanges((float)uint, (float)0x10000, (float)0xffffffff/2, (float)0, (float)100);
        }else{
            return status;
        }
    }else{
        return status;
    }
    status = remotObj.GetProperty("org.allseen.LSF.LampState", "Brightness", value, 5000);
    if(status == ER_OK){
        status = value.Get("u", &uint);
        if(status == ER_OK){
            brightness = mapRanges((float)uint, (float)0x10000, (float)0xffffffff, (float)0, (float)100);
        }
    }
    return status;
}
