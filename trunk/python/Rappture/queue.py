# ----------------------------------------------------------------------
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005  Purdue Research Foundation, West Lafayette, IN
# ======================================================================

import sys,os
import re
import time
import shutil

# import Rappture Related libs
from tools import *

#######################################################################

# http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/52235
class CallbaseError(AttributeError):
    pass

def callbase(obj, base, methodname='__init__', args=(), raiseIfMissing=None):
    try: method = getattr(base, methodname)
    except AttributeError:
        if raiseIfMissing:
            raise CallbaseError, methodname
        return None
    if args is None: args = ()
    return method(obj, *args)

class RpQueueError(Exception):
    pass

class queue:

    def __init__(self):

        # initialize the general queue variable with safe, default values
        self._queue_vars = {}
        self.__initQueueVars__()

    def __initQueueVars__(self):
        self._queue_vars['walltime'] = 1
        self._queue_vars['resultsDir'] = ""
        self._queue_vars['executable'] = ""
        self._queue_vars['execArgs'] = ""
        self._queue_vars['nodes'] = 1
        self._queue_vars['ppn'] = 1
        self._queue_vars['queue'] = ""
        self._queue_vars['outFile'] = ""
        self._queue_vars['errFile'] = ""
        self._queue_vars['logFile'] = ""
        self._queue_vars['jobName'] = ""

    def walltime(self,walltime=''):
        walltime = str(walltime)
        if (walltime == '') or (walltime == 'None'):
            return self._queue_vars['walltime']
        elif re.match(r'^\d{2}:\d{2}:\d{2}',walltime):
            self._queue_vars['walltime'] = walltime
        else:
            error = "Incorrect walltime format: should be 00:00:00"
            raise RpQueueError,error

#        walltime = str(walltime)
#        if re.match(r'^\d{2}:\d{2}:\d{2}',walltime):
#        else:
#            try:
#                walltime = int(walltime)
#            except:
#                error = "walltime must match format"
#        if (walltime <= 0):
#            return self._queue_vars['walltime']
#        else:
#            self._queue_vars['walltime'] = walltime

    def resultsDir(self,resultsDir=''):
        resultsDir = str(resultsDir)
        if (resultsDir == 'None') or (resultsDir == ''):
            return self._queue_vars['resultsDir']
        else:
            self._queue_vars['resultsDir'] = resultsDir

    def executable(self,executable=''):
        executable = str(executable)
        if (executable == 'None') or (executable == ''):
            return self._queue_vars['executable']
        else:
            self._queue_vars['executable'] = executable

    def execArgs(self,execArgs=''):
        execArgs = str(execArgs)
        if (execArgs == 'None') or (execArgs == ''):
            return self._queue_vars['execArgs']
        else:
            self._queue_vars['execArgs'] = execArgs

    def nodes(self,nodes=''):
        if nodes == '':
            return self._queue_vars['nodes']
        nodes = int(nodes)
        if nodes > 0:
            self._queue_vars['nodes'] = nodes
        else:
            error = 'nodes must be a positive integer'
            raise RpQueueError, error

    def ppn(self,ppn=''):
        if ppn == '':
            return self._queue_vars['ppn']
        ppn = int(ppn)
        if ppn > 0:
            self._queue_vars['ppn'] = ppn
        else:
            error = 'ppn must be a positive integer'
            raise RpQueueError, error

    def queue(self,queue=''):
        queue = str(queue)
        if (queue == 'None') or (queue == ''):
            return self._queue_vars['queue']
        else :
            self._queue_vars['queue'] = queue

    def outFile(self,outFile=''):
        outFile = str(outFile)
        if (outFile == 'None') or (outFile == ''):
            return self._queue_vars['outFile']
        else:
            self._queue_vars['outFile'] = outFile

    def errFile(self,errFile=''):
        errFile = str(errFile)
        if (errFile == 'None') or (errFile == ''):
            return self._queue_vars['errFile']
        else:
            self._queue_vars['errFile'] = errFile

    def logFile(self,logFile=''):
        logFile = str(logFile)
        if (logFile == 'None') or (logFile == ''):
            return self._queue_vars['logFile']
        else:
            self._queue_vars['logFile'] = logFile

    def jobName(self,jobName=''):
        jobName = str(jobName)
        if jobName == 'None' or (jobName == ''):
            return self._queue_vars['jobName']
        else: 
            self._queue_vars['jobName'] = jobName

    def __convertWalltime__(self):
        minutesu

    def submit(self):
        pass

    def status(self):
        pass

class pbs(queue):
    """the pbs class organizes the information needed to submit jobs to 
nanohub's pbs queues through Rappture"""

    def __init__(   self,
                    jobName,
                    resultsDir,
                    nodes,
                    executable,
                    execArgs='',
                    walltime='00:01:00' ):

        # call the base class's init
        for base in self.__class__.__bases__:
            callbase(self, base)

        self._pbs_msgs = {}
        self.__fillStatusDict__()

        # initialize pbs script vars
        self._pbs_vars = {}
        self.__initPbsScriptVars__()

        # initialize jobId for this object
        self._jobId = ""

        # set vars based on user input
        self.resultsDir(resultsDir)
        self.jobName(jobName)
        self.nodes(nodes)
        self.walltime(walltime)
        self.executable(executable)
        self.execArgs(execArgs)
        self.queue("@vma118.punch.purdue.edu")

    def __initPbsScriptVars__(self):
        self._pbs_vars['cmd'] = ""

    def __fillStatusDict__(self):
        # Possible PBS Job States 
        # From qstat manpage on hamlet.rcac.purdue.edu
        #   E -  Job is exiting after having run.
        #   H -  Job is held.
        #   Q -  job is queued, eligible to run or routed.
        #   R -  job is running.
        #   T -  job is being moved to new location.
        #   W -  job is waiting for its execution time
        #         (-a option) to be reached.
        #   S -  job is suspended.

        self._pbs_msgs['E'] = 'Job is exiting after having run'
        self._pbs_msgs['H'] = 'Job is held'
        self._pbs_msgs['Q'] = 'Simulation Queued'
        self._pbs_msgs['R'] = 'Running Simulation'
        self._pbs_msgs['T'] = 'Job is being moved to new location. Please Wait...'
        self._pbs_msgs['W'] = 'Job is waiting for its execution time. Please Wait...'
        self._pbs_msgs['S'] = 'Job is suspended'
        self._pbs_msgs['DEFAULT'] = 'Returning Results...'

    def __makePbsScript__(self):
        # create the directory where output should be placed
        self._resultDirPath = os.path.join(os.getcwd(),self.resultsDir())

        # if os.path.exists(self._resultDirPath):
        #     shutil.rmtree(self._resultDirPath)
        # os.mkdir(self._resultDirPath)

        # set the out and err files
        if self.outFile() == '':
            self.outFile(os.path.join(self.resultsDir(),self.jobName()+".out"))

        if self.errFile() == '':
            self.errFile(os.path.join(self.resultsDir(),self.jobName()+".err"))

        # remove existing "FLAG" files if they exist
        if os.path.isfile(self.outFile()):
            os.remove(self.outFile())
        if os.path.isfile(self.errFile()):
            os.remove(self.errFile())

        if self.cmd() == '':
            # find the mpirun command
            mpiCommand = getCommandOutput('which mpirun')
            if mpiCommand == '':
                # command not found?
                error = 'mpirun command not found'
                raise RpQueueError, error
            else :
                mpiCommand = mpiCommand.strip()

            # check to make sure user specified an executable
            if self._queue_vars['executable'] == '':
                error = 'executable not set within pbs queue'
                raise RpQueueError, error

            self.cmd('%s -np %d -machinefile $PBS_NODEFILE %s %s' % \
                (mpiCommand, self.nodes(), self.executable(), self.execArgs()))

        return """#PBS -S /bin/bash
#PBS -l nodes=%s:ppn=%s
#PBS -l walltime=%s
#PBS -q %s
#PBS -o %s
#PBS -e %s
#PBS -mn
#PBS -N %s
cd $PBS_O_WORKDIR

cmd="%s"
echo == running from $HOSTNAME:
echo $cmd
$cmd

touch %s
""" % ( self.nodes(),
        self.ppn(),
        self.walltime(),
        self.queue(),
        self.outFile(),
        self.errFile(),
        self.jobName(),
        self.cmd(),
        self.errFile()   )

    def __checkErrFiles__(self,chkFileName):
        retVal = False
        if not os.path.isfile(chkFileName):
            retVal = True
        return retVal

    def cmd(self,cmd=''):
        cmd = str(cmd)
        if (cmd == 'None') or (cmd == ''):
            return self._pbs_vars['cmd']
        else :
            self._pbs_vars['cmd'] = cmd

    def getCurrentStatus(self):
        retVal = self._pbs_msgs['DEFAULT']
        if self._jobId:
            cmdOutput = getCommandOutput("qstat -a | grep \'^ *%s\'" % (self._jobId))
            # parse a string that should look like this: 
            # 32049.hamlet.rc kearneyd ncn      run.21557.   6104   8   8    --  24:00 R 00:00
            parseResults = cmdOutput.split() # re.compile(r'\W+').split(cmdOutput)
            # results should look like this:
            # ['32049.hamlet.rc','kearneyd','ncn','run.21557.','6104','8','8','--','24:00','R','00:00']

            if len(parseResults) > 9:
                qstat = parseResults[9]
                retVal = self._pbs_msgs[qstat]

        return '('+self._jobId+') '+retVal

    def submit(self):
        submitFileData = self.__makePbsScript__()
        submitFileName = self._resultDirPath+'/'+self.jobName()+'.pbs'
        writeFile(submitFileName,submitFileData)

        myCWD = os.getcwd()
        os.chdir(self._resultDirPath)
        cmdOutData = getCommandOutput('qsub %s' % (submitFileName))
        os.chdir(myCWD)

        self._prevStatus = ''
        self._currStatus = ''

        # a successful submission (on hamlet/lear) follows this regexp
        if re.match(r'[0-9]+',cmdOutData):
            self._jobId = re.search(r'[0-9]+',cmdOutData).group()
        else:
            error = 'Submission to PBS Queue Failed'
            raise RpQueueError, error

    def status(self):
        min = 0
        tCount = 0
        sleepTime = 10

        while self.__checkErrFiles__(self.errFile()):
            if tCount == 60 :
                min += 1
                tCount = 0

            self._currStatus = self.getCurrentStatus()
            if (self._currStatus != self._prevStatus):
                sys.stdout.write("%s: %s\n" % ( self._currStatus, time.ctime() ) )
                sys.stdout.flush()
                self._prevStatus = self._currStatus

            time.sleep(sleepTime)
            tCount += sleepTime



class condor (queue):
    # this class is not working!

    USE_MPI = 1

    def __init__(self,walltime='00:01:00',flags=0):
        # call the base class's init
        for base in self.__class__.__bases__:
            callbase(self, base)

        self._condor_msgs = {}

        self._flags = flags
        self.__setWorkingDir__()

        # set vars based on user input
        self.resultsDir(resultsDir)
        self.jobName(jobName)
        self.nodes(nodes)
        self.walltime(walltime)
        self.executable(executable)
        self.execArgs(execArgs)
        self.queue("tg_workq@lear")
        self.outFile("out.$(cluster).$(process)")
        self.errFile("err.$(cluster).$(process)")
        self.logFile("log.$(cluster)")

    def __fillStatusDict__(self):
        self._condor_msgs['I'] = 'Simulation Queued'
        self._condor_msgs['H'] = 'Simulation Held'
        self._condor_msgs['R'] = 'Running Simulation'
        self._condor_msgs['C'] = 'Simulation Complete'
        self._condor_msgs['X'] = 'Simulation Marked For Deletion'
        self._condor_msgs['DEFAULT'] = 'Returning Results...'

    def __setWorkingDir__(self):
        session = os.getenv("SESSION","00000X")
        epoch = int(time.mktime(datetime.datetime.utcnow().timetuple()))
        self._workingdir = str(epoch) + "_" + session

    def __makeCondorTemplate__(self):
#        return """universe=grid
#    gridresource = gt2 tg-gatekeeper.purdue.teragrid.org:2120/jobmanager-pbs
#    executable = EXECUTABLE
#    output = out.$(cluster).$(process)
#    error = err.$(cluster).$(process)
#    should_transfer_files = YES
#    when_to_transfer_output = ON_EXIT
#    initialDir = MY_RESULT_DIR
#    log = log.$(cluster)
#    #globusrsl = (project=TG-ECD060000N)(jobType=mpi)(count=2)(hostcount=2)(maxWallTime=WALLTIME)
#    globusrsl = (project=TG-ECD060000N)(maxWallTime=WALLTIME)MPIRSL(queue=tg_workq@lear)
#    notification = never
#    """

        self._mpirsl = ''
        if (self.flags & self.USE_MPI):
            self._mpirsl = "(jobType=mpi)(count=%d)(hostcount=%d)" % \
                            (self.nodes(), self.nodes())

        return """universe=grid
    gridresource = gt2 tg-gatekeeper.purdue.teragrid.org:2120/jobmanager-pbs
    executable = %s
    output = %s
    error = %s
    should_transfer_files = YES
    when_to_transfer_output = ON_EXIT
    initialDir = %s
    log = %s
    #globusrsl = (project=TG-ECD060000N)(jobType=mpi)(count=2)(hostcount=2)(maxWallTime=WALLTIME)
    globusrsl = (project=TG-ECD060000N)(maxWallTime=%s)%s(queue=%s)
    notification = never
    """ % ( self.executable(),
            self.outFile(),
            self.errFile(),
            self.resultsDir(),
            self.logFile(),
            self.resultsDir(),
            self.walltime(),
            self._mpirsl,
            self.queue()    )

    def addProcess(self,argsList,inputFiles=[],):
#        return """arguments = ARGUMENT resultsID.tar.gz WORKINGDIR_ID
#    transfer_input_files = INFILE,\\
#                           APPROOT/data/circle-1.e,\\
#                           APPROOT/data/circle-1.n,\\
#                           APPROOT/data/circle-1.d,\\
#                           APPROOT/data/circle-1.s,\\
#                           APPROOT/data/circle-1_ni,\\
#                           APPROOT/data/circle-1.edge,\\
#                           APPROOT/data/circle-1.ele,\\
#                           APPROOT/data/circle-1.node
#    transfer_output_files = resultsID.tar.gz
#    Queue
#    """
        transInFiles = ",\\\\\n\t\t".join(inputFiles)
        nextProcId = len(self._processList)
        newProcess = """arguments = %s results%d.tar.gz %s_%d
    transfer_input_files = %s
    transfer_output_files = resultsID.tar.gz
    Queue

    """ % (argList,nextProcId,self._workingdir,nextProcId,transInFiles)
        self._processList.append(newProcess)


#    def chkCondorStatus(chkID):
    def getCurrentStatus(self):
        global condor_msgs
        if chkID:
            chkCmd = 'condor_q'
            cmdOutput = getCommandOutput("%s | grep \'^ *%s\'" % (chkCmd,chkID))

            # parse a string that should look like this: 
            # 61.0   dkearney       12/9  22:47   0+00:00:00 I  0   0.0  nanowire.sh input-

            parseResults = cmdOutput.split() # re.compile(r'\W+').split(cmdOutput)

            # results should look like this:
            # ['61.0', 'dkearney', '12/9', '22:47', '0+00:00:00', 'I', '0', '0.0', 'nanowire.sh', 'input-']

            retVal = ''

            if len(parseResults) > 6:
                qstat = parseResults[5]
                retVal = condor_msgs[qstat]

            if retVal == '':
                retVal = condor_msgs['DEFAULT']

        return retVal

    def chkCondorLoop(resultTarPathPrefix,numJobs):

        retVal = numJobs
        for i in xrange(numJobs):
            resultTarPath = resultTarPathPrefix + str(i) + ".tar.gz"

            # this might be the point of a race condition,
            # might need to change this to a try/catch statement.
            # check to see that the result file exists, if not raise error
            if (not os.path.exists(resultTarPath)):
                error = 'Condor Result file not created: %s' % (resultTarPath)
                raise RuntimeError, error
                sys.exit(-1)

            # check to see that the result tar size is greater than 0
            resultTarSize = os.path.getsize(resultTarPath)
            if (resultTarSize):
                retVal -= 1

        return retVal

    def runCondor ( inputs,
                    condorScriptPath,
                    resultDirName ):

        # prepare the user output for printing our job submission info to screen

        jobNodeCnt = int(inputs['Nvg'])

        sys.stdout.write ("Your job is set to use %d node(s)\n" % (jobNodeCnt) )
        # sys.stdout.write ("with %d processor(s) per node\n" % int(ppn))
        sys.stdout.write ("Submitting job to queue\n")
        sys.stdout.flush()

        # submit the condor job
        condorCmd = 'condor_submit'
        cmdOutData = getCommandOutput('%s %s' % (condorCmd,condorScriptPath))

        # grab the qID from the return data of the command
        global qID
        qID.append(re.search('cluster [0-9]+', cmdOutData).group().split()[1])

        # track condor's progress
        min = 0
        tCount = 0
        sleepTime = 10
        prevStatus = '?'
        currStatus = '?'
        resultTarNamePrefix = 'results'
        resultTarPathPrefix = resultDirName+'/'+resultTarNamePrefix

        while chkCondorLoop(resultTarPathPrefix,int(inputs['Nvg'])):
            currStatus = chkCondorStatus(qID[0])
            if tCount == 60 :
                min += 1
                tCount = 0
            if (currStatus != prevStatus) :
                sys.stdout.write("%s: %s\n" % ( currStatus, time.ctime() ) )
                prevStatus = currStatus

            sys.stdout.flush()
            time.sleep(sleepTime)
            tCount += sleepTime


        outFiles = {}
        errFiles = {}
        outfileNamePrefix = "out."
        errfileNamePrefix = "err."
        logfileNamePrefix = "log."
        logfileName = resultDirName + logfileNamePrefix + qID[0]

        for i in xrange(int(inputs['Nvg'])):
            resultTarName = resultTarNamePrefix + str(i) + ".tar.gz"
            resultTarPath = resultDirName+'/'+resultTarName

            # check to see that the result file exists, if not raise error
            if (not os.path.exists(resultTarPath)):
                error = 'Returned data not found at %s' % (resultTarPath)
                raise RuntimeError, error
                sys.exit(-1)

            # prepare to return error files and outfiles
            outFiles[i] = resultDirName+'/'+outfileNamePrefix+qID[0]+'.'+str(i)
            errFiles[i] = resultDirName+'/'+errfileNamePrefix+qID[0]+'.'+str(i)

            # check to see that the result tar size is greater than 0
            resultTarSize = os.path.getsize(resultTarPath)
            if (resultTarSize):
                # untar the output and return out and err file texts
    #           print 'tar --directory %s -xzf %s' % (resultDirName,resultTarPath)
                untarRslt = getCommandOutput('tar --directory %s -xzf %s' % (resultDirName,resultTarPath))
    #           print 'untarRslt = %s' % (untarRslt)

            else:
                # tar file is of zero size, simulation did not have ended properly
                error = 'Returned file does not contain any data\n Unsuccessful Run'
                raise RuntimeError, error
                sys.exit(-1)

        return (logfileName, outFiles, errFiles)

#######################################################################
# main python script
#######################################################################

#######################################################################

def createDir(dirName):
    resultDirPath = os.getcwd() + "/" + dirName
    if os.path.exists(resultDirPath):
        shutil.rmtree(resultDirPath)
    os.mkdir(resultDirPath)
    return resultDirPath

if __name__ == '__main__':

    nodes = 5
    walltime = '01:00:00'
    mpiCommand = getCommandOutput('which mpirun')

    if mpiCommand == '':
        # command not found?
        sys.exit(-1)
    else :
        mpiCommand = mpiCommand.strip()

    sys.stdout.write("testing hello\n")
    jobName = 'helloMPITest'
    resultsDir = createDir('1234')
    executable = 'hello'
    shutil.copy('hello/hello',resultsDir)
    myPBSObj =  pbs(jobName,resultsDir,nodes,executable)
    myPBSObj.submit()
    myPBSObj.status()
    sys.stdout.flush()

    sys.stdout.write("testing cycle\n")
    jobName = 'cycleMPITest'
    resultsDir = createDir('5678')
    cmd = '%s -np %d -machinefile $PBS_NODEFILE cycle' % (mpiCommand,nodes)
    shutil.copy('cycle/cycle',resultsDir)
    myPBSObj =  pbs(jobName,resultsDir,nodes,"")
    myPBSObj.cmd(cmd)
    myPBSObj.walltime('00:02:00')
    myPBSObj.submit()
    myPBSObj.status()
    sys.stdout.flush()

    # exit the program
    sys.exit()

#######################################################################
