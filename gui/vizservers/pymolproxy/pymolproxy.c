#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <getopt.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <tcl.h>

#define IO_TIMEOUT (30000)

struct dyBuffer
{
    char *data;
	int   used;
	int   allocated;
};

struct pymol_proxy
{
	int p_stdin;
	int p_stdout;
	int p_stderr;
	int c_stdin;
	int c_stdout;
	struct dyBuffer image;
	int need_update;
	int can_update;
	int immediate_update;
	int sync;
	int labels;
	int frame;
	int rock_offset;
	int cacheid;
	int invalidate_cache;
	int error;
	int status;
};

void
dyBufferInit(struct dyBuffer *buffer)
{
    buffer->data = NULL;
	buffer->used = 0;
	buffer->allocated = 0;
}

void
dyBufferFree(struct dyBuffer *buffer)
{
    if (buffer == NULL)
	    return;

	free(buffer->data);

	dyBufferInit(buffer);

	return;
}

void
dyBufferSetLength(struct dyBuffer *buffer, int length)
{
    char *newdata;

	if (buffer == NULL)
		return;

    if (length == 0)
	    dyBufferFree(buffer);
	else if (length > buffer->used) {
		newdata = realloc(buffer->data, length);

		if (newdata != NULL) {
		    buffer->data = newdata;
			buffer->used = length;
			buffer->allocated = length;
		}
	}
	else
	    buffer->used = length;
}

void
dyBufferAppend(struct dyBuffer *buffer, const char *data, int length)
{
    int offset;

    offset = buffer->used;

    dyBufferSetLength(buffer, offset + length);

    memcpy(buffer->data + offset, data, length);
}

int
bwrite(int sock, char *buffer, int size)
{
    int result;
    int total = 0;
    int left = size;

	while(1) {
		result = write(sock,buffer+total,left);

		if (result <= 0)
			break;

		total += result;
		left -= result;

		if (total == size)
			break;
	}

    return(total);
}

int
bread(int sock, char *buffer, int size)
{
    int result, total, left;

	for( total = 0, left = size; left > 0; left -= result)
	{
		result = read(sock,buffer+total,left);

		if (result > 0) {
			total += result;
			continue;
		}
		
		if ((result < 0) && (errno != EAGAIN) && (errno != EINTR))
		{ 
			fprintf(stderr,"Error reading sock(%d), %d/%s\n", sock, errno,strerror(errno));
			break;
		}

		result = 0;
	}

    return(total);
}

int 
bflush(int sock, char *buffer, int size, int bytes)
{
	int bsize;

	while(bytes)
	{
		if (bytes > size)
			bsize = size;
		else
			bsize = bytes;

		bsize = bread(sock,buffer,bsize);
	
		bytes -= bsize;	
	}
}

#undef timersub
#undef timeradd

void
timersub(struct timeval *a, struct timeval *b, struct timeval *result)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
	result->tv_usec = a->tv_usec - b->tv_usec;

    while(result->tv_usec < 0) {
		result->tv_sec -= 1;
		result->tv_usec += 1000000;
	}
}

void
timersub_ms(struct timeval *a, struct timeval *b, int *result)
{
	struct timeval tmp;

    tmp.tv_sec = a->tv_sec - b->tv_sec;
	tmp.tv_usec = a->tv_usec - b->tv_usec;

    while(tmp.tv_usec < 0) {
		tmp.tv_sec -= 1;
		tmp.tv_usec += 1000000;
	}

	*result = (tmp.tv_sec * 1000) + (tmp.tv_usec / 1000);
}


void
timeradd(struct timeval *a, struct timeval *b, struct timeval *result)
{
	result->tv_sec = a->tv_sec + b->tv_sec;
    result->tv_usec = a->tv_usec + b->tv_usec;

	while(result->tv_usec >= 1000000) {
		result->tv_sec += 1;
		result->tv_usec -= 1000000;
	}
}

void
timerset_ms(struct timeval *result, int timeout)
{
    result->tv_sec = timeout / 1000;
    result->tv_usec = (timeout % 1000) * 1000;
}

int
getline(int sock, char *buffer, int size, long timeout)
{
    int pos = 0, status, timeo;
	struct timeval now, end, tmo;
    struct pollfd ufd;

	gettimeofday(&now,NULL);
	timerset_ms(&tmo, timeout);
    timeradd(&now,&tmo,&end);

	ufd.fd = sock;
	ufd.events = POLLIN;

    size--;

	while(pos < size) {
		if (timeout > 0) {
			gettimeofday(&now,NULL);
			timersub_ms(&now,&end,&timeo);
		} 
		else
			timeo = -1;

    	status = poll(&ufd, 1, timeo);

       	if (status > 0)
			status = read(sock,&buffer[pos],1);

		if ( (status < 0) && ( (errno == EINTR) || (errno == EAGAIN) ) )
			continue; /* try again, if interrupted/blocking */

		if (status <= 0)
			break;
			
		if (buffer[pos] == '\n')
		{
			pos++;
			break;
		}

		pos++;

    }

	buffer[pos]=0;

	return(pos);
}

int
waitForString(struct pymol_proxy *pymol, char *string, char *buffer, int length)
{
    int sock;

	if ((pymol == NULL) || (buffer == NULL))
	    return(TCL_ERROR);

	if (pymol->status != TCL_OK)
		return(pymol->status);

	sock = pymol->p_stdout;

    while(1) {
		if (getline(sock,buffer,length,IO_TIMEOUT) == 0) {
		    pymol->error = 2;
			pymol->status = TCL_ERROR;
			break;
		}

		if (strncmp(buffer,string,strlen(string)) == 0) {
			//fprintf(stderr,"stdout-e> %s",buffer);
			pymol->error = 0;
			pymol->status = TCL_OK;
			break;
		}

		//fprintf(stderr,"stdout-u> %s",buffer);
    }

	return(pymol->status);
}

int
clear_error(struct pymol_proxy *pymol)
{
    if (pymol == NULL)
	    return(TCL_ERROR);

	pymol->error = 0;
	pymol->status = TCL_OK;

    return(pymol->status);
}

int
sendf(struct pymol_proxy *pymol, char *format, ...)
{
    va_list ap;
    char buffer[800];

	if (pymol == NULL)
		return(TCL_ERROR);

	if (pymol->error)
	    return(TCL_ERROR);

    va_start(ap, format);
    vsnprintf(buffer, 800, format, ap);
    va_end(ap);
    //fprintf(stderr,"stdin> %s", buffer);
    write(pymol->p_stdin, buffer, strlen(buffer));

    if (waitForString(pymol, "PyMOL>", buffer, 800)) {
        fprintf(stderr,"Timeout reading data [%s]\n",buffer);
		pymol->error = 1;
		pymol->status = TCL_ERROR;
        return(pymol->status);
    }

	return( pymol->status );
}

int
BallAndStickCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int ghost = 0, defer = 0, push = 0, arg;
	const char *model = "all";
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0)
		    push = 1;
		else if (strcmp(argv[arg],"-ghost") == 0)
		    ghost = 1;
		else if (strcmp(argv[arg],"-normal") == 0)
		    ghost = 0;
		else if (strcmp(argv[arg],"-model") == 0) {
			if (++arg < argc)
				model = argv[arg];
		}
		else
		    model = argv[arg];
	}

	pymol->invalidate_cache = 1;
   	pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

	sendf(pymol, "hide everything,%s\n",model);
	sendf(pymol, "set stick_color,white,%s\n",model);
	
	if (ghost)
		sendf(pymol, "set stick_radius,0.1,%s\n",model);
	else
		sendf(pymol, "set stick_radius,0.14,%s\n",model);

	sendf(pymol, "set sphere_scale=0.25,%s\n", model);

    if (ghost) {
        sendf(pymol, "set sphere_transparency,0.75,%s\n", model);
        sendf(pymol, "set stick_transparency,0.75,%s\n", model);
    }
	else {
        sendf(pymol, "set sphere_transparency,0,%s\n", model);
        sendf(pymol, "set stick_transparency,0,%s\n", model);
    }

	sendf(pymol, "show sticks,%s\n",model);
	sendf(pymol, "show spheres,%s\n",model);

	if (pymol->labels)
		sendf(pymol, "show labels,%s\n", model);

	return( pymol->status );
}

int
SpheresCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int defer = 0, ghost = 0, push = 0, arg;
	const char *model = "all";
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0)
		    push = 1;
		else if (strcmp(argv[arg],"-ghost") == 0)
		    ghost = 1;
		else if (strcmp(argv[arg],"-normal") == 0)
		    ghost = 0;
		else if (strcmp(argv[arg],"-model") == 0) {
			if (++arg < argc)
				model = argv[arg];
		}
		else
		    model = argv[arg];
	}

	pymol->invalidate_cache = 1;
	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;

    sendf(pymol, "hide everything, %s\n", model);
    sendf(pymol, "set sphere_scale,0.41,%s\n", model);
    //sendf(pymol, "set sphere_quality,2,%s\n", model);
    sendf(pymol, "set ambient,.2,%s\n", model);

    if (ghost)
        sendf(pymol, "set sphere_transparency,.75,%s\n", model);
	else
        sendf(pymol, "set sphere_transparency,0,%s\n", model);

    sendf(pymol, "show spheres,%s\n", model);

	if (pymol->labels)
		sendf(pymol, "show labels,%s\n", model);

	return(pymol->status);
}

int
LinesCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int ghost = 0, defer = 0, push = 0, arg;
	const char *model = "all";
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0)
		    push = 1;
		else if (strcmp(argv[arg],"-ghost") == 0)
		    ghost = 1;
		else if (strcmp(argv[arg],"-normal") == 0)
		    ghost = 0;
		else if (strcmp(argv[arg],"-model") == 0) {
			if (++arg < argc)
				model = argv[arg];
		}
		else
		    model = argv[arg];
	}

	pymol->invalidate_cache = 1;
	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;

    sendf(pymol, "hide everything,%s\n",model);

    if (ghost)
	    sendf(pymol, "set line_width,.25,%s\n",model);
	else
	    sendf(pymol, "set line_width,1,%s\n",model);

    sendf(pymol, "show lines,%s\n",model);

	if (pymol->labels)
		sendf(pymol, "show labels,%s\n",model);

	return(pymol->status);
}

int
DisableCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    const char *model = "all";
	int arg, defer = 0, push = 0;

	if (pymol == NULL)
	    return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {

	    if (strcmp(argv[arg], "-defer") == 0 )
			defer = 1;
		else if (strcmp(argv[arg], "-push") == 0 )
		    push = 1;
		else
			model = argv[arg];
	
	}

	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;

	sendf( pymol, "disable %s\n", model);

	return(pymol->status);
}

int
EnableCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    const char *model = "all";
	int arg, defer = 0, push = 0;

	if (pymol == NULL)
	    return(TCL_ERROR);

	clear_error(pymol);

	for(arg = 1; arg < argc; arg++) {
		
		if (strcmp(argv[arg],"-defer") == 0)
			defer = 1;
		else if (strcmp(argv[arg], "-push") == 0 )
		    push = 1;
		else
		    model = argv[arg];

	}

	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;

	sendf( pymol, "enable %s\n", model);

	return(pymol->status);
}

int
VMouseCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int arg, defer = 0, push = 0, varg = 1;
    int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

	if (pymol == NULL)
	    return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
		if (strcmp(argv[arg], "-defer") == 0)
		    defer = 1;
		else if (strcmp(argv[arg], "-push") == 0)
		    push = 1;
		else if (varg == 1) {
			arg1 = atoi(argv[arg]);
			varg++;
		}
		else if (varg == 2) {
			arg2 = atoi(argv[arg]);
			varg++;
		}
		else if (varg == 3) {
			arg3 = atoi(argv[arg]);
			varg++;
		}
		else if (varg == 4) {
			arg4 = atoi(argv[arg]);
			varg++;
		}
		else if (varg == 5) {
			arg5 = atoi(argv[arg]);
			varg++;
		}
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;

    sendf(pymol, "vmouse %d,%d,%d,%d,%d\n", arg1,arg2,arg3,arg4,arg5);

	return(pymol->status);
}

int
RawCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    struct dyBuffer buffer;
    int arg, defer = 0, push = 0;
    
    if (pymol == NULL)
	    return(TCL_ERROR);

	clear_error(pymol);

    dyBufferInit(&buffer);

	for(arg = 1; arg < argc; arg++) {
	    if (strcmp(argv[arg], "-defer") == 0)
		    defer = 1;
		else if (strcmp(argv[arg], "-push") == 0)
		    push = 1;
		else {
		    dyBufferAppend(&buffer,argv[arg],(int)strlen(argv[arg]));
		    dyBufferAppend(&buffer," ",1);
	    }
	}

	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;

    sendf(pymol,"%s\n",buffer);

	dyBufferFree(&buffer);

	return(pymol->status);
}

int
LabelCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int state = 1;
	int arg, push = 0, defer = 0;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

	for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
			defer = 1;
		else if (strcmp(argv[arg],"-push") == 0 )
			push = 1;
		else if (strcmp(argv[arg],"on") == 0 )
			state = 1;
		else if (strcmp(argv[arg],"off") == 0 )
			state = 0;
		else if (strcmp(argv[arg],"toggle") == 0 )
			state =  !pymol->labels;
	}

   	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;
fprintf(stderr,"LabelCmd: state = %d, pymolstate = %d\n",state,pymol->labels);
    if (state) {
		sendf(pymol, "set label_color,white,all\n");
		sendf(pymol, "set label_size,14,all\n");
		sendf(pymol, "label all,\"%%s%%s\" %% (ID,name)\n");
    }
	else
		sendf(pymol, "label all\n");

	pymol->labels = state;

    return(pymol->status);
}

int
FrameCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int frame = 0;
	int arg, push = 0, defer = 0;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

	for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0 )
		    push = 1;
		else
		    frame = atoi(argv[arg]);
	}
		
	pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    pymol->frame = frame;

	sendf(pymol,"frame %d\n", frame);
	
	return(pymol->status);
}

int
ResetCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int arg, push = 0, defer = 0;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

	for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0 )
		    push = 1;
	}
		
	pymol->need_update = !defer || push;
	pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;
	
	sendf(pymol, "reset\n");
	sendf(pymol, "zoom buffer=2\n");

	return(pymol->status);
}

int
RockCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
	float y = 0.0;
    int arg, push = 0, defer = 0;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

	for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0 )
		    push = 1;
		else
		    y = atof( argv[arg] );
	}
		
	pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

	sendf(pymol,"turn y, %f\n", y - pymol->rock_offset);

	pymol->rock_offset = y;

	return(pymol->status);
}

int
ViewportCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int width = 640, height = 480;
    int defer = 0, push = 0, arg, varg = 1;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

	for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 ) 
			defer = 1;
		else if ( strcmp(argv[arg], "-push") == 0 )
		    push = 1;
		else if (varg == 1) {
			width = atoi(argv[arg]);
			height = width;
			varg++;
		}
		else if (varg == 2) {
			height = atoi(argv[arg]);
			varg++;
		}
	}

    pymol->need_update = !defer || push;
	pymol->immediate_update |= push;
	pymol->invalidate_cache = 1;

    sendf(pymol, "viewport %d,%d\n", width, height);

	//usleep(205000); // .2s delay for pymol to update its geometry *HACK ALERT*
	
	return(pymol->status);
}

int
LoadPDBStrCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    const char *pdbdata, *pdb, *name;
	char *buf;
	char buffer[800];
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int state = 1;
    int tmpf;
	int arg, defer = 0, push = 0, varg = 1;
	char filename[] = "/tmp/fileXXXXXX.pdb";

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
		if ( strcmp(argv[arg],"-defer") == 0 )
			defer = 1;
		else if (strcmp(argv[arg],"-push") == 0)
		    push = 1;
        else if (varg == 1) {
			pdbdata = argv[arg];
			varg++;
		}
		else if (varg == 2) {
			name = argv[arg];
			varg++;
		}
		else if (varg == 3) {
			state = atoi( argv[arg] );
			varg++;
		}
	}

	pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    tmpf = open(filename,O_RDWR|O_TRUNC|O_CREAT,0700);
	
	if (tmpf <= 0) 
	    fprintf(stderr,"error opening file %d\n",errno);

    write(tmpf,pdbdata,strlen(pdbdata));
	close(tmpf);
	
    sendf(pymol, "load %s, %s, %d\n", filename, name, state);
	sendf(pymol, "zoom buffer=2\n");

	return(pymol->status);
}

int
RotateCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	double turnx = 0.0;
	double turny = 0.0;
	double turnz = 0.0;
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int defer = 0, push = 0, arg, varg = 1;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++)
	{
	    if (strcmp(argv[arg],"-defer") == 0)
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0)
		    push = 1;
        else  if (varg == 1) {
			turnx = atof(argv[arg]);
			varg++;
		}
		else if (varg == 2) {
		    turny = atof(argv[arg]);
			varg++;
		}
        else if (varg == 3) {
			turnz = atof(argv[arg]);
			varg++;
		}
    } 
 
    pymol->need_update = !defer || push;
	pymol->immediate_update  |= push;
	pymol->invalidate_cache = 1;

	if (turnx != 0.0)
		sendf(pymol,"turn x, %f\n", turnx);
	
	if (turny != 0.0)
		sendf(pymol,"turn y, %f\n", turny);
	
	if (turnz != 0.0) 
		sendf(pymol,"turn z, %f\n", turnz);

	return(pymol->status);
}

int
ZoomCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    double factor = 0.0;
    double zmove = 0.0;
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;
    int defer = 0, push = 0, arg, varg = 1;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    for(arg = 1; arg < argc; arg++)
	{
	    if (strcmp(argv[arg],"-defer") == 0)
		    defer = 1;
		else if (strcmp(argv[arg],"-push") == 0)
		    push = 1;
		else if (varg == 1) {
	        factor = atof(argv[arg]);
			varg++;
		}
    }

	zmove = factor * -75;
 
    pymol->need_update = !defer || push;
	pymol->immediate_update  |= push;
	pymol->invalidate_cache = 1;

	if (zmove != 0.0)
		sendf(pymol,"move z, %f\n", factor);

	return(pymol->status);
}

int
PNGCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	char buffer[800];
	int bytes=0;
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;

	if (pymol == NULL) 
		return(TCL_ERROR);

	clear_error(pymol);

    if (pymol->invalidate_cache)
	    pymol->cacheid++;

	pymol->need_update = 0;
	pymol->immediate_update = 0;
	pymol->invalidate_cache = 0;

	sendf(pymol,"png -\n");

	waitForString(pymol, "image follows: ", buffer, 800);

	sscanf(buffer, "image follows: %d\n", &bytes);

    dyBufferSetLength(&pymol->image, bytes);

	bread(pymol->p_stdout, pymol->image.data, pymol->image.used);

	waitForString(pymol, " ScenePNG", buffer,800);

    if (bytes && (pymol->image.used == bytes)) {
	    sprintf(buffer, "nv>image %d %d,%d,%d\n", 
	        bytes, pymol->cacheid, pymol->frame, pymol->rock_offset);

	    write(pymol->c_stdin, buffer, strlen(buffer));
	    bwrite(pymol->c_stdin, pymol->image.data, bytes);
    }

	return(pymol->status);
}

int
BMPCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
	char buffer[800];
	int bytes=0;
	struct pymol_proxy *pymol = (struct pymol_proxy *) cdata;

	if (pymol == NULL)
		return(TCL_ERROR);

	clear_error(pymol);

    if (pymol->invalidate_cache)
	    pymol->cacheid++;

	pymol->need_update = 0;
	pymol->immediate_update = 0;
	pymol->invalidate_cache = 0;

	sendf(pymol,"bmp -\n");

	waitForString(pymol, "image follows: ", buffer, 800);

	sscanf(buffer, "image follows: %d\n", &bytes);

    dyBufferSetLength(&pymol->image, bytes);

	bread(pymol->p_stdout, pymol->image.data, pymol->image.used);

    if (bytes && (pymol->image.used == bytes)) {
	    sprintf(buffer, "nv>image %d %d,%d,%d\n", 
		    bytes, pymol->cacheid, pymol->frame, pymol->rock_offset);
	    write(pymol->c_stdin, buffer, strlen(buffer));
		//fprintf(stderr,buffer);
	    bwrite(pymol->c_stdin, pymol->image.data, bytes);
    }

	return(pymol->status);
}
	
int pyMol_Proxy(int c_in, int c_out, char *command, char *argv[])
{
    int flags;
	int status;
	int eof;
	int pairIn[2];
	int pairOut[2];
	int pairErr[2];
	char buffer[800];
	Tcl_Interp *interp;
	Tcl_DString cmdbuffer;
	struct dyBuffer dybuffer, dybuffer2;
	struct pollfd ufd[3];
	int pid;
	struct pymol_proxy pymol;
	struct timeval now,end;
	int timeout;

	/* Create three pipes for communication with the external application	*/
	/* One each for the applications's: stdin, stdout, and stderr			*/

	if (pipe(pairIn) == -1)
		return(-1);

	if (pipe(pairOut) == -1) {
		close(pairIn[0]);
		close(pairIn[1]);
		return(-1);
	}

	if (pipe(pairErr) == -1) {
		close(pairIn[0]);
		close(pairIn[1]);
		close(pairOut[0]);
		close(pairOut[1]);
		return(-1);
	}

	/* Fork the new process.  Connect i/o to the new socket.				*/

	pid = fork();
        
	if (pid < 0) 
		return(-3);

	if (pid == 0)  /* child process */
	{
		int i, fd;

		/* create a new process group, so we can later kill this process and 
		 * all its children without affecting the process that created this one
		 */

		setpgid(pid, 0); 

		/* Redirect stdin, stdout, and stderr to pipes before execing 		*/

		dup2(pairIn[0] ,0);  // stdin
		dup2(pairOut[1],1);  // stdout
		dup2(pairErr[1],2);  // stderr

		for(fd = 3; fd < FD_SETSIZE; fd++) 	/* close all other descriptors	*/
			close(fd);

		execvp(command,argv);
		fprintf(stderr,"Failed to start pyMol\n");
		exit(-1);
	}
       
	/* close opposite end of pipe, these now belong to the child process	*/
    close(pairIn[0]);
    close(pairOut[1]);
    close(pairErr[1]);

	pymol.p_stdin = pairIn[1];
	pymol.p_stdout = pairOut[0];
	pymol.p_stderr = pairErr[0];
	pymol.c_stdin  = c_in;
	pymol.c_stdout = c_out;
	pymol.labels = 0;
	pymol.need_update = 0;
	pymol.can_update = 1;
	pymol.immediate_update = 0;
	pymol.sync = 0;
    pymol.frame = 1;
	pymol.rock_offset = 0;
	pymol.cacheid = 0;
	pymol.invalidate_cache = 0;

	ufd[0].fd = pymol.c_stdout;
	ufd[0].events = POLLIN | POLLHUP; /* ensure catching EOF */
	ufd[1].fd = pymol.p_stdout;
	ufd[1].events = POLLIN | POLLHUP;
	ufd[2].fd = pymol.p_stderr;
	ufd[2].events = POLLIN | POLLHUP;

    flags = fcntl(pymol.p_stdout, F_GETFL);
    fcntl(pymol.p_stdout, F_SETFL, flags|O_NONBLOCK);

	interp = Tcl_CreateInterp();
	Tcl_MakeSafe(interp);

	Tcl_DStringInit(&cmdbuffer);
	dyBufferInit(&pymol.image);
    dyBufferInit(&dybuffer);
    dyBufferInit(&dybuffer2);

	Tcl_CreateCommand(interp,"bmp",BMPCmd,(ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"png",PNGCmd,(ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"screen",ViewportCmd,(ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"viewport",ViewportCmd,(ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"rotate",RotateCmd,(ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"zoom",ZoomCmd,(ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"loadpdb", LoadPDBStrCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(interp,"ball_and_stick", BallAndStickCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"spheres", SpheresCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"lines", LinesCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"raw", RawCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"label", LabelCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"reset", ResetCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"rock", RockCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"frame", FrameCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"vmouse", VMouseCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"disable", DisableCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);
	Tcl_CreateCommand(interp,"enable", EnableCmd, (ClientData)&pymol, (Tcl_CmdDeleteProc*)NULL);

	// Main Proxy Loop
	//	accept tcl commands from socket
	//  translate them into pyMol commands, and issue them to child proccess
	//  send images back

	if (1)
		fprintf(stderr,"Pymol Ready.\n");

	gettimeofday(&end, NULL);

	while(1) 
	{
		char ch;

	    gettimeofday(&now,NULL);

        if ( (!pymol.need_update) )
            timeout = -1;
        else if ((now.tv_sec > end.tv_sec) || ( (now.tv_sec == end.tv_sec) && (now.tv_usec >= end.tv_usec)) )
            timeout = 0; 
        else
        {
            timeout = (end.tv_sec - now.tv_sec) * 1000;

            if (end.tv_usec > now.tv_usec)
                timeout += (end.tv_usec - now.tv_usec) / 1000;
            else
                timeout += (((1000000 + end.tv_usec) - now.tv_usec) / 1000) - 1000;

        }

		if (!pymol.immediate_update)
			status = poll(ufd, 3, timeout);
	    else
		    status = 0;

		if ( status < 0 )
		{
			fprintf(stderr, "POLL ERROR: status = %d, errno = %d, %s \n", status,errno,strerror(errno));
		}
		else if (status > 0)
		{
			gettimeofday(&now,NULL);

            if (ufd[0].revents) {

			    if (read(ufd[0].fd,&ch,1) <= 0) 
			    {
				    fprintf(stderr,"EOF or Lost Connection. status = %d, errno = %d, %s \n", status,errno,strerror(errno));
				    break;
			    }
			    else
			    {
				    Tcl_DStringAppend(&cmdbuffer, &ch, 1);

				    if (ch == '\n' && Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer))) {
			            Tcl_Eval(interp, Tcl_DStringValue(&cmdbuffer));
					    fprintf(stderr,"Executed(%d,%d): %s", pymol.need_update, pymol.immediate_update, Tcl_DStringValue(&cmdbuffer));
			            Tcl_DStringSetLength(&cmdbuffer, 0);

					    if (timeout == 0) status = 0; // send update
		            }
			    }
			}

			if (ufd[1].revents ) {
			    if (read(ufd[1].fd, &ch, 1) <= 0)
				{
				    if (errno == EAGAIN)
					    fprintf(stderr,"EAGAIN error reading pymol stdout... retrying.\n");
                    else {
				        fprintf(stderr,"Lost PyMol Session. (errno=%d)\n",errno);
						break;
					}
				}
				else
				{
				    dyBufferAppend(&dybuffer, &ch, 1);

					if (ch == '\n') {
					    ch = 0;
					    dyBufferAppend(&dybuffer, &ch, 1);
						fprintf(stderr,"STDOUT>%s",dybuffer.data);

						if (dybuffer.data[0]=='I' && dybuffer.data[1] == 0)
						    pymol.need_update = 1;

						dyBufferSetLength(&dybuffer,0);
					}
				}
			}

			if (ufd[2].revents) {
				if (read(ufd[2].fd, &ch, 1) <= 0)
				{
				    if (errno == EAGAIN) 
					    fprintf(stderr,"EAGAIN error reading pymol stderr... retrying.\n");
	                else {
					    fprintf(stderr,"Lost PyMol Stderr Session. (errno=%d)\n", errno);
						break;
					}
				}
				else {
					dyBufferAppend(&dybuffer2, &ch, 1);

					if (ch == '\n') {
					    ch = 0;
						dyBufferAppend(&dybuffer2, &ch, 1);
						fprintf(stderr,"stderr>%s", dybuffer2.data);

						dyBufferSetLength(&dybuffer2,0);
					}
				}
			}
		}

        if (status == 0) 
		{
			gettimeofday(&now,NULL);

			if (pymol.need_update && pymol.can_update)
				Tcl_Eval(interp, "bmp -\n");

			end.tv_sec = now.tv_sec;
			end.tv_usec = now.tv_usec + 150000;

            if (end.tv_usec >= 1000000)
			{
				end.tv_sec++;
				end.tv_usec -= 1000000;
			}

		}

    }

	fprintf(stderr,"Waiting for process to end\n");

	status = waitpid(pid, NULL, WNOHANG);

	if (status == -1) 
		fprintf(stderr, "Error waiting on process (%d)\n", errno);
	else if (status == 0) {
		fprintf(stderr, "Attempting to SIGTERM process.\n");
		kill(-pid, SIGTERM); // kill process group
		alarm(5);
		status = waitpid(pid, NULL, 0);
		alarm(0);

		while ((status == -1) && (errno == EINTR))
		{
			fprintf(stderr, "Attempting to SIGKILL process.\n");
			kill(-pid, SIGKILL); // kill process group
			alarm(10);
			status = waitpid(pid, NULL, 0);
			alarm(0); 
		}
	}

	fprintf(stderr, "Process ended\n");

	dyBufferFree(&pymol.image);

	Tcl_DeleteInterp(interp);

	return( (status == pid) ? 0 : 1);
}

#ifdef STANDALONE

#define MAX_ARGS 100

int
main(int argc, char *argv[])
{
    int arg;
    char *myargv[MAX_ARGS+1];

	if (argc > MAX_ARGS+1) {
        fprintf(stderr, "%s: Argument list too long (%d > %d arguments).", argv[0], argc, MAX_ARGS + 1);
		return(1);
	}

    for(arg = 1; arg < argc; arg++)
	    myargv[arg-1] = argv[arg];

	myargv[arg-1] = NULL;

    pyMol_Proxy(fileno(stdin), fileno(stdout), myargv[0], myargv);
}

#endif

