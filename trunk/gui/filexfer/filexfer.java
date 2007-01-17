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
import java.applet.Applet;
import java.awt.*;

public class filexfer extends java.applet.Applet {
    public BufferedReader istream;
    public PrintStream ostream;
    public TextArea status;

    private monitor mon = null;

    public void init() {
        int port = 0;
        String portstr = getParameter("port");
        if (portstr == null) {
            port = 9001;
	} else {
            port = Integer.parseInt(portstr);
        }

        String user = "";
        user = getParameter("user");
        if (user == null)
            user = "???";

        String cookie = "";
        cookie = getParameter("cookie");
        if (cookie == null)
            cookie = "<missing>";

        String connect_param = null;
        connect_param = getParameter("connect");

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

        mon = new monitor(this, getCodeBase().getHost(), port,
            user, ipAddr, cookie, connect_param);
        mon.start();
    }

    public void destroy() {
        mon.disconnect();
    }

    public void start() {
        mon.activate();
    }

    public void stop() {
        mon.deactivate();
    }
}
