// ----------------------------------------------------------------------
//  filexfer - client for triggering nanoHUB file interactions
//
//  This Java applet sits in the Web browser containing the Rappture
//  VNC session.  When the user triggers a file I/O operation in
//  Rappture, Rappture sends a signal to this applet, which pops up
//  a Web page to complete the file I/O transaction.
// ======================================================================
//  AUTHOR:  Michael McLennan, Purdue University
//  Copyright (c) 2004-2005  Purdue Research Foundation
//
//  See the file "license.terms" for information on usage and
//  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
// ======================================================================

import java.net.*;
import java.io.*;
import java.awt.*;

public class monitor extends Thread {
    private filexfer parent;
    private String hostName;
    private int hostPort;
    private String user;
    private String ipAddr;
    private String cookie;

    private Socket socket = null;
    public BufferedReader istream;
    public PrintStream ostream;

    private boolean run = true;

    public monitor(filexfer parent, String hostName, int hostPort,
          String user, String ipAddr, String cookie) {
        this.parent = parent;
        this.hostName = hostName;
        this.hostPort = hostPort;
        this.user = user;
        this.ipAddr = ipAddr;
        this.cookie = cookie;
    }

    public void run() {
        String line = "";
        while (run) {
            if (socket == null) {
                if (!connect()) {
                    try {
                        // sleep a little, so we don't chew up CPU
                        sleep(5000);
                    }
                    catch (java.lang.InterruptedException e) { 
                        // just move on
                    }
                }
            } else {
                try {
                    line = istream.readLine();
                    if (line == null) {
                        parent.status.append("CLOSED\n");
                        socket.close();
                        socket = null;
                    } else {
                        handle(line);
                    }
                }
                catch (java.io.IOException e){
                    parent.status.append("I/O error: "+e.getMessage()+"\n");
                }
                catch (Exception e) {
                    parent.status.append("Internal error: "+e.getMessage()+"\n");
                }
            }
        }
    }

    public void disconnect() {
        if (socket != null) {
            try {
                ostream.println("UNREGISTER RAPPTURE");
            }
            catch (Exception e) {
                parent.status.append("UNREGISTER notice failed\n");
            }
            try {
                socket.close();
            }
            catch (Exception e) {
                parent.status.append("trouble closing socket\n");
            }
            socket = null;
        }
        run = false;
    }

    public void activate() {
        if (socket != null) {
            try {
                ostream.println("ACTIVATE RAPPTURE");
            }
            catch (Exception e) {
                parent.status.append("ACTIVATE notice failed\n");
            }
        }
    }

    public void deactivate() {
        if (socket != null) {
            try {
                ostream.println("DEACTIVATE RAPPTURE");
            }
            catch (Exception e) {
                parent.status.append("DEACTIVATE notice failed\n");
            }
        }
    }

    public boolean connect() {
        try {
            socket = new Socket(hostName, hostPort);
            istream = new BufferedReader(
                new InputStreamReader(socket.getInputStream())
            );
            ostream = new PrintStream(socket.getOutputStream());
        }
        catch (UnknownHostException e) {
            parent.status.append("Can't connect: Unknown host "+hostName+"\n");
            socket = null;
            return false;
        }
        catch (Exception e) {
            socket = null;
            return false;
        }

        try {
            ostream.println("REGISTER "+user+" "+ipAddr+" "+cookie+" RAPPTURE");
            ostream.println("ACTIVATE RAPPTURE");
            parent.status.append("Connected\n");
        }
        catch (Exception e) {
            parent.status.append("\nCan't talk to server");
        }
        return true;
    }

    public void handle(String line) {
        parent.status.append("read "+line+"\n");

        if (line.startsWith("url ")) {
            try {
                URL url = new URL("http",
                    hostName, hostPort, line.substring(4));

                try {
                    parent.getAppletContext().showDocument(url,"_blank");
                    parent.status.append("launched "+url+"\n");
                }
                catch (Exception e) {
                    parent.status.append("Bad URL "+url+"\n");
                }
            }
            catch (Exception e) {
                parent.status.append("Malformed URL "+line.substring(4)+"\n");
            }
        } else {
            parent.status.append("huh? |"+line+"|\n");
        }
    }
}
