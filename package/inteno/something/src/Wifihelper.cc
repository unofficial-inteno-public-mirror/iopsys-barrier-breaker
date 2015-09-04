#include "Wifihelper.h"
#include "SomethingListener.h"

QStatus Wifihelper::getInfo(string& Ssid, string& key, int* chanel2G, int* chanel5G){
    MsgArg value;
    char *str;
    QStatus status = remotObj.GetProperty("org.allseen.WiFi", "Ssid", value, 5000);
    if(status == ER_OK){
        status = value.Get("s", &str);
        if(status == ER_OK){
            Ssid = string(str);
        }
        else return status;
    }else{
        return status;
    }
    status = remotObj.GetProperty("org.allseen.WiFi", "Key", value, 5000);
    if(status == ER_OK){
        status = value.Get("s", &str);
        if(status == ER_OK){
            key = string(str);
        }
        else return status;
    }else{
        return status;
    }
    status = remotObj.GetProperty("org.allseen.WiFi", "Channel2g", value, 5000);
    if(status == ER_OK){
        value.Get("i", chanel2G);
    }else{
        return status;
    }
    status = remotObj.GetProperty("org.allseen.WiFi", "Channel5g", value, 5000);
    if(status == ER_OK){
        value.Get("i", chanel5G);
    }
    return status;
}

QStatus Wifihelper::applyChanges(){
	return remotObj.MethodCall("org.allseen.WiFi", "ApplyChanges", NULL, 0, 0);
}

int* Wifihelper::getChannels(Message message, int channel, size_t* numChannels){
    MsgArg values[1];
    values[0].Set("i", channel);
    QStatus status = remotObj.MethodCall("org.allseen.WiFi", "GetChannels", values, 1, message, 5000, 0);
    if(status == ER_OK){
    	const MsgArg *retargs = message->GetArg(0);
        size_t numArgs;
        int *values;
        retargs->Get("ai", &numArgs, &values);
        /*cout << "successfully retreved " << numArgs << " channels" << endl;
        for(size_t i = 0; i < numArgs; i++){
            cout << "channel: " << values[i] << endl;          
       }*/
       int* list = (int*)malloc(50*sizeof(int));
       for(int i = 0; i < numArgs; i++){
       		list[i] = values[i];
       }
       *numChannels = numArgs;
       return list;
    }else{
        cout << "error getting channels" << SomethingListener::QStatusToString(status) << endl;
        return NULL;
    }
}

QStatus Wifihelper::changeSSID(const char* SSID){    
    QStatus status = remotObj.SetProperty("org.allseen.WiFi", "Ssid", SSID, 5000);
    if (ER_OK == status) {
        return applyChanges();
    }
    return status;
}

QStatus Wifihelper::setChannel2G(const int channel){    
    QStatus status = remotObj.SetProperty("org.allseen.WiFi", "Channel2g", channel, 5000);
    if (ER_OK == status) {
        return applyChanges();
    }
    return status;
}

QStatus Wifihelper::setChannel5G(const int channel){    
    QStatus status = remotObj.SetProperty("org.allseen.WiFi", "Channel5g", channel, 5000);
    if (ER_OK == status) {
        return applyChanges();
    }
    return status;
}

QStatus Wifihelper::changePass(const char* key){    
    QStatus status = remotObj.SetProperty("org.allseen.WiFi", "Key", key, 5000);
    if (ER_OK == status) {
        return applyChanges();
    }
    return status;
}

