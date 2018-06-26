package com.android;

import com.android.sace.SaceManager;
import com.android.sace.SaceCommand;
import com.android.sace.SaceService;

import java.io.InputStream;
import java.io.IOException;

public class Main {
    public static void main (String args[]) {
        System.out.println("====start====");

        testCommand("ls /sdcard");
        testService("java_service", "ping www.baidu.com");
    }

    private static void wait_time (int time) {
        try {
            Thread.sleep(time *1000);
        }
        catch (Exception e) {}
    }

    private static void testCommand (String cm) {
        byte[] buf = new byte[1024];
        int ret;

        SaceManager manager = SaceManager.getInstance();

        System.out.println("=====> run command " + cm + " <=====");
        SaceCommand cmd = manager.runCommand(cm, true);

        try {
            InputStream in = cmd.getInput();
            while ((ret = in.read(buf, 0, 1024)) > 0) {
                System.out.print(new String(buf, 0, ret));
            }
        }
        catch (IOException e) {
            System.out.println("IOException");
        }

        cmd.close();
    }

    private static void testService (String name, String cmd) {
        SaceManager manager = SaceManager.getInstance();

        System.out.println("=====> run service " + cmd + " <=====");
        SaceService sve = manager.checkService(name, cmd);

        wait_time(10);
        System.out.println("pause");
        sve.pause();
        wait_time(20);
        System.out.println("restart");
        sve.restart();
        wait_time(10);
        System.out.println("stop");
        sve.stop();
    }
}
