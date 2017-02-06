package com.newer.ch4.implicit;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.telephony.SmsMessage;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Created by xjx on 2016/6/1.
 */
public class ReceiverDemo extends BroadcastReceiver
{
    private static final String strRes = "android.provider.Telephony.SMS_RECEIVED";
    private TextView textView;

    public ReceiverDemo(TextView textView){
        this.textView = textView;
    }



    @Override
    public void onReceive(Context context, Intent intent)
    {
        StringBuilder sb = new StringBuilder();
        Bundle bundle = intent.getExtras();

        if(bundle != null)
        {
            Object[] pdus = (Object[])bundle.get("pdus");
            SmsMessage[] msg = new SmsMessage[pdus.length];
            for(int i = 0 ;i<pdus.length;i++){
                msg[i] = SmsMessage.createFromPdu((byte[])pdus[i]);
            }

            for(SmsMessage curMsg:msg){
                sb.append("You got the message From:【");
                sb.append(curMsg.getDisplayOriginatingAddress());  // 手机号码
                sb.append("】Content：");
                sb.append(curMsg.getDisplayMessageBody()); // 短信内容
            }
            //Toast.makeText(context, "Got The Message:" + sb.toString(), Toast.LENGTH_SHORT).show();

            textView.setText(sb.toString());  // 显示在手机屏幕上
           // Bundle b = new Bundle();
            //b.putString("key",sb.toString());
            //Intent i = new Intent(context,  MainActivity.class);
            //i.putExtra("key",sb.toString());
            //context.startActivity(i);

        }
    }
}
