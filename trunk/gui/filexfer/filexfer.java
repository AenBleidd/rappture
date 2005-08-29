// ----------------------------------------------------------------------
//  filexfer - client for triggering nanoHUB file interactions
//
//  This Java applet sits in the Web browser containing the Rappture
//  VNC session.  When the user triggers a file I/O operation in
//  Rappture, Rappture sends a signal to this applet, which pops up
//  a Web page to complete the file I/O transaction.
// ======================================================================
//  AUTHOR:  Michael McLennan, Purdue University
//  Copyright (c) 2004-2005
//  Purdue Research Foundation, West Lafayette, IN
// ======================================================================

import java.net.*;
import java.io.*;
import java.applet.Applet;
import java.awt.*;

public class filexfer extends java.applet.Applet {
    public BufferedReader istream;
    public PrintStream ostream;
    public TextArea status;
    public int port;

    private Socket socket;
    private monitor mon;

    private String user;
    private String cookie;

    public void init() {
        String portstr = getParameter("port");
        if (portstr == null) {
            port = 9001;
	} else {
            port = Integer.parseInt(portstr);
        }

        user = getParameter("user");
        if (user == null)
            user = "???";

        cookie = getParameter("cookie");
        if (cookie == null)
            cookie = "<missing>";

        String mesg="OK";
        try {
            socket = new Socket(getCodeBase().getHost(), port);
            istream = new BufferedReader(
                new InputStreamReader(socket.getInputStream())
            );
            ostream = new PrintStream(socket.getOutputStream());
        }
        catch (UnknownHostException e) { mesg="Unknown host"; }
        catch (Exception e) { mesg="Can't connect to server"; }

        String ipAddr = "";
        try {
            // Get IP Address of the local machine
            InetAddress addr = InetAddress.getLocalHost();
            ipAddr = addr.toString();
            // Get hostname
            //String hostname = addr.getHostName();
        } catch (UnknownHostException e) {
            ipAddr = "???";
        }

        try {
            ostream.println("REGISTER "+user+" "+ipAddr+" "+cookie+" RAPPTURE");
        }
        catch (Exception e) { mesg="Can't talk to server"; }

        GridBagLayout gridbag = new GridBagLayout();
        setLayout(gridbag);

        GridBagConstraints cons = new GridBagConstraints();
        cons.gridx=0;
        cons.gridy=0;
        cons.anchor=GridBagConstraints.CENTER;
        cons.fill=GridBagConstraints.HORIZONTAL;
        cons.weightx=1.0;
        cons.weighty=0.0;

        status = new TextArea(8,60);
        gridbag.setConstraints(status, cons);
        add(status);
	status.setText(mesg);

        if (mesg == "OK") {
            mon = new monitor(this);
            mon.start();
        }
    }

    public void destroy() {
        try {
            ostream.println("UNREGISTER RAPPTURE");
            socket.close();
        }
        catch (Exception e) {
            status.append("\nUNREGISTER notice failed");
        }
    }

    public void start() {
        try {
            ostream.println("ACTIVATE RAPPTURE");
        }
        catch (Exception e) {
            status.append("\nACTIVATE notice failed");
        }
    }

    public void stop() {
        try {
            ostream.println("DEACTIVATE RAPPTURE");
        }
        catch (Exception e) {
            status.append("\nDEACTIVATE notice failed");
        }
    }
}
