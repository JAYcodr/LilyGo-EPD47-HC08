# LilyGo-EPD47-HC08
LilyGo-EPD47 利用hc08蓝牙唤醒示例

<b>一.功能：</b><br/>
    随时唤醒休眠中的lilygo-epd47墨水屏, 唤醒后显示文字<br/>
  数据指标:<br/>
    1.利用有限的电池电量最大化节能，休眠时整体电流<1ma，18650电池平均能用1-3个月<br/>
    2.唤醒时间<1秒<br/>
<br/>
<b>二.代码说明:</b> <br/>
   1.epd47_blue_waker<br/>
     烧录到LilyGo-EPD47墨水屏， 实现墨水屏电池供电高效节能。<br/>
     平时休眠，无线唤醒。唤醒前电源<1ma<br/>
     
   2.epd47_blue_waker_center<br/>
     烧录到普通的ESP32开发板上，提供发送示例。 具体发送天气，记事，日期节日等信息需要自己发挥。
<br/>    
<b>三.硬件说明:</b><br/>
  1. lilygo-epd47墨水屏<br/>
  2. hc-08 BLE4.0蓝牙模块 (购买时要询问是告诉卖方要双晶振的，否则不支持一级节能模式!)<br/>
     hc-08设置成客户模式，一级节能模式,其蓝牙名称最好用官方工具修改下,防止被别的设备误连，<br/>
     建议名称：edp47_ink<br/>
     注：官方数据，一级节能模式电流约 6μA ~2.6mA （待机） /1.6mA（工作）<br/>
        相对于全速模式 8.5mA（待机）/9mA（待机） 节能效果明显<br/>
        hc-08模块每日大部分时间应处于在 6μA ~2.6mA （待机）模式,理论电流消耗极低<br/>
  3.接线<br/>
     lilygo-epd47  hc-08<br/>
       VCC         VCC<br/>
       12          TX<br/>
       13          RX<br/>
       GND         GND<br/>
  注：墨水屏进入节能休眠模式后，顶端12，13引脚处的VCC的电压输出会关闭，不能在此处取电，要从dh2.0或18650处取电<br/>
<br/>  
<b>四.电流实测:</b><br/>
  1.休眠： <1ma （客户端蓝牙发完信息要断开，否则墨水屏的蓝牙模块不能进入休眠，电流约9ma)<br/>
  2.唤醒后: 50-60ma<br/>
  蓝牙模块官方数据提到待机电流约6μA ~2.6mA来看，墨水屏待机电流约0.1ma(口头询问，官方数据未找到)，估算平均电流0.5ma<br/>
  1200ma的电池约能用 1200*0.8/24/0.4=80天,满足预期<br/>
  注：不能由USB供电，内部电路会导致休眠时也要耗电80ma, 达不到节能目的,必须由dh2.0或18650电池供电<br/>
  
