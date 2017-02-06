package com.newer.ch4.implicit;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Picture;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;


public class MainActivity extends Activity {

    private static final int REQUEST_PICK_PIC = 1;
    private static final String TAG = "MainActivity";
    private int MSG_CODE = 100;
    private ImageView imageView;
    private TextView textView;
    private Handler handler = new Handler() {  // 利用Handler来实现UI线程的更新的。
        // Handler来根据接收的消息，处理UI更新。Thread线程发出Handler消息，通知更新UI。
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_CODE) {  // 判断是否接收到数据
//                Bundle bundle = msg.getData();  // 读数据
//                textView.setText(bundle.getInt("msg"));


               int width =  MainActivity.this.getWindowManager().getDefaultDisplay().getWidth();
                RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,(int)(width*0.8));

                layout.addRule(RelativeLayout.BELOW,R.id.xin);
                imageView.setLayoutParams(layout);
                imageView.setScaleType(ImageView.ScaleType.FIT_XY);

                Bitmap bmp = (Bitmap) msg.obj;
                imageView.setImageBitmap(bmp);

                imageView.invalidate();



//                SurfaceView
//                imageView.setImageResource(R.drawable.hehe);

                // 打印(显示)数据
            }
        }
    };
    /**
     * ATTENTION: This was auto-generated to implement the App Indexing API.
     * See https://g.co/AppIndexing/AndroidStudio for more information.
     */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        textView = (TextView) findViewById(R.id.xin);
        imageView = (ImageView) findViewById(R.id.imageView);

        ReceiverDemo receiver = new ReceiverDemo(textView);
        IntentFilter filter = new IntentFilter();
        filter.addAction("android.provider.Telephony.SMS_RECEIVED");
        this.registerReceiver(receiver, filter);
        new Thread() {
            @Override
            public void run() {
                try {
                    ServerSocket serverSocket = new ServerSocket(8087);
                    while (true) {
                        Socket socket = serverSocket.accept();
                        // 接收客户端 阻塞式等待

                        new Thread(new ArmServerThread(socket)).start();
                        // 如果接收到了数据 就创建一个线程
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }.start();
    }

    private class ArmServerThread implements Runnable {
        private Socket socket;

        // 把客户端 socket用构造函数传进来
        public ArmServerThread(Socket socket) {
            this.socket = socket;  // 用自己创建的socket保存传进来的socket
        }

        @Override
        public void run() {
            Message msg = new Message(); // 消息
            msg.what = MSG_CODE; // 标记是哪个activity发过来的消息
            Bundle bundle = new Bundle(); // 用于不同Activity之间的数据传递
            bundle.clear(); // 清除

            try {
                BufferedInputStream in = new BufferedInputStream(socket.getInputStream()); // socket输入流



                ByteArrayOutputStream bos = new ByteArrayOutputStream();
                int t = in.read();
                while (t != -1) {
                    bos.write(t);
                    t = in.read();
                }
                byte[] bs = bos.toByteArray();
                Log.v(TAG, "" + bs.length);


//                File f = Environment.getExternalStoragePublicDirectory("Pic");
//                if(!f.exists()){
//                    f.mkdir();
//                }
//
//                BufferedOutputStream bfile = new BufferedOutputStream(new FileOutputStream(f.getAbsolutePath()+"/aa.jpg"));
//                bfile.write(bs);
//                bfile.flush();
//                bfile.close();

                //bundle.getInt("msg", bs.length);   // 键值对
                // Toast.makeText(MainActivity.this, ""+bs.length, Toast.LENGTH_LONG).show();
//              msg.setData(bundle);   // 把bundle对应的内容放到msg里
//              handler.sendMessage(msg); //andler 句柄
                Bitmap bmp = BitmapFactory.decodeByteArray(bs, 0, bs.length);




                // bundle.putByteArray("jp",bs);
                msg.obj = bmp;
                msg.what = MSG_CODE;
                handler.sendMessage(msg);
//
//                String line = null;
//                String buff = null;
//                line = in.readLine(); // 读一行

//                bundle.putString("msg",line); // 键值对
//                msg.setData(bundle);   // 把bundle对应的内容放到msg里
//                handler.sendMessage(msg); //andler 句柄
//
//                while ((line = in.readLine())!=null){
//                    buff += line;   // 一行一行的读  读到末尾
//                }
//
//                bundle.putString("msg",buff);  // 键值对
//                msg.setData(bundle); // 把bundle对应的内容放到msg里
//                handler.sendMessage(msg); //  这个方法是指 Handler
                // 在发送消息的时候，需要发送一个新的对象。也就是每次在发送消息时，
                // 在更新了 Date 后，需要重新构造 Message 对象，
                // 而不是利用已经构建的对象(激活handler)
                in.close();  // 关闭输入流
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


    public void do0(View v) {

        PendingIntent pi = PendingIntent.getActivity(this, 0, new Intent(), 0);

        SmsManager sms = SmsManager.getDefault();
         sms.sendTextMessage("15526480462", null, "see", pi, null);
//        sms.sendTextMessage("18229876570", null, "see", pi, null);
//         sms.sendTextMessage("18229876570", null, "see", pi, null);

//        sms.sendTextMessage("10086", null, "101", pi, null);

    }


    public void do1(View v) {
        PendingIntent pi = PendingIntent.getActivity(this, 0, new Intent(), 0);

        SmsManager sms = SmsManager.getDefault();
            sms.sendTextMessage("15526480462", null, "look", pi, null);
//        sms.sendTextMessage("18229876570", null, "look", pi, null);
        //    sms.sendTextMessage("18229876570", null, "look", pi, null);


//        sms.sendTextMessage("10086", null, "101", pi, null);

    }


    public void do2(View v) {

        PendingIntent pi = PendingIntent.getActivity(this, 0, new Intent(), 0);

        SmsManager sms = SmsManager.getDefault();

        //sms.sendTextMessage("10010", null, "102", pi, null);
         sms.sendTextMessage("15526480462", null, "close", pi, null);
//        sms.sendTextMessage("18229876570", null, "close", pi, null);
//        sms.sendTextMessage("18229876570", null, "close", pi, null);
    }


    public void do3(View v) {
        PendingIntent pi = PendingIntent.getActivity(this, 0, new Intent(), 0);

        SmsManager sms = SmsManager.getDefault();

        //sms.sendTextMessage("10010", null, "102", pi, null);
          sms.sendTextMessage("15526480462", null, "open", pi, null);
//        sms.sendTextMessage("18229876570", null, "open", pi, null);
//        sms.sendTextMessage("18229876570", null, "open", pi, null);
    }

}


