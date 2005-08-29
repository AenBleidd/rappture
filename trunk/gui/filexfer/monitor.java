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
import java.awt.*;

public class monitor extends Thread {
    private filexfer parent;

    public monitor(filexfer parent) {
        this.parent = parent;
    }

    public void run() {
        String line = "";
        while (true) {
            try {
                line = parent.istream.readLine();
                if (line == null) {
                    parent.status.append("\nCLOSED");
                    stop();
                }
                parent.status.append("\nread "+line);

                if (line.startsWith("url ")) {
                    URL url = new URL("http",
                        parent.getCodeBase().getHost(),
                        parent.port, line.substring(4));

                    try {
                        parent.getAppletContext().showDocument(url,"_blank");
                        parent.status.append("\nlaunched "+url);
                    }
                    catch (Exception e) {
                        parent.status.append("\nBad URL "+url);
                    }
                } else {
                    parent.status.append("huh? |"+line+"|");
                }
            }
            catch (java.io.IOException e){
                parent.status.append("\nI/O error: "+e.getMessage());
            }
            catch (Exception e) {
                parent.status.append("\nInternal error: "+e.getMessage());
            }
        }
    }
}
