#include "ble_to_hc08.h"


BLEUUID serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");
BLEUUID    write_charUUID("0000ffe1-0000-1000-8000-00805f9b34fb");

String blue_receive_txt = "";
int blue_cmd_state = 0;    //全局共享蓝牙命令处理状态
bool blue_connected = false;
char * blue_name = "edp47_ink";
bool blue_doConnect = false;
String  blue_server_address = "";
int scan_time = 60; //扫描秒数时长

BLEAdvertisedDevice* myDevice;

#ifdef no_blue_notify
//会分好几次收到。。。
//blue_cmd_state=2只表示至少收到1次
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic_tmp,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
  Serial.print(">>>> 收到返回 <");
  char tmp_cArr[length + 1] = {0};
  memcpy((uint8_t *)tmp_cArr, pData, length);
  tmp_cArr[length] = '\0';
  Serial.print(pBLERemoteCharacteristic_tmp->getUUID().toString().c_str());
  Serial.print("> 长度=");
  Serial.println(length);
  Serial.println((char*)tmp_cArr);
  blue_receive_txt = blue_receive_txt + String(tmp_cArr);

  //收到>>>结束标志，设置标志位
  if (blue_receive_txt.endsWith(">>>"))
  {
    blue_receive_txt.replace(">>>", "");
    blue_cmd_state = 3;
  }

  if (blue_cmd_state == 1)
    blue_cmd_state = 2;
}
#endif

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.println("blue onConnect");
    }

    void onDisconnect(BLEClient* pclient) {
      blue_connected = false;
      Serial.println("blue onDisconnect");
    }
};


/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    //打印找到的服务
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("查到蓝牙服务: ");
      Serial.println(advertisedDevice.toString().c_str());

      Serial.println(String("  ") + advertisedDevice.getAddress().toString().c_str());
      //  Serial.println(advertisedDevice.getServiceDataUUID().toString().c_str());   返回:NULL
      // Serial.println(advertisedDevice.getServiceUUID().toString().c_str());  打印异常！
      Serial.println("  " + String(advertisedDevice.getName().c_str()));

      //首次上电查询时，getName能获得正确名称，再次扫描时会扫不到名称
      //算法中记忆上次的地址，下次时直接用
      if ( String(advertisedDevice.getName().c_str()) == blue_name ||
           ( blue_server_address.length() > 0 &&  String(advertisedDevice.getAddress().toString().c_str()) == blue_server_address)
         )
      {
        blue_server_address = advertisedDevice.getAddress().toString().c_str();
        Serial.println("找到蓝牙BLE服务");
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        blue_doConnect = true;
        //blue_doScan = true;
      }

    } // onResult
}; // MyAdvertisedDeviceCallbacks


Manager_blue_to_hc08::Manager_blue_to_hc08() {
  //初始化蓝牙客户端
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  //找到服务后回调
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  //pBLEScan->start(5, false);
  Serial.println("blue_to_hc08 init");

}

Manager_blue_to_hc08::~Manager_blue_to_hc08() {
  //client.stop();
  //WiFi.disconnect();
}


bool Manager_blue_to_hc08::connectToServer() {
  Serial.print("A.连接蓝牙地址: ");
  Serial.println(myDevice->getAddress().toString().c_str());

  if (pClient_ok == false)
  {
    pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient_ok = true;
  }

  Serial.print("B.开始连接");

  //这条语句在连接不上时会一直阻塞，导致程序跑飞！！！
  //一般出现在上次连接成功，下次省去扫描步骤的情况，所以最好不要省去蓝牙扫描的步骤！
  pClient->connect(myDevice);

  Serial.println(String("C.连接服务: ") + serviceUUID.toString().c_str());
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(String("D.查找特征-写入: ") + write_charUUID.toString().c_str());

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(write_charUUID );
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(write_charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

#ifdef no_blue_notify
  Serial.println("E.registerForNotify") ;
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);
#endif

  blue_connected = true;
  return true;
}


//按hc-08蓝牙手册:
//  9600bps 波特率以下每个数据包不要超出 500 个字节，
//  发 20 字节间隔时间(ms) 20ms
//字串长于20字符必须特殊处理,分拆发送，否则只能发出部分数据
void Manager_blue_to_hc08::send_cmd_long(String cmd)
{
  //将字串拆成20个字节一组多次发出
  char cArr[cmd.length() + 1];
  cmd.toCharArray(cArr, cmd.length() + 1);
  uint8_t * sound_bodybuff;
  sound_bodybuff = (uint8_t *)cArr;
  int all_send_num = 0; //已经发送出去的字节
  int send_num = 0;     //本次准备发送出去的字节
  while (all_send_num < cmd.length())
  {
    if (cmd.length() - all_send_num > 20)
      send_num = 20;
    else
      send_num =  cmd.length() - all_send_num ;
    pRemoteCharacteristic->writeValue(sound_bodybuff + all_send_num, send_num);
    all_send_num = all_send_num + send_num;
    delay(20);
  }
  //Serial.println(">> 发出长度 " + String(cmd.length() ));
  //Serial.println(">> 发出长度 all_send_num=" + String(all_send_num));
}



//蓝牙发送命令串
//delay_sec 最长等待秒数，超时
//delay_sec 最少等待秒数
bool Manager_blue_to_hc08::blue_send_cmd(String cmd, float delay_sec, int min_delay_sec)
{
  bool ret = false;
  blue_receive_txt = "";

  Serial.println(">> 发出命令 \"" + cmd + "\"");
  //pRemoteCharacteristic->writeValue((uint8_t*)cmd.c_str(), cmd.length());
  send_cmd_long(cmd);


#ifdef blue_notify
  uint32_t cmd_starttime = millis() / 1000; //蓝牙字符串命令发出时间
  blue_cmd_state = 1;   //命令发送状态

  while (true)
  {
    if (millis() / 1000 - cmd_starttime > delay_sec)
    {
      Serial.println("command [" + cmd + "] timeout! " + String(int(millis() / 1000 - cmd_starttime)) + "秒!");
      break;
    }
    if (millis() / 1000 < cmd_starttime)
      cmd_starttime = millis() / 1000;

    //检查此状态值是否修改成返回状态
    if (blue_cmd_state == 2)
    {
      ret = true;
      //保底信息，不跳出，要等到时间用完！
      //break;
    }
    else if (blue_cmd_state == 3)
    {
      ret = true;
      //跳出
      break;
    }
    yield();
    delay(10);
  }
  Serial.println("用时:" + String(millis() / 1000 - cmd_starttime) + "秒！");
  return ret;
#else
  Serial.println("sleep..." + String(min_delay_sec) + "秒!");
  delay(1000 * min_delay_sec);
#endif

  return true;
}




//注意算法,尽量不重新创建对象
void Manager_blue_to_hc08::blue_connect_sendmsg(String txt, bool quick)
{
  uint32_t sendmsg_time = millis() / 1000;

  //不允许在非退出状态插入新任务
  if (blue_doConnect_step > 0) return;

  blue_doConnect = false;
  blue_connected = false;
  blue_datasend = false;

  //1.扫描
  //注：如果扫描时已被其它蓝牙客户端连接，或者蓝牙服务器已关停， 扫描会找不到蓝牙服务端，这样能阻止下一步连接服务器造成错误
  //代价是每次扫描会占用一些时间。
  blue_doConnect_step = 1;
  Serial.println("1.开始蓝牙扫描目标服务器");

  //上次找到过蓝牙地址，跳过蓝牙扫描，省略会带来阻塞问题，最好不要省去此步！
  if (quick)
  {
    if (blue_server_address.length() > 0)
    {
      blue_doConnect = true;
      Serial.println("skip 蓝牙扫描");
    }
    else
      pBLEScan->start(scan_time, false);  //阻塞式查找蓝牙设备
  }
  else
    pBLEScan->start(scan_time, false);  //阻塞式查找蓝牙设备

  blue_doConnect_step = 2;
  //2.连接目标蓝牙服务器
  //有可能在连接时阻塞，建议用定时狗，如异常就重启！！！
  if (blue_doConnect == true) {
    Serial.println("2.连接目标蓝牙服务器");
    if (connectToServer()) {
      Serial.println("检索 BLE Server 服务成功！");
    } else {
      Serial.println("检索 BLE Server 服务失败！");
    }
  }

  blue_doConnect_step = 3;
  //3.蓝牙服务器连接上
  if (blue_connected == true)
  {
    Serial.println("3.向蓝牙服务器发送数据");
    //3.1发出唤醒命令，远程唤醒蓝牙服务端
    //时间要略长一点，太快有可能蓝牙端重启没收到信息,最少要2秒，否则服务端收不到数据
    bool ret = blue_send_cmd("waker", 2, 2);
    delay(500);
    //理论应不返回数据
    Serial.println(">>>" + blue_receive_txt);

    //3.2发送文本串数据
    //如果不数据长度较大，5秒限时可能要加长
    ret = blue_send_cmd(txt + ">>>", 5, 1);
    Serial.println(">>>" + blue_receive_txt);

    //全部任务执行完毕，可以开始休眠
    blue_datasend = true;
  }

  blue_doConnect_step = 4;
  //4.关闭连接(如果连接上服务器)
  if (blue_connected == true)
  {
    Serial.println("4.蓝牙服务器断开");
    pClient->disconnect();
  }

  //非工作状态
  blue_doConnect_step = 0;

  //经验值：5-6秒(接收通知)
  //      3-4秒(不接收通知)
  Serial.println("总用时:" + String(millis() / 1000 - sendmsg_time) + "秒！");
}
